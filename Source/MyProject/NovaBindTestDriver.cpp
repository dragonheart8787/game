// Project Nova — dev-only PIE Bind regression driver

#include "NovaBindTestDriver.h"

#include "ElementAbilityComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NovaDummyTarget.h"
#include "NovaTetherLink.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaBindTest, Log, All);

namespace
{
	void LogCheck(bool bPass, const FString& What)
	{
		UE_LOG(LogNovaBindTest, Log, TEXT("[BindTest] %s %s"),
			bPass ? TEXT("[PASS]") : TEXT("[FAIL]"), *What);
	}
}

void ANovaBindTestDriver::BeginPlay()
{
	Super::BeginPlay();

	// Gaps are deliberately wide: the editor throttles PIE to a few FPS when it
	// isn't the foreground window, so move->check must straddle several frames
	// (>= ~0.33s each) for the tether's per-frame Tick to re-run after a move.
	UE_LOG(LogNovaBindTest, Log,
		TEXT("[BindTest] armed: cast@1.0s, verify-slow@1.5s, move@2.0s, verify-follow@3.5s, verify-restore@5.0s"));

	Schedule(1.0f, &ANovaBindTestDriver::StepCast);
	Schedule(1.5f, &ANovaBindTestDriver::StepVerifySlow);
	Schedule(2.0f, &ANovaBindTestDriver::StepMoveDummy);
	Schedule(3.5f, &ANovaBindTestDriver::StepVerifyFollow);
	Schedule(5.0f, &ANovaBindTestDriver::StepVerifyRestore);
	Schedule(5.3f, &ANovaBindTestDriver::StepDone);
}

void ANovaBindTestDriver::Schedule(float Delay, void (ANovaBindTestDriver::*Step)())
{
	FTimerHandle& Handle = StepHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, this, Step, Delay, false);
}

UElementAbilityComponent* ANovaBindTestDriver::GetAbilities() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return Pawn ? Pawn->FindComponentByClass<UElementAbilityComponent>() : nullptr;
}

ANovaDummyTarget* ANovaBindTestDriver::GetDummy() const
{
	for (TActorIterator<ANovaDummyTarget> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

ANovaTetherLink* ANovaBindTestDriver::FindTether() const
{
	for (TActorIterator<ANovaTetherLink> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ANovaBindTestDriver::StepCast()
{
	UElementAbilityComponent* Abilities = GetAbilities();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	ANovaDummyTarget* Target = GetDummy();
	if (!Abilities || !Pawn || !Target)
	{
		LogCheck(false, FString::Printf(TEXT("setup: abilities=%d pawn=%d dummy=%d"),
			Abilities != nullptr, Pawn != nullptr, Target != nullptr));
		return;
	}

	Dummy = Target;
	BaseSpeed = Target->GetBaseMoveSpeed();
	DummyHomeLocation = Target->GetActorLocation();

	// Put the dummy at a deterministic spot inside Bind's 400cm sphere so the hit
	// never depends on where the level happened to place it.
	const FVector CastSpot = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 250.f;
	Target->SetActorLocation(CastSpot, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	const float EnergyBefore = Abilities->GetEnergy();
	Abilities->SetRuntimeDirection(Target->GetActorLocation() - Pawn->GetActorLocation());

	UE_LOG(LogNovaBindTest, Log, TEXT("[BindTest] step cast: Bind + same-frame re-cast (base speed %.0f, energy %.0f)"),
		BaseSpeed, EnergyBefore);

	const bool bFirst = Abilities->CastAbilityById(TEXT("Bind"));
	const bool bSecond = Abilities->CastAbilityById(TEXT("Bind"));
	const float EnergyAfter = Abilities->GetEnergy();
	const float Cooldown = Abilities->GetRemainingCooldown(TEXT("Bind"));

	LogCheck(bFirst, TEXT("first Bind cast succeeded"));
	LogCheck(!bSecond && Cooldown > 0.f,
		FString::Printf(TEXT("same-frame re-cast blocked by cooldown (%.1fs remaining)"), Cooldown));
	LogCheck(FMath::IsNearlyEqual(EnergyBefore - EnergyAfter, 15.f, 0.5f),
		FString::Printf(TEXT("energy spent 15 (%.0f -> %.0f)"), EnergyBefore, EnergyAfter));
}

void ANovaBindTestDriver::StepVerifySlow()
{
	ANovaDummyTarget* Target = Dummy.Get();
	if (!Target)
	{
		LogCheck(false, TEXT("verify-slow: dummy missing"));
		return;
	}

	const float Speed = Target->GetCurrentMoveSpeed();
	const float Expected = BaseSpeed * 0.5f;
	LogCheck(Target->IsSlowed() && FMath::IsNearlyEqual(Speed, Expected, 1.f),
		FString::Printf(TEXT("dummy slowed to %.0f (expected %.0f, base %.0f)"), Speed, Expected, BaseSpeed));

	LogCheck(FindTether() != nullptr, TEXT("tether link spawned"));
}

void ANovaBindTestDriver::StepMoveDummy()
{
	ANovaDummyTarget* Target = Dummy.Get();
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Target || !Pawn)
	{
		LogCheck(false, TEXT("move-dummy: dummy/pawn missing"));
		return;
	}

	// Slide the dummy sideways so the tether has to redraw to a new endpoint.
	DummyMovedLocation = Pawn->GetActorLocation()
		+ Pawn->GetActorForwardVector() * 250.f
		+ Pawn->GetActorRightVector() * 300.f;
	Target->SetActorLocation(DummyMovedLocation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	UE_LOG(LogNovaBindTest, Log, TEXT("[BindTest] step move: dummy -> %s"), *DummyMovedLocation.ToCompactString());
}

void ANovaBindTestDriver::StepVerifyFollow()
{
	ANovaDummyTarget* Target = Dummy.Get();
	ANovaTetherLink* Tether = FindTether();
	if (!Target || !Tether)
	{
		LogCheck(false, FString::Printf(TEXT("verify-follow: dummy=%d tether=%d"),
			Target != nullptr, Tether != nullptr));
		return;
	}

	// The tether redraws to the target's live location every tick, so its last
	// endpoint should track the dummy we just moved.
	const FVector Endpoint = Tether->GetLastTargetLocation();
	const float Drift = FVector::Dist(Endpoint, Target->GetActorLocation());
	LogCheck(Tether->GetTetherTarget() == Target && Drift < 5.f,
		FString::Printf(TEXT("tether followed moved dummy (endpoint drift %.2fcm)"), Drift));
}

void ANovaBindTestDriver::StepVerifyRestore()
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

	LogCheck(FindTether() == nullptr, TEXT("tether released after duration"));
}

void ANovaBindTestDriver::StepDone()
{
	// Leave the level as we found it (scene is never saved, but be tidy).
	if (ANovaDummyTarget* Target = Dummy.Get())
	{
		Target->SetActorLocation(DummyHomeLocation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}
	UE_LOG(LogNovaBindTest, Log, TEXT("[BindTest] done"));
}
