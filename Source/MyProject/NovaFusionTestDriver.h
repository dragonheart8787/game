// Project Nova — dev-only PIE fusion regression driver (never saved into a level)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilityGraphTypes.h"
#include "NovaFusionTestDriver.generated.h"

class ANovaDummyTarget;
class ANovaTetherLink;
class UElementAbilityComponent;

/**
 * Dev-only regression driver for ability fusion (Patch 7):
 * RendLock = FuseAbilityGraphDefs(Slash, Bind), registered at BeginPlay.
 * Every expectation is DERIVED from the live registered Slash/Bind definitions
 * (DataAsset or C++ default) — nothing is hardcoded, so the test itself proves
 * the fusion tracks its component abilities. Spawn in memory (never save),
 * start PIE, compare LogNovaFusionTest PASS/FAIL lines:
 *   0.5s  Composition: fused def's shape==Slash's, affect chain==Slash.Damage
 *         then Bind.Slow, constraint==Bind's tether, cost==sum, cd==max.
 *   1.0s  Cast RendLock: succeeds, same-frame re-cast blocked by own cooldown,
 *         energy spend == Slash.cost + Bind.cost, Slash/Bind cooldowns stay 0.
 *   1.5s  Dummy took exactly Slash's damage AND is slowed to Bind's multiplier;
 *         tether link spawned on the dummy.
 *   2.0s  Move the dummy sideways.
 *   3.5s  Tether endpoint followed the moved dummy.
 *   5.0s  Slow auto-restored and tether released (Bind's durations).
 *   5.5s  Independence: Slash, Bind and RendLock each cast once, each holds its
 *         own cooldown, none blocks the others.
 */
UCLASS(NotBlueprintable)
class ANovaFusionTestDriver : public AActor
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

private:

	UElementAbilityComponent* GetAbilities() const;
	ANovaDummyTarget* GetDummy() const;
	ANovaTetherLink* FindTether() const;

	void Schedule(float Delay, void (ANovaFusionTestDriver::*Step)());

	// --- Timed steps ---
	void StepVerifyComposition();
	void StepCast();
	void StepVerifyHit();
	void StepMoveDummy();
	void StepVerifyFollow();
	void StepVerifyRestore();
	void StepIndependence();
	void StepDone();

	TArray<FTimerHandle> StepHandles;

	/** Live component defs cached at composition check; every later expectation
	 *  (damage, slow, costs) reads from these. */
	FNovaAbilityGraphDef SlashDef;
	FNovaAbilityGraphDef BindDef;
	FNovaAbilityGraphDef RendLockDef;
	bool bDefsValid = false;

	TWeakObjectPtr<ANovaDummyTarget> Dummy;
	FVector DummyHomeLocation = FVector::ZeroVector;
	float BaseSpeed = 0.f;
	float HealthBeforeCast = 0.f;
};
