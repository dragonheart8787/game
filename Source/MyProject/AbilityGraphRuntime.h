// Project Nova — Ability Graph runtime (vertical slice)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilityGraphTypes.h"
#include "AbilityGraphRuntime.generated.h"

/**
 * Executes one ability graph definition at runtime.
 * Pipeline: Shape (geometry) -> Path (where it resolves) -> Spawn (persistent
 * actors) -> Affect (effects on detected targets). Constraint/Interact node
 * evaluation comes in a later patch.
 */
UCLASS(BlueprintType)
class UNovaAbilityGraphRuntime : public UObject
{
	GENERATED_BODY()

public:

	/** Bind this runtime to a graph definition. */
	UFUNCTION(BlueprintCallable, Category="Ability")
	void Initialize(const FNovaAbilityGraphDef& InDefinition);

	/** Execute the graph with the given runtime params. Returns false if not initialized. */
	UFUNCTION(BlueprintCallable, Category="Ability")
	bool Execute(AActor* Instigator, const FNovaAbilityRuntimeParams& Params, float DebugDrawSeconds = 1.5f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	const FNovaAbilityGraphDef& GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	bool IsInitialized() const { return bInitialized; }

private:

	/** Shape debug visualization at the cast/effect origin. */
	void DrawShape(UWorld* World, const FVector& CastOrigin, const FVector& EffectOrigin,
		const FVector& Direction, float Range, float HalfArcRad, float Radius, float DebugDrawSeconds) const;

	/** Spawn node: leave persistent actors at the effect origin. */
	void ExecuteSpawn(UWorld* World, const FVector& EffectOrigin, const FVector& Direction) const;

	/** Affect node: detect targets inside the shape and apply the effect. */
	void ExecuteAffect(UWorld* World, AActor* Instigator, const FVector& EffectOrigin,
		const FVector& Direction, float Range, float HalfArcRad, float DebugDrawSeconds) const;

	FNovaAbilityGraphDef Definition;

	bool bInitialized = false;
};
