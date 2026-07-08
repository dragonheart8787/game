// Project Nova — Player character (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Character.h"
#include "NovaPlayerCharacter.generated.h"

class UCameraComponent;
class UElementAbilityComponent;
class UIdentityOverrideComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogNovaCharacter, Log, All);

/**
 * Lyn — third-person player character.
 * Move/Look/Jump/Dash, two ability casts, two story triggers and a debug toggle.
 */
UCLASS()
class ANovaPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ANovaPlayerCharacter();

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UElementAbilityComponent* GetElementAbilityComp() const { return ElementAbilityComp; }
	FORCEINLINE UIdentityOverrideComponent* GetIdentityOverrideComp() const { return IdentityOverrideComp; }

	/** Dash input handler. Public so dev test drivers can exercise the
	 *  control-mask and identity dash-lock gates directly. */
	void Dash();

	/** Cast Bind (Patch 6) from the PIE console: press ` and type NovaCastBind.
	 *  Kept off the CastAbility1/2 slots — no new IMC key needed. */
	UFUNCTION(Exec)
	void NovaCastBind();

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** True while the Director's control mask is Hold (player control locked). */
	bool IsControlHeld() const;

	/** Adds DefaultMappingContext to the local player's Enhanced Input subsystem.
	 *  Called from both NotifyControllerChanged and SetupPlayerInputComponent so
	 *  whichever runs with a valid LocalPlayer wins; logs every skip reason.
	 *  Caller tag makes the log sequence unambiguous. */
	void TryAddDefaultMappingContext(const TCHAR* Caller);

	// --- Input handlers ---

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpStart();
	void JumpStop();
	void CastAbility1();
	void CastAbility2();
	void TriggerStoryA();
	void TriggerStoryB();
	void ToggleDebug();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UElementAbilityComponent> ElementAbilityComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UIdentityOverrideComponent> IdentityOverrideComp;

	// --- Input assets (assigned in the character Blueprint) ---

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_Dash;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_CastAbility1;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_CastAbility2;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_TriggerStoryA;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_TriggerStoryB;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> IA_ToggleDebug;

	// --- Dash tuning ---

	/** How far a dash carries the character (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dash")
	float DashDistance = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dash")
	float DashCooldown = 1.5f;

	/** How long the dash burst lasts before normal physics take back over (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dash", meta=(ClampMin="0.05", UIMin="0.05"))
	float DashDurationSeconds = 0.2f;

private:

	/** Ends the dash burst: caps lateral speed back to MaxWalkSpeed, leaving Z untouched. */
	void EndDash();

	/** World time (seconds) when the next dash becomes available. */
	double NextDashTime = 0.0;

	FTimerHandle DashEndTimerHandle;
};
