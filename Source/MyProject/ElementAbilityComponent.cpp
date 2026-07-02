// Project Nova — Element ability component (vertical slice stub)

#include "ElementAbilityComponent.h"

#include "AbilityGraphRuntime.h"
#include "IdentityOverrideComponent.h"
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

	// Vertical slice: register two placeholder abilities so CastAbility1/2 have targets.
	if (!Abilities.Contains(TEXT("Slash")))
	{
		FNovaAbilityGraphDef Slash;
		Slash.AbilityId = TEXT("Slash");
		Slash.DisplayName = NSLOCTEXT("Nova", "AbilitySlash", "Slash");
		Slash.CooldownSeconds = 0.75f;
		Slash.EnergyCost = 10.f;
		FNovaAbilityNode& ShapeNode = Slash.Nodes.AddDefaulted_GetRef();
		ShapeNode.NodeId = TEXT("Shape_Blade");
		ShapeNode.NodeType = ENovaAbilityNodeType::Shape;
		RegisterAbility(Slash);
	}
	if (!Abilities.Contains(TEXT("Edgewall")))
	{
		FNovaAbilityGraphDef Edgewall;
		Edgewall.AbilityId = TEXT("Edgewall");
		Edgewall.DisplayName = NSLOCTEXT("Nova", "AbilityEdgewall", "Edgewall");
		Edgewall.CooldownSeconds = 3.f;
		Edgewall.EnergyCost = 25.f;
		FNovaAbilityNode& SpawnNode = Edgewall.Nodes.AddDefaulted_GetRef();
		SpawnNode.NodeId = TEXT("Spawn_Wall");
		SpawnNode.NodeType = ENovaAbilityNodeType::Spawn;
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
				OnAbilityEvent.Broadcast(AbilityId, TEXT("CastBlocked_Identity"));
				return false;
			}
		}
	}

	if (GetRemainingCooldown(AbilityId) > 0.f)
	{
		OnAbilityEvent.Broadcast(AbilityId, TEXT("CastBlocked_Cooldown"));
		return false;
	}

	if (Energy < Definition->EnergyCost)
	{
		OnAbilityEvent.Broadcast(AbilityId, TEXT("CastBlocked_Cost"));
		return false;
	}

	UNovaAbilityGraphRuntime* Runtime = Runtimes.FindRef(AbilityId);
	if (!Runtime || !Runtime->Execute(GetOwner(), RuntimeParams))
	{
		OnAbilityEvent.Broadcast(AbilityId, TEXT("CastFailed"));
		return false;
	}

	Energy -= Definition->EnergyCost;
	CooldownEndTimes.Add(AbilityId, GetWorld()->GetTimeSeconds() + Definition->CooldownSeconds);

	// VFX hookup point (stub): CastVFX is spawned here once a Niagara asset is assigned.

	OnAbilityEvent.Broadcast(AbilityId, TEXT("CastStarted"));
	return true;
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
