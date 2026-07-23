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
#include "Core/CustomizationComponent.h"
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
       InitHUDForCurrentLevel();
    }
}

// [핵심] 심리스 트래블(Seamless Travel) 완료 후 컨트롤러가 새 레벨에 안착할 때 호출됨
void AParcelPlayerController::ClientRestart_Implementation(APawn* NewPawn)
{
    Super::ClientRestart_Implementation(NewPawn);

    if (IsLocalController())
    {
       CONTROLLER_LOG(Log, TEXT("[ClientRestart] 심리스 트래블 완료 감지. 레벨 맞춤형 HUD 재구성 실행."));
       InitHUDForCurrentLevel();
    }
}

void AParcelPlayerController::InitHUDForCurrentLevel()
{
    if (!IsLocalController()) return;

    const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    CONTROLLER_LOG(Log, TEXT("[InitHUD] 현재 레벨: %s 기반 HUD 초기화 시퀀스 개시"), *CurrentLevelName);

    // 1. 메인 메뉴 레벨인 경우
    if (CurrentLevelName == ParcelFrontendMaps::GetMainMenuLevelName())
    {
        ResetAllHUDInstances();
        FInputModeUIOnly InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = true;
        return;
    }

    // 2. 로비 레벨인 경우 (Lobby -> InGame 또는 InGame -> Lobby 돌아왔을 때)
    if (CurrentLevelName == ParcelFrontendMaps::Lobby)
    {
        ResetAllHUDInstances();

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
                
                UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] C++ 컨트롤러가 레벨 전환 후 로비 HUD 안착 완료!"));
            }
        }
        return;
    }

    // 3. 인게임 스테이지 레벨인 경우 (LV_DF_Stage01, Stage02 등)
    ResetAllHUDInstances();

    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    bShowMouseCursor = false;
    
    if (HUDWidgetClass)
    {
        HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
        if (HUDWidgetInstance)
        {
            HUDWidgetInstance->AddToViewport();
            
            if (UParcelHUDWidget* ParcelHUD = Cast<UParcelHUDWidget>(HUDWidgetInstance))
            {
                ParcelHUD->TryBindUIEvents();
            }
            CONTROLLER_LOG(Log, TEXT("[InGame HUD] 인게임 HUD 위젯 뷰포트 생성 및 이벤트 결합 완료."));
        }
    }

    CreateTrapStatusOverlayIfNeeded();
}

void AParcelPlayerController::ResetAllHUDInstances()
{
    if (HUDWidgetInstance)
    {
       HUDWidgetInstance->RemoveFromParent();
       HUDWidgetInstance = nullptr;
    }

    if (LobbyHUDWidgetInstance)
    {
       LobbyHUDWidgetInstance->RemoveFromParent();
       LobbyHUDWidgetInstance = nullptr;
    }

    if (DeadHUDWidgetInstance)
    {
       DeadHUDWidgetInstance->RemoveFromParent();
       DeadHUDWidgetInstance = nullptr;
    }

    if (ESCMenuRef)
    {
       ESCMenuRef->RemoveFromParent();
       ESCMenuRef = nullptr;
    }

    if (TrapStatusOverlayWidgetInstance)
    {
       TrapStatusOverlayWidgetInstance->RemoveFromParent();
       TrapStatusOverlayWidgetInstance = nullptr;
    }

    LastRespawnNotifiedPawn = nullptr;
}

void AParcelPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetAllHUDInstances();
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
    SubmitLocalCustomizationToServer();
}

void AParcelPlayerController::AcknowledgePossession(APawn* InPawn)
{
    Super::AcknowledgePossession(InPawn);
    SubmitLocalLoadoutToServer();
    SubmitLocalCustomizationToServer();

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

void AParcelPlayerController::SubmitLocalCustomizationToServer()
{
    if (!IsLocalController())
    {
       return;
    }

    if (const UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
    {
       Server_SubmitCustomization(GI->GetEquippedSkin(), GI->GetEquippedTitle(), GI->GetEquippedEffect());
    }
}

void AParcelPlayerController::Server_SubmitCustomization_Implementation(
    FGameplayTag SkinTag, FGameplayTag TitleTag, FGameplayTag EffectTag)
{
    AParcelPlayerState* ParcelPlayerState = GetPlayerState<AParcelPlayerState>();
    UCustomizationComponent* Customization = ParcelPlayerState
       ? ParcelPlayerState->GetCustomizationComponent()
       : nullptr;

    if (!Customization)
    {
       CONTROLLER_LOG(Warning, TEXT("[Customization] Server rejected submission: CustomizationComponent unavailable."));
       return;
    }

    Customization->EquipSkin(SkinTag);
    Customization->EquipTitle(TitleTag);
    Customization->EquipEffect(EffectTag);

    CONTROLLER_LOG(
       Log,
       TEXT("[Customization] Server accepted submission: Skin=%s, Title=%s"),
       *SkinTag.ToString(),
       *TitleTag.ToString());
}

// 🆕 복구된 사망 처리 Client RPC 구현체
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
    
    if (DeadHUDWidgetInstance && DeadHUDWidgetInstance->IsInViewport())
    {
       DeadHUDWidgetInstance->RemoveFromParent();
       DeadHUDWidgetInstance = nullptr;
    }
    
    if (ESCMenuRef && ESCMenuRef->IsInViewport())
    {
       ESCMenuRef->K2_OnMenuCloseStarted();
       ESCMenuRef = nullptr;
    }

    // 만약 심리스 트래블 직후라 HUDWidgetInstance가 아직 스폰 안 되었다면 재생성
    if (!HUDWidgetInstance)
    {
       InitHUDForCurrentLevel();
    }
    else
    {
       HUDWidgetInstance->SetVisibility(ESlateVisibility::Visible);
        
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
    if (!IsLocalController() || Duration <= 0.0f)
    {
       return;
    }

    CreateTrapStatusOverlayIfNeeded();
    if (IsValid(TrapStatusOverlayWidgetInstance))
    {
       TrapStatusOverlayWidgetInstance->ShowTrapStatus(Color, Duration);
    }
}

void AParcelPlayerController::CreateTrapStatusOverlayIfNeeded()
{
    const bool bIsLocalController = IsLocalController();
    if (!bIsLocalController) return;

    if (IsValid(TrapStatusOverlayWidgetInstance))
    {
       if (!TrapStatusOverlayWidgetInstance->IsInViewport())
       {
          TrapStatusOverlayWidgetInstance->AddToViewport(100);
       }
       return;
    }

    if (!TrapStatusOverlayWidgetClass) return;

    TrapStatusOverlayWidgetInstance = CreateWidget<UParcelTrapStatusOverlayWidget>(this, TrapStatusOverlayWidgetClass);
    if (TrapStatusOverlayWidgetInstance)
    {
       TrapStatusOverlayWidgetInstance->AddToViewport(100);
    }
}

void AParcelPlayerController::ToggleInGameMenu()
{
    if (!IsLocalController()) return;

    if (ESCMenuRef && ESCMenuRef->IsValidLowLevel() && ESCMenuRef->IsInViewport())
    {
       ESCMenuRef->K2_OnMenuCloseStarted();
       ESCMenuRef = nullptr;

       if (DeadHUDWidgetInstance && DeadHUDWidgetInstance->IsInViewport())
       {
          FInputModeUIOnly InputMode;
          InputMode.SetWidgetToFocus(DeadHUDWidgetInstance->TakeWidget());
          SetInputMode(InputMode);
          bShowMouseCursor = true;
       }
       return;
    }
    
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
}

bool AParcelPlayerController::Server_RequestChangeLobbyMap_Validate(int32 NewMapIndex)
{
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