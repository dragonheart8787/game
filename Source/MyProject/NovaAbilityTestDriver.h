// Project Nova — dev-only PIE ability regression driver (never ships, never saved into a level)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaAbilityTestDriver.generated.h"

class UElementAbilityComponent;

/**
 * Dev-only regression driver for ability behavior comparisons across patches.
 * Spawn it into the level in memory (do not save), start PIE, and it fires a
 * fixed, game-time-deterministic cast sequence aimed at the level's dummy:
 *   t=1.0s  Slash + same-frame re-cast (expects CastBlocked_Cooldown)
 *   t=2.5s  Edgewall
 *   t=4.0s  Slash
 * Compare LogNovaAbility / LogNovaDummy / LogNovaWall output between runs.
 */
UCLASS(NotBlueprintable)
class ANovaAbilityTestDriver : public AActor
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

private:

	UElementAbilityComponent* GetPlayerAbilityComp() const;

	/** Point runtime direction from the player pawn at the first dummy target. */
	void AimAtDummy(UElementAbilityComponent& Comp) const;

	void StepSlashTwice();
	void StepEdgewall();
	void StepSlash();

	FTimerHandle Step1Handle;
	FTimerHandle Step2Handle;
	FTimerHandle Step3Handle;
};
