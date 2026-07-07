// Project Nova — dev-only PIE Identity Override regression driver (never saved into a level)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaIdentityTestDriver.generated.h"

class ANovaPlayerCharacter;
class UElementAbilityComponent;
class UIdentityOverrideComponent;
class UStoryDirectorSubsystem;

/**
 * Dev-only regression driver for the Identity Override system (Patch 5).
 * Spawn it into the level in memory (do not save), start PIE, and it runs a
 * fixed game-time sequence through all three override modes plus the
 * Story-Director hookup:
 *   1.0-3.8s  Full override: swapped loadout (Dmg=55, cd 2.0), Edgewall gone,
 *             dash locked; revert restores loadout + dash.
 *   5.5-8.5s  Partial override: exposure +35 per cast, threshold event at 100,
 *             revert zeroes the meter.
 *  10.0-14.0s Lens override standalone: Hold mask + camera on the dummy,
 *             auto-revert after 3s.
 *  15.5-20.0s StoryB_Demo drives ApplyLens via its LensIn beat.
 * Every checkpoint logs a state line mirroring the debug overlay's getters.
 */
UCLASS(NotBlueprintable)
class ANovaIdentityTestDriver : public AActor
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	/** Yaw for the free-dash step; overridable at spawn if level geometry or
	 *  story trigger zones occupy the default (-X) path. */
	UPROPERTY(EditAnywhere, Category="Test")
	float FreeDashYawDegrees = 180.f;

private:

	ANovaPlayerCharacter* GetPlayerCharacter() const;
	UElementAbilityComponent* GetAbilities() const;
	UIdentityOverrideComponent* GetIdentity() const;
	UStoryDirectorSubsystem* GetDirector() const;

	/** One state line per checkpoint: mode/identity/dash/exposure/mask/viewtarget/story. */
	void LogState(const TCHAR* Tag) const;

	void CastSlash(bool bAimAtDummy) const;

	void Schedule(float Delay, void (ANovaIdentityTestDriver::*Step)());

	// --- Timed steps ---
	void StepFullApply();
	void StepFullCast();
	void StepFullDash();
	void StepFullEdge();
	void StepFullRevert();
	void StepNormalCast();
	void StepFreeDash();
	void StepPartialApply();
	void StepPartialCast();
	void StepPartialRevert();
	void StepLensApply();
	void StepLensMid();
	void StepLensCast();
	void StepLensAfter();
	void StepStoryStart();
	void StepStoryMid();
	void StepStoryEnd();

	TArray<FTimerHandle> StepHandles;
};
