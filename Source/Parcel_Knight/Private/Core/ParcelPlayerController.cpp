#include "Core/ParcelPlayerController.h"

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
