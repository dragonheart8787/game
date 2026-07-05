// Project Nova — Element ability component (vertical slice stub)

#include "ElementAbilityComponent.h"

#include "AbilityGraphRuntime.h"
#include "IdentityOverrideComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogNovaAbility);

UElementAbilityComponent::UElementAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UElementAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	Energy = MaxEnergy;

	// Vertical slice: register the two demo abilities as node compositions.
	// Real defs load from /Content/Data/Abilities JSON/DataAssets later.
	if (!Abilities.Contains(TEXT("Slash")))
	{
		// Slash = Cone shape + Instant path + Damage affect. Geometry inherits
		// the runtime params (overrides stay 0) so player shaping applies.
		FNovaAbilityGraphDef Slash;
		Slash.AbilityId = TEXT("Slash");
		Slash.DisplayName = NSLOCTEXT("Nova", "AbilitySlash", "Slash");
		Slash.CooldownSeconds = 0.75f;
		Slash.EnergyCost = 10.f;
		Slash.Shape.ShapeType = ENovaAbilityShapeType::Cone;
		Slash.Path.PathType = ENovaAbilityPathType::Instant;
		Slash.Spawn.SpawnType = ENovaAbilitySpawnType::None;
		Slash.Affect.AffectType = ENovaAbilityAffectType::Damage;
		Slash.Affect.Damage = 20.f;
		RegisterAbility(Slash);
	}
	if (!Abilities.Contains(TEXT("Edgewall")))
	{
		// Edgewall = Line shape + Linear path + BlockingWall spawn. The wall's
		// own collision does the blocking, so Affect stays None.
		FNovaAbilityGraphDef Edgewall;
		Edgewall.AbilityId = TEXT("Edgewall");
		Edgewall.DisplayName = NSLOCTEXT("Nova", "AbilityEdgewall", "Edgewall");
		Edgewall.CooldownSeconds = 3.f;
		Edgewall.EnergyCost = 25.f;
		Edgewall.Shape.ShapeType = ENovaAbilityShapeType::Line;
		Edgewall.Shape.RangeOverride = 300.f;
		Edgewall.Path.PathType = ENovaAbilityPathType::Linear;
		Edgewall.Path.TravelDistance = 300.f;
		Edgewall.Spawn.SpawnType = ENovaAbilitySpawnType::BlockingWall;
		Edgewall.Spawn.WallHalfExtent = FVector(30.f, 200.f, 150.f);
		Edgewall.Spawn.LifeSeconds = 5.f;
		Edgewall.Affect.AffectType = ENovaAbilityAffectType::None;
		Edgewall.Affect.Damage = 0.f;
		RegisterAbility(Edgewall);
	}
}

void UElementAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Energy = FMath::Min(MaxEnergy, Energy + EnergyRegenPerSecond * DeltaTime);
}

void UElementAbilityComponent::RegisterAbility(const FNovaAbilityGraphDef& Definition)
{
	if (Definition.AbilityId.IsNone())
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("RegisterAbility: rejected definition with no AbilityId"));
		return;
	}

	Abilities.Add(Definition.AbilityId, Definition);

	UNovaAbilityGraphRuntime* Runtime = NewObject<UNovaAbilityGraphRuntime>(this);
	Runtime->Initialize(Definition);
	Runtimes.Add(Definition.AbilityId, Runtime);
}

bool UElementAbilityComponent::CastAbilityById(FName AbilityId)
{
	const FNovaAbilityGraphDef* Definition = Abilities.Find(AbilityId);
	if (!Definition)
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("CastAbilityById: unknown ability '%s'"), *AbilityId.ToString());
		return false;
	}

	// Identity Override can forbid ability use (e.g. Partial Override in public).
	if (const AActor* Owner = GetOwner())
	{
		if (const UIdentityOverrideComponent* Identity = Owner->FindComponentByClass<UIdentityOverrideComponent>())
		{
			if (Identity->AreAbilitiesRestricted())
			{
				BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Identity"),
					TEXT("abilities restricted by identity override"));
				return false;
			}
		}
	}

	const float Remaining = GetRemainingCooldown(AbilityId);
	if (Remaining > 0.f)
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Cooldown"),
			FString::Printf(TEXT("%.1fs remaining"), Remaining));
		return false;
	}

	if (Energy < Definition->EnergyCost)
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Cost"),
			FString::Printf(TEXT("need %.0f, have %.0f"), Definition->EnergyCost, Energy));
		return false;
	}

	// Single execution path: Shape -> Path -> Spawn -> Affect all run inside the
	// graph runtime. Debug draws stand in for VFX until CastVFX is hooked up.
	UNovaAbilityGraphRuntime* Runtime = Runtimes.FindRef(AbilityId);
	if (!Runtime || !Runtime->Execute(GetOwner(), RuntimeParams, DebugDrawSeconds))
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastFailed"), TEXT("runtime execute failed"));
		return false;
	}

	Energy -= Definition->EnergyCost;
	CooldownEndTimes.Add(AbilityId, GetWorld()->GetTimeSeconds() + Definition->CooldownSeconds);

	BroadcastAbilityEvent(AbilityId, TEXT("CastStarted"),
		FString::Printf(TEXT("W=%.0f Arc=%.0f R=%.0f Dmg=%.0f"),
			RuntimeParams.Width, RuntimeParams.Arc, RuntimeParams.Range, Definition->Affect.Damage));
	return true;
}

void UElementAbilityComponent::BroadcastAbilityEvent(FName AbilityId, FName EventType, const FString& Detail)
{
	LastAbilityEventText = FString::Printf(TEXT("%s: %s (%s)"),
		*AbilityId.ToString(), *EventType.ToString(), *Detail);
	UE_LOG(LogNovaAbility, Log, TEXT("%s"), *LastAbilityEventText);
	OnAbilityEvent.Broadcast(AbilityId, EventType);
}

void UElementAbilityComponent::SetRuntimeDirection(const FVector& Direction)
{
	RuntimeParams.Direction = Direction.GetSafeNormal();
}

void UElementAbilityComponent::SetRuntimeWidth(float Width)
{
	RuntimeParams.Width = FMath::Max(0.f, Width);
}

void UElementAbilityComponent::SetRuntimeArc(float Arc)
{
	RuntimeParams.Arc = FMath::Clamp(Arc, 0.f, 360.f);
}

void UElementAbilityComponent::SetRuntimeRange(float Range)
{
	RuntimeParams.Range = FMath::Max(0.f, Range);
}

float UElementAbilityComponent::GetRemainingCooldown(FName AbilityId) const
{
	const double* EndTime = CooldownEndTimes.Find(AbilityId);
	if (!EndTime || !GetWorld())
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(*EndTime - GetWorld()->GetTimeSeconds()));
}

TArray<FName> UElementAbilityComponent::GetRegisteredAbilityIds() const
{
	TArray<FName> Ids;
	Abilities.GenerateKeyArray(Ids);
	return Ids;
}
