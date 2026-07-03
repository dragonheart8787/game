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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dummy")
	float MaxHealth = 100.f;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dummy")
	float Health = 100.f;
};
