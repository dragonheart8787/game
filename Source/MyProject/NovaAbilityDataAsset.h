// Project Nova — ability graph definition as an editable Content asset (Patch 4)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilityGraphTypes.h"
#include "NovaAbilityDataAsset.generated.h"

/**
 * One composed ability graph definition (Shape/Path/Spawn/Affect nodes plus
 * AbilityId, CooldownSeconds, EnergyCost) stored as a .uasset so tuning
 * (damage, travel distance, wall extents, lifetimes, cooldowns...) is an
 * editor edit + save, not a recompile. UElementAbilityComponent maps
 * AbilityId -> asset and registers these definitions at runtime.
 */
UCLASS(BlueprintType)
class UNovaAbilityDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	FNovaAbilityGraphDef Definition;
};
