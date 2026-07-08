// Project Nova — dev-only PIE Bind regression driver (never saved into a level)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaBindTestDriver.generated.h"

class ANovaDummyTarget;
class UElementAbilityComponent;

/**
 * Dev-only regression driver for the Bind ability (Patch 6):
 * Shape(Sphere) -> Path(Instant) -> Affect(Slow 50%/3s) -> Constraint(TetherToActor 3s).
 * Spawn it into the level in memory (do not save), start PIE, and it runs a fixed
 * game-time sequence against the level's dummy and logs PASS/FAIL per checkpoint:
 *   1.0s  Teleport dummy in range, cast Bind + same-frame re-cast (expect
 *         CastBlocked_Cooldown), record energy spend.
 *   1.3s  Assert dummy move speed dropped to 50% (150 of 300) and IsSlowed.
 *   1.6s  Move the dummy to a new spot.
 *   1.8s  Assert the tether link's last-drawn endpoint followed the moved dummy.
 *   4.3s  Assert speed auto-restored (300, not slowed) and the tether is gone.
 * Compare LogNovaBindTest / LogNovaAbility / LogNovaDummy / LogNovaTether output.
 */
UCLASS(NotBlueprintable)
class ANovaBindTestDriver : public AActor
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

private:

	UElementAbilityComponent* GetAbilities() const;
	ANovaDummyTarget* GetDummy() const;

	/** Find the live tether link, if any. */
	class ANovaTetherLink* FindTether() const;

	void Schedule(float Delay, void (ANovaBindTestDriver::*Step)());

	// --- Timed steps ---
	void StepCast();
	void StepVerifySlow();
	void StepMoveDummy();
	void StepVerifyFollow();
	void StepVerifyRestore();
	void StepDone();

	TArray<FTimerHandle> StepHandles;

	/** Cached at StepCast so restore/cleanup can compare against it. */
	TWeakObjectPtr<ANovaDummyTarget> Dummy;
	FVector DummyHomeLocation = FVector::ZeroVector;
	FVector DummyMovedLocation = FVector::ZeroVector;
	float BaseSpeed = 0.f;
};
