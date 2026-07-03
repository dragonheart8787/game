// Project Nova — Element ability component (vertical slice stub)

#include "ElementAbilityComponent.h"

#include "AbilityGraphRuntime.h"
#include "IdentityOverrideComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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

	UNovaAbilityGraphRuntime* Runtime = Runtimes.FindRef(AbilityId);
	if (!Runtime || !Runtime->Execute(GetOwner(), RuntimeParams))
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastFailed"), TEXT("runtime execute failed"));
		return false;
	}

	Energy -= Definition->EnergyCost;
	CooldownEndTimes.Add(AbilityId, GetWorld()->GetTimeSeconds() + Definition->CooldownSeconds);

	// VFX hookup point (stub): once a Niagara asset is assigned to CastVFX it spawns here.
	// Until then the cast shape is debug-drawn inside ApplyAbilityEffects.
	ApplyAbilityEffects(*Definition);

	BroadcastAbilityEvent(AbilityId, TEXT("CastStarted"),
		FString::Printf(TEXT("W=%.0f Arc=%.0f R=%.0f Dmg=%.0f"),
			RuntimeParams.Width, RuntimeParams.Arc, RuntimeParams.Range, Definition->Damage));
	return true;
}

void UElementAbilityComponent::ApplyAbilityEffects(const FNovaAbilityGraphDef& Definition)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	const FVector Origin = Owner->GetActorLocation();
	const FVector Direction = RuntimeParams.Direction.GetSafeNormal();
	const float Range = RuntimeParams.Range;
	const float HalfArcRad = FMath::DegreesToRadians(FMath::Max(RuntimeParams.Arc, 1.f) * 0.5f);

	// Placeholder VFX: direction line, range/arc cone, width sphere at the far end.
	DrawDebugLine(World, Origin, Origin + Direction * Range, FColor::Magenta, false, DebugDrawSeconds, 0, 3.f);
	DrawDebugCone(World, Origin, Direction, Range, HalfArcRad, HalfArcRad, 16, FColor::Purple, false, DebugDrawSeconds);
	DrawDebugSphere(World, Origin + Direction * Range, FMath::Max(RuntimeParams.Width * 0.5f, 10.f), 12,
		FColor::Cyan, false, DebugDrawSeconds);

	// Hit check: everything inside the range sphere whose bearing falls within the arc.
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NovaAbilityCast), /*bTraceComplex=*/false, Owner);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Range), QueryParams);

	const float MinDot = FMath::Cos(HalfArcRad);
	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Target == Owner || DamagedActors.Contains(Target))
		{
			continue;
		}

		const FVector ToTarget = Target->GetActorLocation() - Origin;
		if (ToTarget.IsNearlyZero() || FVector::DotProduct(ToTarget.GetSafeNormal(), Direction) < MinDot)
		{
			continue;
		}

		DamagedActors.Add(Target);
		const APawn* OwnerPawn = Cast<APawn>(Owner);
		UGameplayStatics::ApplyDamage(Target, Definition.Damage,
			OwnerPawn ? OwnerPawn->GetController() : nullptr, Owner, nullptr);
		DrawDebugSphere(World, Target->GetActorLocation(), 40.f, 12, FColor::Red, false, DebugDrawSeconds);
	}
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
