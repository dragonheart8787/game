// Project Nova — Identity Override component (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IdentityOverrideComponent.generated.h"

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

/**
 * Identity Overwrite state on the player.
 * Vertical slice: tracks mode/identity, ability restriction and an exposure meter stub.
 */
UCLASS(ClassGroup=(Nova), meta=(BlueprintSpawnableComponent))
class UIdentityOverrideComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Player literally becomes IdentityId (abilities/gear/relations swap — stubbed). */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyFullOverride(FName IdentityId);

	/** Player stays themselves but abilities are restricted and exposure builds. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyPartialOverride(FName IdentityId);

	/** Player borrows a viewpoint only (CCTV, bystander, memory, drone...). */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void ApplyLensOverride(FName IdentityId);

	/** Return to the player's own identity. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void RevertOverride();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	ENovaIdentityOverrideMode GetOverrideMode() const { return OverrideMode; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	FName GetActiveIdentityId() const { return ActiveIdentityId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	bool AreAbilitiesRestricted() const { return bAbilitiesRestricted; }

	/** Exposure meter stub: 0 = unnoticed, 1 = blown cover. Only meaningful in Partial mode. */
	UFUNCTION(BlueprintCallable, Category="Identity")
	void AddExposure(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Identity")
	float GetExposure() const { return Exposure; }

	UPROPERTY(BlueprintAssignable, Category="Identity")
	FNovaOnIdentityChanged OnIdentityChanged;

	/** Whether the current override forbids using abilities in the open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	bool bAbilitiesRestricted = false;

private:

	void SetOverride(ENovaIdentityOverrideMode Mode, FName IdentityId, bool bRestrictAbilities);

	ENovaIdentityOverrideMode OverrideMode = ENovaIdentityOverrideMode::None;

	FName ActiveIdentityId = NAME_None;

	float Exposure = 0.f;
};
