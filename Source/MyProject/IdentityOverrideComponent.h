// Project Nova — Identity Override component (Patch 5: minimal verifiable core)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IdentityOverrideComponent.generated.h"

class UNovaAbilityDataAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogNovaIdentity, Log, All);

/** Identity Overwrite modes. */
UENUM(BlueprintType)
enum class ENovaIdentityOverrideMode : uint8
{
	None,
	/** Player becomes another identity entirely (chapter-level). */
	Full,
	/** Player stays themselves but is constrained (infiltration/disguise). */
	Partial,
	/** Player only borrows a viewpoint/information feed. */
	Lens
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNovaOnIdentityChanged, ENovaIdentityOverrideMode, Mode, FName, IdentityId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNovaOnExposureMaxed, FName, IdentityId);

/**
 * Identity Overwrite state on the player (Patch 5 minimal core).
 *  - Full: swaps the ability loadout (Patch 4 DataAsset mechanism) and locks Dash.
 *  - Partial: keeps abilities, arms an exposure meter that rises per cast and
 *    fires OnExposureMaxed once at ExposureThreshold.
 *  - Lens: Director Hold mask + camera to an observation point, auto-reverts
 *    after LensDurationSeconds.
 *  - Revert: restores the no-override baseline from any mode; idempotent.
 * Relationship webs / quest goals / NPC reactions are explicitly out of scope.
 */
UCLASS(ClassGroup=(Nova), meta=(BlueprintSpawnableComponent))
class UIdentityOverrideComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UIdentityOverrideComponent();

	/** Player literally becomes IdentityId: ability loadout swapped, Dash locked. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyFullOverride(FName IdentityId);

	/** Player stays themselves; abilities allowed but every cast raises exposure. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyPartialOverride(FName IdentityId);

	/** Player borrows a viewpoint: control Held, camera on the observation point,
	 *  auto-revert after LensDurationSeconds. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyLensOverride(FName IdentityId);

	/** Return to the player's own identity from any mode. Safe to call when idle. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void RevertOverride();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	ENovaIdentityOverrideMode GetOverrideMode() const { return OverrideMode; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	FName GetActiveIdentityId() const { return ActiveIdentityId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	bool AreAbilitiesRestricted() const { return bAbilitiesRestricted; }

	/** True while the current identity forbids dashing (Full override constraint). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	bool IsDashLocked() const { return bDashLocked; }

	/** Exposure meter: 0 = unnoticed, ExposureThreshold = cover blown (Partial only). */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void AddExposure(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	float GetExposure() const { return Exposure; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	float GetExposureThreshold() const { return ExposureThreshold; }

	UPROPERTY(BlueprintAssignable, Category="Identity")
	FNovaOnIdentityChanged OnIdentityChanged;

	UPROPERTY(BlueprintAssignable, Category="Identity")
	FNovaOnExposureMaxed OnExposureMaxed;

	/** Whether the current override forbids using abilities in the open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	bool bAbilitiesRestricted = false;

	// --- Tuning ---

	/** Ability loadout the player gets under a Full override.
	 *  Constructor seeds Slash -> DA_Ability_TestOverride; values live in the asset. */
	UPROPERTY(EditAnywhere, Category="Identity|Full")
	TMap<FName, TSoftObjectPtr<UNovaAbilityDataAsset>> FullOverrideLoadout;

	/** Full-override identity constraint: this identity cannot Dash. */
	UPROPERTY(EditAnywhere, Category="Identity|Full")
	bool bFullOverrideLocksDash = true;

	UPROPERTY(EditAnywhere, Category="Identity|Partial", meta=(ClampMin="0"))
	float ExposurePerCast = 35.f;

	UPROPERTY(EditAnywhere, Category="Identity|Partial", meta=(ClampMin="1"))
	float ExposureThreshold = 100.f;

	UPROPERTY(EditAnywhere, Category="Identity|Lens", meta=(ClampMin="0.1"))
	float LensDurationSeconds = 3.f;

	UPROPERTY(EditAnywhere, Category="Identity|Lens", meta=(ClampMin="0"))
	float LensBlendSeconds = 0.5f;

protected:

	virtual void BeginPlay() override;

private:

	/** Exposure hookup: every CastStarted while Partial adds ExposurePerCast. */
	UFUNCTION()
	void HandleAbilityEvent(FName AbilityId, FName EventType);

	class UElementAbilityComponent* GetSiblingAbilities() const;
	class UStoryDirectorSubsystem* GetStoryDirector() const;
	class APlayerController* GetOwnerPlayerController() const;

	/** Lens observation point: first dummy target in the level (slice stand-in). */
	AActor* FindLensViewTarget() const;

	void SetOverride(ENovaIdentityOverrideMode Mode, FName IdentityId, bool bRestrictAbilities);

	ENovaIdentityOverrideMode OverrideMode = ENovaIdentityOverrideMode::None;

	FName ActiveIdentityId = NAME_None;

	float Exposure = 0.f;

	bool bDashLocked = false;

	/** Latch so the threshold event fires once per Partial session. */
	bool bExposureMaxedFired = false;

	FTimerHandle LensRevertTimerHandle;
};
