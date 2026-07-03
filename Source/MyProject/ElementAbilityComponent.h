// Project Nova — Element ability component (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityGraphTypes.h"
#include "ElementAbilityComponent.generated.h"

class UNovaAbilityGraphRuntime;
class UNiagaraSystem;

DECLARE_LOG_CATEGORY_EXTERN(LogNovaAbility, Log, All);

/** EventType examples: "CastStarted", "CastBlocked", "CooldownReady". */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNovaOnAbilityEvent, FName, AbilityId, FName, EventType);

/**
 * Ability Graph runtime entry point on the character.
 * Owns registered ability definitions, runtime params, cooldown and energy cost.
 */
UCLASS(ClassGroup=(Nova), meta=(BlueprintSpawnableComponent))
class UElementAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UElementAbilityComponent();

	/** Register (or replace) an ability definition at runtime. */
	UFUNCTION(BlueprintCallable, Category="Ability")
	void RegisterAbility(const FNovaAbilityGraphDef& Definition);

	/** Cast a registered ability. Handles cooldown, cost and restriction checks. */
	UFUNCTION(BlueprintCallable, Category="Ability")
	bool CastAbilityById(FName AbilityId);

	// --- Runtime shaping ---

	UFUNCTION(BlueprintCallable, Category="Ability|Runtime")
	void SetRuntimeDirection(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category="Ability|Runtime")
	void SetRuntimeWidth(float Width);

	UFUNCTION(BlueprintCallable, Category="Ability|Runtime")
	void SetRuntimeArc(float Arc);

	UFUNCTION(BlueprintCallable, Category="Ability|Runtime")
	void SetRuntimeRange(float Range);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability|Runtime")
	const FNovaAbilityRuntimeParams& GetRuntimeParams() const { return RuntimeParams; }

	// --- State queries (debug overlay reads these) ---

	/** Remaining cooldown in seconds for an ability; 0 when ready. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	float GetRemainingCooldown(FName AbilityId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	float GetEnergy() const { return Energy; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	TArray<FName> GetRegisteredAbilityIds() const;

	/** Human-readable text of the last ability event, for the debug overlay. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	const FString& GetLastAbilityEventText() const { return LastAbilityEventText; }

	UPROPERTY(BlueprintAssignable, Category="Ability")
	FNovaOnAbilityEvent OnAbilityEvent;

	// --- Tuning ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|Energy")
	float MaxEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|Energy")
	float EnergyRegenPerSecond = 10.f;

	/** VFX hookup point (stub — assigned in Blueprint later, not spawned yet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|VFX")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** Lifetime of the placeholder debug-draw shapes standing in for real VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability|VFX")
	float DebugDrawSeconds = 1.5f;

protected:

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Debug-draw the cast shape and apply damage to targets in the range/arc cone. */
	void ApplyAbilityEffects(const FNovaAbilityGraphDef& Definition);

	void BroadcastAbilityEvent(FName AbilityId, FName EventType, const FString& Detail);

	/** Registered graph definitions by ability id. */
	TMap<FName, FNovaAbilityGraphDef> Abilities;

	/** Per-ability runtime executors. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UNovaAbilityGraphRuntime>> Runtimes;

	/** World-time (seconds) at which each ability's cooldown ends. */
	TMap<FName, double> CooldownEndTimes;

	FNovaAbilityRuntimeParams RuntimeParams;

	float Energy = 100.f;

	FString LastAbilityEventText;
};
