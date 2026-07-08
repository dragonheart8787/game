// Project Nova — Dummy training target (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaDummyTarget.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogNovaDummy, Log, All);

/**
 * Static training dummy. Takes damage from ability casts, reports remaining
 * health on screen and in the log, hides itself when health reaches zero.
 */
UCLASS()
class ANovaDummyTarget : public AActor
{
	GENERATED_BODY()

public:

	ANovaDummyTarget();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dummy")
	float GetHealth() const { return Health; }

	// --- Move-speed / slow (Affect: Slow) ---
	// The dummy is static (no CharacterMovement), so BaseMoveSpeed is a nominal
	// stat that a Bind's Slow scales down and auto-restores. Queryable so the
	// regression driver can assert the reduce-then-restore behavior.

	/** Apply a slow: keep SpeedMultiplier of base speed for DurationSeconds, then restore. */
	UFUNCTION(BlueprintCallable, Category="Dummy")
	void ApplySlow(float SpeedMultiplier, float DurationSeconds);

	/** Current effective move speed (BaseMoveSpeed * active multiplier). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dummy")
	float GetCurrentMoveSpeed() const { return BaseMoveSpeed * CurrentSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dummy")
	float GetBaseMoveSpeed() const { return BaseMoveSpeed; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dummy")
	bool IsSlowed() const { return CurrentSpeedMultiplier < 1.f; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dummy")
	float MaxHealth = 100.f;

	/** Nominal unslowed move speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dummy")
	float BaseMoveSpeed = 300.f;

protected:

	virtual void BeginPlay() override;

	/** Slow timer callback: return to full speed. */
	void RestoreSpeed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dummy")
	float Health = 100.f;

	/** 1.0 = full speed; a Slow drops this and RestoreSpeed puts it back. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dummy")
	float CurrentSpeedMultiplier = 1.f;

	FTimerHandle SlowTimerHandle;
};
