// Project Nova — Player character (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
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

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// --- Input handlers ---

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Dash();
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

private:

	/** World time (seconds) when the next dash becomes available. */
	double NextDashTime = 0.0;
};
