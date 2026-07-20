#include "Core/ParcelPlayerController.h"
#include "ParcelLog.h"
#include "UI/ParcelInGameDeadHUDWidget.h"
#include "UI/ParcelInGameESCMenuWidget.h"
#include "UI/ParcelHUDWidget.h"
#include "Core/ParcelCheatManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

namespace ParcelFrontendMaps
{
	const FString Lobby = TEXT("LV_DF_Lobby_Stage00");
	const FString MainMenuBootstrap = TEXT("Testing_DF_Stage01");
}

DEFINE_LOG_CATEGORY(LogParcelPlayerController);

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

void AParcelPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	Client_NotifyRespawn();
}

void AParcelPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	if (IsLocalController())
	{
		CONTROLLER_LOG(Log, TEXT("[클라이언트 빙의 확정] AcknowledgePossession 감지 - UI 최종 정렬을 실행합니다."));
		Client_NotifyRespawn();
	}
}

void AParcelPlayerController::Client_NotifyDeath_Implementation()
{
	if (!IsLocalController()) return;
	
	CONTROLLER_LOG(All, TEXT("[UI] 로컬 플레이어 사망 감지 - HUD 교체 작업을 시작합니다."));
	
	// Collapse InGameHUD
	if (HUDWidgetInstance && HUDWidgetInstance->IsInViewport())
	{
		HUDWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	// Close ESCMenu
	if (ESCMenuRef && ESCMenuRef->IsInViewport())
	{
		ESCMenuRef->K2_OnMenuCloseStarted();
		ESCMenuRef = nullptr;
	}

	// Dead HUD Widget
	if (DeadHUDWidgetClass && !DeadHUDWidgetInstance)
	{
		DeadHUDWidgetInstance = CreateWidget<UParcelInGameDeadHUDWidget>(this, DeadHUDWidgetClass);
	}

	if (DeadHUDWidgetInstance)
	{
		if (!DeadHUDWidgetInstance->IsInViewport())
		{
			DeadHUDWidgetInstance->AddToViewport(200);
		}

		// Start CountDown, Set Input Focus and Mouse Activate
		DeadHUDWidgetInstance->StartDeathCountdown(5);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(DeadHUDWidgetInstance->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void AParcelPlayerController::Client_NotifyRespawn_Implementation()
{
	if (!IsLocalController()) return;

	CONTROLLER_LOG(All, TEXT("[UI] 로컬 플레이어 리스폰 완료 - 일반 HUD로 복구 및 컴포넌트 재결합을 시작합니다."));
    
	// Collapse DeadHUD
	if (DeadHUDWidgetInstance && DeadHUDWidgetInstance->IsInViewport())
	{
		DeadHUDWidgetInstance->RemoveFromParent();
		DeadHUDWidgetInstance = nullptr;
	}
    
	// ESCMenuRef
	if (ESCMenuRef && ESCMenuRef->IsInViewport())
	{
		ESCMenuRef->K2_OnMenuCloseStarted();
		ESCMenuRef = nullptr;
	}

	// InGameWidget 다시 켜기
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetVisibility(ESlateVisibility::Visible);
		
		// Respawn 후 새로운 컴포넌트들과 동기화 및 델리게이트
		if (UParcelHUDWidget* ParcelHUD = Cast<UParcelHUDWidget>(HUDWidgetInstance))
		{
			ParcelHUD->RequestRebindPlayerEvents();
		}
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void AParcelPlayerController::ToggleInGameMenu()
{
	if (!IsLocalController()) return;

	// ESC 메뉴 닫을 때 애니메이션 재생
	if (ESCMenuRef && ESCMenuRef->IsValidLowLevel() && ESCMenuRef->IsInViewport())
	{
		ESCMenuRef->K2_OnMenuCloseStarted();
		ESCMenuRef = nullptr;

		// [예외] 사망 상태에서 일시정지를 닫았다면, 포커스를 다시 Dead HUD로 복구
		if (DeadHUDWidgetInstance && DeadHUDWidgetInstance->IsInViewport())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(DeadHUDWidgetInstance->TakeWidget());
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
		return;
	}
    
	// 메뉴
	if (ESCMenuClass)
	{
		ESCMenuRef = CreateWidget<UParcelInGameESCMenuWidget>(this, ESCMenuClass);
		if (ESCMenuRef)
		{
			ESCMenuRef->AddToViewport(300);
			ESCMenuRef->SetupMenu();
		}
	}
}