// Project Nova — dev-only PIE fusion regression driver

#include "NovaFusionTestDriver.h"

#include "ElementAbilityComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NovaDummyTarget.h"
#include "NovaTetherLink.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaFusionTest, Log, All);

namespace
{
	void LogCheck(bool bPass, const FString& What)
	{
		UE_LOG(LogNovaFusionTest, Log, TEXT("[FusionTest] %s %s"),
			bPass ? TEXT("[PASS]") : TEXT("[FAIL]"), *What);
	}
}

void ANovaFusionTestDriver::BeginPlay()
{
	Super::BeginPlay();

	// Same wide gaps as the Bind driver: background-window PIE runs at ~3 FPS,
	// so any check that depends on a per-frame Tick (tether follow) must sit
	// several frames after the state change it observes.
	UE_LOG(LogNovaFusionTest, Log,
		TEXT("[FusionTest] armed: composition@0.5s, cast@1.0s, verify-hit@1.5s, move@2.0s, ")
		TEXT("verify-follow@3.5s, verify-restore@5.0s, independence@5.5s"));

	Schedule(0.5f, &ANovaFusionTestDriver::StepVerifyComposition);
	Schedule(1.0f, &ANovaFusionTestDriver::StepCast);
	Schedule(1.5f, &ANovaFusionTestDriver::StepVerifyHit);
	Schedule(2.0f, &ANovaFusionTestDriver::StepMoveDummy);
	Schedule(3.5f, &ANovaFusionTestDriver::StepVerifyFollow);
	Schedule(5.0f, &ANovaFusionTestDriver::StepVerifyRestore);
	Schedule(5.5f, &ANovaFusionTestDriver::StepIndependence);
	Schedule(6.0f, &ANovaFusionTestDriver::StepDone);
}

void ANovaFusionTestDriver::Schedule(float Delay, void (ANovaFusionTestDriver::*Step)())
{
	FTimerHandle& Handle = StepHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, this, Step, Delay, false);
}

UElementAbilityComponent* ANovaFusionTestDriver::GetAbilities() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return Pawn ? Pawn->FindComponentByClass<UElementAbilityComponent>() : nullptr;
}

ANovaDummyTarget* ANovaFusionTestDriver::GetDummy() const
{
	for (TActorIterator<ANovaDummyTarget> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

ANovaTetherLink* ANovaFusionTestDriver::FindTether() const
{
	for (TActorIterator<ANovaTetherLink> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ANovaFusionTestDriver::StepVerifyComposition()
{
	UElementAbilityComponent* Abilities = GetAbilities();
	if (!Abilities)
	{
		LogCheck(false, TEXT("composition: no ability component"));
		return;
	}

	const bool bSlash = Abilities->GetAbilityDefinition(TEXT("Slash"), SlashDef);
	const bool bBind = Abilities->GetAbilityDefinition(TEXT("Bind"), BindDef);
	const bool bRend = Abilities->GetAbilityDefinition(TEXT("RendLock"), RendLockDef);
	bDefsValid = bSlash && bBind && bRend;
	LogCheck(bDefsValid, FString::Printf(
		TEXT("all three defs registered (Slash=%d Bind=%d RendLock=%d)"), bSlash, bBind, bRend));
	if (!bDefsValid)
	{
		return;
	}

	// Delivery inherited from Slash, byte-for-byte on the shape overrides so
	// runtime-param inheritance (<=0 slots) behaves identically.
	LogCheck(RendLockDef.Shape.ShapeType == SlashDef.Shape.ShapeType
			&& RendLockDef.Shape.ArcDegreesOverride == SlashDef.Shape.ArcDegreesOverride
			&& RendLockDef.Shape.RangeOverride == SlashDef.Shape.RangeOverride
			&& RendLockDef.Shape.RadiusOverride == SlashDef.Shape.RadiusOverride
			&& RendLockDef.Path.PathType == SlashDef.Path.PathType,
		TEXT("shape+path inherited from Slash"));

	// Affect chain = Slash's Damage first, Bind's Slow appended (P8: one array).
	const bool bParts = SlashDef.Affects.Num() >= 1 && BindDef.Affects.Num() >= 1;
	const bool bChain = bParts
		&& RendLockDef.Affects.Num() == 2
		&& RendLockDef.Affects[0].AffectType == ENovaAbilityAffectType::Damage
		&& RendLockDef.Affects[0].Damage == SlashDef.Affects[0].Damage
		&& RendLockDef.Affects[1].AffectType == ENovaAbilityAffectType::Slow
		&& RendLockDef.Affects[1].SlowSpeedMultiplier == BindDef.Affects[0].SlowSpeedMultiplier
		&& RendLockDef.Affects[1].SlowDurationSeconds == BindDef.Affects[0].SlowDurationSeconds;
	LogCheck(bChain, FString::Printf(
		TEXT("affect chain = Slash.Damage(%.0f) + Bind.Slow(%.2fx/%.1fs)"),
		bParts ? SlashDef.Affects[0].Damage : -1.f,
		bParts ? BindDef.Affects[0].SlowSpeedMultiplier : -1.f,
		bParts ? BindDef.Affects[0].SlowDurationSeconds : -1.f));

	// Constraint inherited from Bind (Slash has none).
	LogCheck(RendLockDef.Constraint.ConstraintType == ENovaAbilityConstraintType::TetherToActor
			&& RendLockDef.Constraint.TetherDurationSeconds == BindDef.Constraint.TetherDurationSeconds,
		TEXT("constraint inherited from Bind (TetherToActor)"));

	// Costs derived, not authored: sum / max (P8 Task B: node fields).
	LogCheck(RendLockDef.Cost.Amount == SlashDef.Cost.Amount + BindDef.Cost.Amount
			&& RendLockDef.Cooldown.CooldownSeconds
				== FMath::Max(SlashDef.Cooldown.CooldownSeconds, BindDef.Cooldown.CooldownSeconds),
		FString::Printf(TEXT("cost %.0f == %.0f+%.0f, cd %.1f == max(%.1f, %.1f)"),
			RendLockDef.Cost.Amount, SlashDef.Cost.Amount, BindDef.Cost.Amount,
			RendLockDef.Cooldown.CooldownSeconds,
			SlashDef.Cooldown.CooldownSeconds, BindDef.Cooldown.CooldownSeconds));

	// P8 Task B reserved fields: everything still spends Energy, and nobody has
	// a shared cooldown group until that system actually lands.
	LogCheck(RendLockDef.Cost.CostType == ENovaAbilityCostType::Energy
			&& RendLockDef.Cooldown.SharedCooldownGroup == NAME_None
			&& SlashDef.Cooldown.SharedCooldownGroup == NAME_None
			&& BindDef.Cooldown.SharedCooldownGroup == NAME_None,
		TEXT("cost type Energy, SharedCooldownGroup defaults to None everywhere"));
}

void ANovaFusionTestDriver::StepCast()
{
	UElementAbilityComponent* Abilities = GetAbilities();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	ANovaDummyTarget* Target = GetDummy();
	if (!Abilities || !Pawn || !Target || !bDefsValid)
	{
		LogCheck(false, FString::Printf(TEXT("cast setup: abilities=%d pawn=%d dummy=%d defs=%d"),
			Abilities != nullptr, Pawn != nullptr, Target != nullptr, bDefsValid));
		return;
	}

	Dummy = Target;
	BaseSpeed = Target->GetBaseMoveSpeed();
	DummyHomeLocation = Target->GetActorLocation();
	HealthBeforeCast = Target->GetHealth();

	// Deterministic hit: dummy straight ahead inside Slash's cone range.
	const FVector CastSpot = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 250.f;
	Target->SetActorLocation(CastSpot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	Abilities->SetRuntimeDirection(Target->GetActorLocation() - Pawn->GetActorLocation());

	const float EnergyBefore = Abilities->GetEnergy();
	UE_LOG(LogNovaFusionTest, Log,
		TEXT("[FusionTest] step cast: RendLock (health %.0f, energy %.0f, base speed %.0f)"),
		HealthBeforeCast, EnergyBefore, BaseSpeed);

	const bool bFirst = Abilities->CastAbilityById(TEXT("RendLock"));
	const bool bSecond = Abilities->CastAbilityById(TEXT("RendLock"));
	const float EnergyAfter = Abilities->GetEnergy();

	LogCheck(bFirst, TEXT("first RendLock cast succeeded"));
	LogCheck(!bSecond && Abilities->GetRemainingCooldown(TEXT("RendLock")) > 0.f,
		FString::Printf(TEXT("same-frame re-cast blocked by own cooldown (%.1fs remaining)"),
			Abilities->GetRemainingCooldown(TEXT("RendLock"))));
	LogCheck(FMath::IsNearlyEqual(EnergyBefore - EnergyAfter, RendLockDef.Cost.Amount, 0.5f),
		FString::Printf(TEXT("energy spent %.0f == Slash+Bind cost (%.0f -> %.0f)"),
			RendLockDef.Cost.Amount, EnergyBefore, EnergyAfter));

	// Casting the fusion must not touch the component abilities' cooldowns.
	LogCheck(Abilities->GetRemainingCooldown(TEXT("Slash")) == 0.f
			&& Abilities->GetRemainingCooldown(TEXT("Bind")) == 0.f,
		TEXT("RendLock cast left Slash and Bind cooldowns at 0"));
}

void ANovaFusionTestDriver::StepVerifyHit()
{
	ANovaDummyTarget* Target = Dummy.Get();
	if (!Target || !bDefsValid)
	{
		LogCheck(false, TEXT("verify-hit: dummy/defs missing"));
		return;
	}

	// Both halves of the fusion landed on the same cast: exactly Slash's damage
	// AND Bind's slow, against the live component defs.
	const float Health = Target->GetHealth();
	const float ExpectedHealth = HealthBeforeCast
		- (SlashDef.Affects.Num() > 0 ? SlashDef.Affects[0].Damage : 0.f);
	LogCheck(FMath::IsNearlyEqual(Health, ExpectedHealth, 0.1f),
		FString::Printf(TEXT("dummy took Slash's damage (health %.0f -> %.0f, expected %.0f)"),
			HealthBeforeCast, Health, ExpectedHealth));

	const float Speed = Target->GetCurrentMoveSpeed();
	const float ExpectedSpeed = BaseSpeed
		* (BindDef.Affects.Num() > 0 ? BindDef.Affects[0].SlowSpeedMultiplier : 1.f);
	LogCheck(Target->IsSlowed() && FMath::IsNearlyEqual(Speed, ExpectedSpeed, 1.f),
		FString::Printf(TEXT("dummy slowed to Bind's multiplier (%.0f, expected %.0f)"), Speed, ExpectedSpeed));

	ANovaTetherLink* Tether = FindTether();
	LogCheck(Tether != nullptr && Tether->GetTetherTarget() == Target,
		TEXT("tether link spawned on the dummy"));
}

void ANovaFusionTestDriver::StepMoveDummy()
{
	ANovaDummyTarget* Target = Dummy.Get();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Target || !Pawn)
	{
		LogCheck(false, TEXT("move-dummy: dummy/pawn missing"));
		return;
	}

	const FVector MovedLocation = Pawn->GetActorLocation()
		+ Pawn->GetActorForwardVector() * 250.f
		+ Pawn->GetActorRightVector() * 300.f;
	Target->SetActorLocation(MovedLocation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(LogNovaFusionTest, Log, TEXT("[FusionTest] step move: dummy -> %s"), *MovedLocation.ToCompactString());
}

void ANovaFusionTestDriver::StepVerifyFollow()
{
	ANovaDummyTarget* Target = Dummy.Get();
	ANovaTetherLink* Tether = FindTether();
	if (!Target || !Tether)
	{
		LogCheck(false, FString::Printf(TEXT("verify-follow: dummy=%d tether=%d"),
			Target != nullptr, Tether != nullptr));
		return;
	}

	const float Drift = FVector::Dist(Tether->GetLastTargetLocation(), Target->GetActorLocation());
	LogCheck(Tether->GetTetherTarget() == Target && Drift < 5.f,
		FString::Printf(TEXT("tether followed moved dummy (endpoint drift %.2fcm)"), Drift));
}

void ANovaFusionTestDriver::StepVerifyRestore()
{
	ANovaDummyTarget* Target = Dummy.Get();
	if (!Target)
	{
		LogCheck(false, TEXT("verify-restore: dummy missing"));
		return;
	}

	const float Speed = Target->GetCurrentMoveSpeed();
	LogCheck(!Target->IsSlowed() && FMath::IsNearlyEqual(Speed, BaseSpeed, 1.f),
		FString::Printf(TEXT("dummy speed restored to %.0f (base %.0f)"), Speed, BaseSpeed));
	LogCheck(FindTether() == nullptr, TEXT("tether released after Bind's duration"));
}

void ANovaFusionTestDriver::StepIndependence()
{
	UElementAbilityComponent* Abilities = GetAbilities();
	if (!Abilities || !bDefsValid)
	{
		LogCheck(false, TEXT("independence: abilities/defs missing"));
		return;
	}

	// All cooldowns have expired by now. Cast all three in one frame: each must
	// succeed against its own cooldown only — a component ability on cooldown
	// must never gate the fusion, and vice versa.
	const bool bSlashCast = Abilities->CastAbilityById(TEXT("Slash"));
	const bool bSlashOnly = Abilities->GetRemainingCooldown(TEXT("Slash")) > 0.f
		&& Abilities->GetRemainingCooldown(TEXT("Bind")) == 0.f
		&& Abilities->GetRemainingCooldown(TEXT("RendLock")) == 0.f;
	LogCheck(bSlashCast && bSlashOnly, TEXT("Slash cast holds only Slash's cooldown"));

	const bool bBindCast = Abilities->CastAbilityById(TEXT("Bind"));
	const bool bRendReady = Abilities->GetRemainingCooldown(TEXT("RendLock")) == 0.f;
	LogCheck(bBindCast && bRendReady, TEXT("Bind cast leaves RendLock ready"));

	// Decisive: both components now on cooldown, the fusion still casts.
	const bool bRendCast = Abilities->CastAbilityById(TEXT("RendLock"));
	LogCheck(bRendCast, TEXT("RendLock cast while Slash AND Bind on cooldown"));
}

void ANovaFusionTestDriver::StepDone()
{
	// Leave the level as we found it (scene is never saved, but be tidy).
	if (ANovaDummyTarget* Target = Dummy.Get())
	{
		Target->SetActorLocation(DummyHomeLocation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}
	UE_LOG(LogNovaFusionTest, Log, TEXT("[FusionTest] done"));
}
