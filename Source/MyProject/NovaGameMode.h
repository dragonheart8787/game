// Project Nova — Game mode (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovaGameMode.generated.h"

/** Vertical slice game mode: wires up the Nova character and controller. */
UCLASS()
class ANovaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ANovaGameMode();

	/** Lazily resolves BP_NovaCharacter so creation order (BP after C++) doesn't matter. */
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};
