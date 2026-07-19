#include "Core/ParcelPlayerController.h"
#include "Core/ParcelCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

namespace ParcelFrontendMaps
{
	const FString Lobby = TEXT("LV_DF_Lobby_Stage00");
	const FString MainMenuBootstrap = TEXT("Testing_DF_Stage01");
}

AParcelPlayerController::AParcelPlayerController()
{
	CheatClass = UParcelCheatManager::StaticClass();
}

void AParcelPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
		const bool bUsesLevelBlueprintUI =
			CurrentLevelName == ParcelFrontendMaps::Lobby ||
			CurrentLevelName == ParcelFrontendMaps::MainMenuBootstrap;

		// Both front-end maps create their own UI and input mode in their Level
		// Blueprint. Avoid adding WBP_InGameHUD or overriding that input state.
		if (bUsesLevelBlueprintUI)
		{
			return;
		}

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		
		// HUD
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

void AParcelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}
