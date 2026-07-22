#include "Core/ParcelPlayerController.h"
#include "ParcelLog.h"
#include "Parcel_Knight/Public/UI/ParcelInGameDeadHUDWidget.h"
#include "UI/ParcelInGameESCMenuWidget.h"
#include "UI/ParcelHUDWidget.h"
#include "UI/ParcelTrapStatusOverlayWidget.h"
#include "Core/ParcelCheatManager.h"
#include "Core/HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "UI/ParcelLobbyHUDWidget.h"
#include "Core/ParcelGameMode.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelGameInstance.h"
#include "Core/ParcelPlayerState.h"
#include "Core/InventoryComponent.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

namespace ParcelFrontendMaps
{
	const FString Lobby = TEXT("LV_DF_Lobby_Stage00");

	FString GetMainMenuLevelName()
	{
		return FPackageName::GetShortName(
			FPackageName::ObjectPathToPackageName(UGameMapsSettings::GetGameDefaultMap()));
	}
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
		
		if (CurrentLevelName == ParcelFrontendMaps::Lobby)
		{
			if (LobbyHUDWidgetClass)
			{
				LobbyHUDWidgetInstance = CreateWidget<UParcelLobbyHUDWidget>(this, LobbyHUDWidgetClass);
				if (LobbyHUDWidgetInstance)
				{
					LobbyHUDWidgetInstance->AddToViewport();
					
					FInputModeGameAndUI InputMode;
					InputMode.SetWidgetToFocus(LobbyHUDWidgetInstance->TakeWidget());
					SetInputMode(InputMode);
					bShowMouseCursor = true;
                
					UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] C++ 컨트롤러가 타이밍 렉 없이 %s 화면에 로비 HUD 최종 안착 완료!"), 
						HasAuthority() ? TEXT("호스트") : TEXT("클라이언트"));
				}
			}
			return;
		}

		if (CurrentLevelName == ParcelFrontendMaps::GetMainMenuLevelName())
		{
			return;
		}
		
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
       
		if (HUDWidgetClass)
		{
			HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (HUDWidgetInstance)
			{
				HUDWidgetInstance->AddToViewport();
			}
		}

		CreateTrapStatusOverlayIfNeeded();
	}
}

void AParcelPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TrapStatusOverlayWidgetInstance)
	{
		TrapStatusOverlayWidgetInstance->RemoveFromParent();
		TrapStatusOverlayWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AParcelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AParcelPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SubmitLocalLoadoutToServer();

	// ClientRestart가 owning client의 AcknowledgePossession을 호출하며,
	// 새 Pawn 승인 뒤 해당 경로에서만 UI를 한 번 복구한다.
}

void AParcelPlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	SubmitLocalLoadoutToServer();

	if (IsLocalController() && InPawn && LastRespawnNotifiedPawn.Get() != InPawn)
	{
		const UHealthComponent* HealthComp = InPawn->FindComponentByClass<UHealthComponent>();
		if (!HealthComp || !HealthComp->IsDead())
		{
			LastRespawnNotifiedPawn = InPawn;
			CONTROLLER_LOG(Log, TEXT("[클라이언트 빙의 확정] AcknowledgePossession 감지 - UI 최종 정렬을 실행합니다."));
			Client_NotifyRespawn();
		}
	}
}

void AParcelPlayerController::SubmitLocalLoadoutToServer()
{
	if (!IsLocalController())
	{
		return;
	}

	if (const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
	{
		Server_SubmitLoadout(GI->GetLoadout());
	}
}

void AParcelPlayerController::Server_SubmitLoadout_Implementation(
	const TArray<FGameplayTag>& RequestedItems)
{
	AParcelPlayerState* ParcelPlayerState = GetPlayerState<AParcelPlayerState>();
	UInventoryComponent* Inventory = ParcelPlayerState
		? ParcelPlayerState->GetInventoryComponent()
		: nullptr;

	if (!Inventory)
	{
		CONTROLLER_LOG(Warning, TEXT("[Loadout] Server rejected submission: InventoryComponent unavailable."));
		return;
	}

	if (!Inventory->SetValidatedLoadout(RequestedItems, 3))
	{
		CONTROLLER_LOG(
			Warning,
			TEXT("[Loadout] Server rejected %d submitted item(s)."),
			RequestedItems.Num());
		return;
	}

	CONTROLLER_LOG(Log, TEXT("[Loadout] Server accepted %d submitted item(s)."), RequestedItems.Num());
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

void AParcelPlayerController::Client_PlayTrapActivationSound_Implementation(
	USoundBase* ActivationSound,
	float VolumeMultiplier,
	float PitchMultiplier
)
{
	if (!IsLocalController() || !ActivationSound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		this,
		ActivationSound,
		FMath::Max(0.0f, VolumeMultiplier),
		FMath::Max(0.01f, PitchMultiplier)
	);
}

void AParcelPlayerController::Client_ShowTrapStatus_Implementation(FLinearColor Color, float Duration)
{
	UE_LOG(
		LogParcelPlayerController,
		Warning,
		TEXT("[Trap UI] Client RPC received: PC=%s IsLocalController=%d OverlayValid=%d Color RGBA=(%.3f, %.3f, %.3f, %.3f) Duration=%.3f"),
		*GetNameSafe(this),
		IsLocalController(),
		IsValid(TrapStatusOverlayWidgetInstance),
		Color.R,
		Color.G,
		Color.B,
		Color.A,
		Duration
	);

	if (!IsLocalController() || Duration <= 0.0f)
	{
		UE_LOG(
			LogParcelPlayerController,
			Warning,
			TEXT("[Trap UI] Client RPC ignored: PC=%s IsLocalController=%d Duration=%.3f"),
			*GetNameSafe(this),
			IsLocalController(),
			Duration
		);
		return;
	}

	CreateTrapStatusOverlayIfNeeded();
	if (IsValid(TrapStatusOverlayWidgetInstance))
	{
		TrapStatusOverlayWidgetInstance->ShowTrapStatus(Color, Duration);
	}
	else
	{
		UE_LOG(
			LogParcelPlayerController,
			Error,
			TEXT("[Trap UI] Client RPC could not show status: overlay widget reference is invalid. PC=%s Class=%s"),
			*GetNameSafe(this),
			*GetNameSafe(TrapStatusOverlayWidgetClass)
		);
	}
}

void AParcelPlayerController::CreateTrapStatusOverlayIfNeeded()
{
	const bool bIsLocalController = IsLocalController();
	if (!bIsLocalController)
	{
		UE_LOG(
			LogParcelPlayerController,
			Warning,
			TEXT("[Trap UI] Overlay widget creation skipped: PC=%s IsLocalController=0"),
			*GetNameSafe(this)
		);
		return;
	}

	if (IsValid(TrapStatusOverlayWidgetInstance))
	{
		if (!TrapStatusOverlayWidgetInstance->IsInViewport())
		{
			TrapStatusOverlayWidgetInstance->AddToViewport(100);
			UE_LOG(
				LogParcelPlayerController,
				Warning,
				TEXT("[Trap UI] Existing overlay widget restored to viewport: PC=%s Instance=%s"),
				*GetNameSafe(this),
				*GetNameSafe(TrapStatusOverlayWidgetInstance)
			);
		}
		return;
	}

	if (!TrapStatusOverlayWidgetClass)
	{
		UE_LOG(
			LogParcelPlayerController,
			Error,
			TEXT("[Trap UI] Overlay widget creation failed: TrapStatusOverlayWidgetClass is None. PC=%s IsLocalController=%d HUDValid=%d"),
			*GetNameSafe(this),
			bIsLocalController,
			IsValid(HUDWidgetInstance)
		);
		return;
	}

	TrapStatusOverlayWidgetInstance = CreateWidget<UParcelTrapStatusOverlayWidget>(this, TrapStatusOverlayWidgetClass);
	if (TrapStatusOverlayWidgetInstance)
	{
		TrapStatusOverlayWidgetInstance->AddToViewport(100);
		UE_LOG(
			LogParcelPlayerController,
			Warning,
			TEXT("[Trap UI] Overlay widget created: PC=%s IsLocalController=%d HUDValid=%d Class=%s Instance=%s IsInViewport=%d"),
			*GetNameSafe(this),
			bIsLocalController,
			IsValid(HUDWidgetInstance),
			*GetNameSafe(TrapStatusOverlayWidgetClass),
			*GetNameSafe(TrapStatusOverlayWidgetInstance),
			TrapStatusOverlayWidgetInstance->IsInViewport()
		);
	}
	else
	{
		UE_LOG(
			LogParcelPlayerController,
			Error,
			TEXT("[Trap UI] Overlay widget creation failed: CreateWidget returned null. PC=%s IsLocalController=%d Class=%s"),
			*GetNameSafe(this),
			bIsLocalController,
			*GetNameSafe(TrapStatusOverlayWidgetClass)
		);
	}
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

bool AParcelPlayerController::Server_SendLobbyChatMessage_Validate(const FText& ChatText)
{
	return !ChatText.IsEmpty() && ChatText.ToString().Len() < 200;
}

void AParcelPlayerController::Server_SendLobbyChatMessage_Implementation(const FText& ChatText)
{
	if (!GetWorld()) return;
	
	FString SenderNickname = PlayerState ? PlayerState->GetPlayerName() : TEXT("알 수 없는 참가자");
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AParcelPlayerController* TargetPC = Cast<AParcelPlayerController>(It->Get());
		if (TargetPC)
		{
			TargetPC->Client_ReceiveLobbyChatMessage(SenderNickname, ChatText);
		}
	}
}

void AParcelPlayerController::Client_ReceiveLobbyChatMessage_Implementation(const FString& SenderName, const FText& ChatText)
{
	if (LobbyHUDWidgetInstance && LobbyHUDWidgetInstance->IsValidLowLevel())
	{
		LobbyHUDWidgetInstance->AddChatLog(SenderName, ChatText);
	}
    
	UE_LOG(LogTemp, Log, TEXT("[Lobby Chat RPC] %s 님의 메시지 수신 완료: %s"), *SenderName, *ChatText.ToString());
}

bool AParcelPlayerController::Server_RequestChangeLobbyMap_Validate(int32 NewMapIndex)
{
	// Authorization failures are ordinary request rejections, not malformed RPCs.
	// Returning false here would route the connection through RPC_ValidateFailed.
	return true;
}

void AParcelPlayerController::Server_RequestChangeLobbyMap_Implementation(int32 NewMapIndex)
{
	AParcelGameMode* ParcelGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AParcelGameMode>() : nullptr;
	if (!ParcelGameMode || !ParcelGameMode->RequestLobbyMapSelection(this, NewMapIndex))
	{
		CONTROLLER_LOG(Warning, TEXT("Lobby map request rejected for index %d."), NewMapIndex);
	}
}

void AParcelPlayerController::Server_RequestStartLobbyGame_Implementation()
{
	AParcelGameMode* ParcelGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AParcelGameMode>() : nullptr;
	if (!ParcelGameMode || !ParcelGameMode->RequestStartLobbyGame(this))
	{
		CONTROLLER_LOG(Warning, TEXT("Lobby start request rejected."));
	}
}
