#include "Core/ParcelPlayerController.h"
#include "Core/ParcelCheatManager.h"
#include "Blueprint/UserWidget.h"

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
		
		// HUD
		if (IsLocalController())
		{
			if (HUDWidgetClass)
			{
				HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
				
				if (HUDWidgetInstance)
				{
					HUDWidgetInstance->AddToViewport();
				}
			}
		}
	}
}

void AParcelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}
