// Project Nova — Player controller (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NovaPlayerController.generated.h"

/**
 * Thin player controller. Input mapping lives on the character for the slice;
 * this exists as the hook point for UI/camera-manager work in later patches.
 */
UCLASS()
class ANovaPlayerController : public APlayerController
{
	GENERATED_BODY()
};
