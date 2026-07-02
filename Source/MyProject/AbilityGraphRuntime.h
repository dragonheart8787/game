// Project Nova — Ability Graph runtime (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilityGraphTypes.h"
#include "AbilityGraphRuntime.generated.h"

/**
 * Executes one ability graph definition at runtime.
 * Vertical slice: walks the node list and logs; real node evaluation comes later.
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
	bool Execute(AActor* Instigator, const FNovaAbilityRuntimeParams& Params);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	const FNovaAbilityGraphDef& GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ability")
	bool IsInitialized() const { return bInitialized; }

private:

	FNovaAbilityGraphDef Definition;

	bool bInitialized = false;
};
