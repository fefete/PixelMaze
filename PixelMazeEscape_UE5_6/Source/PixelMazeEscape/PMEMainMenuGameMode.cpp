#include "PMEMainMenuGameMode.h"

#include "PMEMainMenuPlayerController.h"

APMEMainMenuGameMode::APMEMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = APMEMainMenuPlayerController::StaticClass();
}
