// Project Nova — Ability fusion (Patch 7)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityGraphTypes.h"
#include "NovaAbilityFusion.generated.h"

/**
 * Fuses two ability graph definitions into a third (Slash x Bind -> RendLock).
 * The fused def is DERIVED, never authored: every field comes from the source
 * defs through the deterministic rules below, so retuning a component ability
 * (C++ default or DataAsset) retunes every fusion built on it for free.
 *
 * Rules (Primary = the ability that defines the delivery):
 *   Shape / Path / DefaultParams  -> Primary's (runtime-param inheritance intact)
 *   Spawn                         -> Primary's unless None, else Secondary's
 *   Affect chain                  -> Primary's chain + Secondary's chain appended
 *   Constraint                    -> Primary's unless None, else Secondary's
 *   EnergyCost                    -> Primary + Secondary (a fusion costs both parts)
 *   CooldownSeconds               -> max(Primary, Secondary) (as heavy as its heaviest part)
 */
UCLASS()
class UNovaAbilityFusionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Ability|Fusion")
	static FNovaAbilityGraphDef FuseAbilityGraphDefs(const FNovaAbilityGraphDef& Primary,
		const FNovaAbilityGraphDef& Secondary, FName FusedAbilityId, FText FusedDisplayName);
};
