// Project Nova — Game mode (vertical slice stub)

#include "NovaGameMode.h"

#include "NovaPlayerCharacter.h"
#include "NovaPlayerController.h"

ANovaGameMode::ANovaGameMode()
{
	// Blueprint subclasses (BP_NovaCharacter etc.) will override these once created.
	DefaultPawnClass = ANovaPlayerCharacter::StaticClass();
	PlayerControllerClass = ANovaPlayerController::StaticClass();
}
