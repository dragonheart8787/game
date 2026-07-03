// Project Nova — Game mode (vertical slice stub)

#include "NovaGameMode.h"

#include "NovaPlayerCharacter.h"
#include "NovaPlayerController.h"

ANovaGameMode::ANovaGameMode()
{
	DefaultPawnClass = ANovaPlayerCharacter::StaticClass();
	PlayerControllerClass = ANovaPlayerController::StaticClass();
}

UClass* ANovaGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// Prefer the Blueprint subclass (mesh + input assets live there); fall back
	// to the raw C++ character if the BP is missing. StaticLoadClass caches, so
	// this costs a map lookup after the first spawn.
	if (UClass* NovaCharacterBP = StaticLoadClass(APawn::StaticClass(), nullptr,
		TEXT("/Game/Blueprints/Player/BP_NovaCharacter.BP_NovaCharacter_C")))
	{
		return NovaCharacterBP;
	}
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}
