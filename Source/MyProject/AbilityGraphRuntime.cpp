// Project Nova — Ability Graph runtime (vertical slice)

#include "AbilityGraphRuntime.h"

#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NovaBlockingWall.h"
#include "NovaDummyTarget.h"
#include "NovaTetherLink.h"

void UNovaAbilityGraphRuntime::Initialize(const FNovaAbilityGraphDef& InDefinition)
{
	Definition = InDefinition;
	bInitialized = true;
}

bool UNovaAbilityGraphRuntime::Execute(AActor* Instigator, const FNovaAbilityRuntimeParams& Params,
	float DebugDrawSeconds)
{
	if (!bInitialized || !Instigator)
	{
		return false;
	}
	UWorld* World = Instigator->GetWorld();
	if (!World)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[AbilityGraph] Executed '%s' by '%s' (range %.0f, width %.0f, arc %.0f)"),
		*Definition.AbilityId.ToString(),
		*Instigator->GetName(),
		Params.Range, Params.Width, Params.Arc);

	// 1) Shape — resolve geometry. Node overrides pin values; <= 0 inherits the
	//    runtime params so player shaping keeps working.
	const FVector Direction = Params.Direction.GetSafeNormal();
	const float Range = Definition.Shape.RangeOverride > 0.f ? Definition.Shape.RangeOverride : Params.Range;
	const float ArcDegrees = Definition.Shape.ArcDegreesOverride > 0.f ? Definition.Shape.ArcDegreesOverride : Params.Arc;
	const float Radius = Definition.Shape.RadiusOverride > 0.f
		? Definition.Shape.RadiusOverride
		: FMath::Max(Params.Width * 0.5f, 10.f);
	const float HalfArcRad = FMath::DegreesToRadians(FMath::Max(ArcDegrees, 1.f) * 0.5f);

	// 2) Path — where the shape resolves.
	const FVector CastOrigin = Instigator->GetActorLocation();
	FVector EffectOrigin = CastOrigin;
	if (Definition.Path.PathType == ENovaAbilityPathType::Linear)
	{
		EffectOrigin += Direction * Definition.Path.TravelDistance;
	}

	DrawShape(World, CastOrigin, EffectOrigin, Direction, Range, HalfArcRad, Radius, DebugDrawSeconds);

	// 3) Spawn — leave persistent actors behind.
	ExecuteSpawn(World, EffectOrigin, Direction);

	// 4) Affect — apply effects to targets inside the shape (records who was hit).
	TArray<AActor*> AffectedActors;
	ExecuteAffect(World, Instigator, EffectOrigin, Direction, Range, HalfArcRad, DebugDrawSeconds, AffectedActors);

	// 5) Constraint — anchor the effect to a hit target (tether follow).
	ExecuteConstraint(World, Instigator, AffectedActors);

	return true;
}

void UNovaAbilityGraphRuntime::DrawShape(UWorld* World, const FVector& CastOrigin, const FVector& EffectOrigin,
	const FVector& Direction, float Range, float HalfArcRad, float Radius, float DebugDrawSeconds) const
{
	switch (Definition.Shape.ShapeType)
	{
	case ENovaAbilityShapeType::Cone:
		// The pre-node-system Slash visual, kept verbatim: direction line,
		// range/arc cone, width sphere at the far end.
		DrawDebugLine(World, CastOrigin, CastOrigin + Direction * Range, FColor::Magenta, false, DebugDrawSeconds, 0, 3.f);
		DrawDebugCone(World, CastOrigin, Direction, Range, HalfArcRad, HalfArcRad, 16, FColor::Purple, false, DebugDrawSeconds);
		DrawDebugSphere(World, CastOrigin + Direction * Range, Radius, 12, FColor::Cyan, false, DebugDrawSeconds);
		break;

	case ENovaAbilityShapeType::Line:
	{
		// Direction line from caster to wherever the Path resolved the effect;
		// an Instant path resolves in place, so fall back to Range length.
		const FVector LineEnd = (EffectOrigin - CastOrigin).IsNearlyZero()
			? CastOrigin + Direction * Range
			: EffectOrigin;
		DrawDebugLine(World, CastOrigin, LineEnd, FColor::Magenta, false, DebugDrawSeconds, 0, 3.f);
		break;
	}

	case ENovaAbilityShapeType::Sphere:
		DrawDebugSphere(World, EffectOrigin, Radius, 12, FColor::Cyan, false, DebugDrawSeconds);
		break;
	}
}

void UNovaAbilityGraphRuntime::ExecuteSpawn(UWorld* World, const FVector& EffectOrigin, const FVector& Direction) const
{
	if (Definition.Spawn.SpawnType != ENovaAbilitySpawnType::BlockingWall)
	{
		return;
	}

	// Seat the wall on static ground under the effect origin; fall back to the
	// origin height when nothing is below (e.g. cast over a pit). Static-only
	// on purpose: a visibility trace would land on whatever pawn/wall happens
	// to stand there and float the new wall on top of it.
	FVector Center = EffectOrigin;
	FHitResult GroundHit;
	const FVector TraceStart = EffectOrigin + FVector(0.f, 0.f, 200.f);
	const FVector TraceEnd = EffectOrigin - FVector(0.f, 0.f, 1000.f);
	if (World->LineTraceSingleByObjectType(GroundHit, TraceStart, TraceEnd,
			FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		Center.Z = GroundHit.ImpactPoint.Z + Definition.Spawn.WallHalfExtent.Z;
	}

	// Wall X axis (thin side) faces along the cast direction.
	const FRotator Facing(0.f, Direction.Rotation().Yaw, 0.f);
	const FTransform SpawnTransform(Facing, Center);

	if (ANovaBlockingWall* Wall = World->SpawnActorDeferred<ANovaBlockingWall>(
			ANovaBlockingWall::StaticClass(), SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Wall->InitWall(Definition.Spawn.WallHalfExtent, Definition.Spawn.LifeSeconds);
		Wall->FinishSpawning(SpawnTransform);
	}
}

void UNovaAbilityGraphRuntime::ExecuteAffect(UWorld* World, AActor* Instigator, const FVector& EffectOrigin,
	const FVector& Direction, float Range, float HalfArcRad, float DebugDrawSeconds,
	TArray<AActor*>& OutAffected) const
{
	const ENovaAbilityAffectType AffectType = Definition.Affect.AffectType;
	if (AffectType != ENovaAbilityAffectType::Damage && AffectType != ENovaAbilityAffectType::Slow)
	{
		// None / Block: nothing to detect. Block is purely physical — the spawned
		// wall's collision does the work.
		return;
	}

	// Hit check: everything inside the range sphere whose bearing falls within
	// the arc (Cone shapes only; Line/Sphere skip the angle filter).
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NovaAbilityCast), /*bTraceComplex=*/false, Instigator);
	World->OverlapMultiByObjectType(Overlaps, EffectOrigin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Range), QueryParams);

	const bool bUseArcFilter = Definition.Shape.ShapeType == ENovaAbilityShapeType::Cone;
	const float MinDot = FMath::Cos(HalfArcRad);
	const APawn* InstigatorPawn = Cast<APawn>(Instigator);
	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Target == Instigator || HitActors.Contains(Target))
		{
			continue;
		}

		const FVector ToTarget = Target->GetActorLocation() - EffectOrigin;
		if (bUseArcFilter &&
			(ToTarget.IsNearlyZero() || FVector::DotProduct(ToTarget.GetSafeNormal(), Direction) < MinDot))
		{
			continue;
		}

		HitActors.Add(Target); // dedupe: never process the same actor twice this cast

		if (AffectType == ENovaAbilityAffectType::Damage)
		{
			OutAffected.Add(Target);
			UGameplayStatics::ApplyDamage(Target, Definition.Affect.Damage,
				InstigatorPawn ? InstigatorPawn->GetController() : nullptr, Instigator, nullptr);
			DrawDebugSphere(World, Target->GetActorLocation(), 40.f, 12, FColor::Red, false, DebugDrawSeconds);
		}
		else // Slow — only actors that can actually be slowed count as affected, so a
		{    // Constraint tethers to a real target, not an incidental trigger volume.
			ANovaDummyTarget* Dummy = Cast<ANovaDummyTarget>(Target);
			if (!Dummy)
			{
				continue;
			}
			OutAffected.Add(Target);
			Dummy->ApplySlow(Definition.Affect.SlowSpeedMultiplier, Definition.Affect.SlowDurationSeconds);
			DrawDebugSphere(World, Target->GetActorLocation(), 40.f, 12, FColor::Purple, false, DebugDrawSeconds);
		}
	}
}

void UNovaAbilityGraphRuntime::ExecuteConstraint(UWorld* World, AActor* Instigator,
	const TArray<AActor*>& AffectedActors) const
{
	if (Definition.Constraint.ConstraintType != ENovaAbilityConstraintType::TetherToActor
		|| !World || !Instigator || AffectedActors.Num() == 0)
	{
		return;
	}

	// Tether to the affected actor nearest the instigator — a single, deterministic
	// anchor for the minimal Bind. (Multi-tether can come later if a fusion needs it.)
	const FVector Origin = Instigator->GetActorLocation();
	AActor* Anchor = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (AActor* Actor : AffectedActors)
	{
		if (!Actor)
		{
			continue;
		}
		const double DistSq = FVector::DistSquared(Actor->GetActorLocation(), Origin);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Anchor = Actor;
		}
	}
	if (!Anchor)
	{
		return;
	}

	if (ANovaTetherLink* Link = World->SpawnActor<ANovaTetherLink>(
			ANovaTetherLink::StaticClass(), FTransform(Origin)))
	{
		Link->InitTether(Instigator, Anchor, Definition.Constraint.TetherDurationSeconds);
	}
}
