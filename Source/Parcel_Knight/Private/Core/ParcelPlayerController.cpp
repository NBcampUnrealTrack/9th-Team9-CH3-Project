#include "Core/ParcelPlayerController.h"
#include "Core/ParcelCheatManager.h"

AParcelPlayerController::AParcelPlayerController()
{
	CheatClass = UParcelCheatManager::StaticClass();
}

void AParcelPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
}

void AParcelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}
