#include "UI/ParcelLobbyHUDWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ParcelLobbyPlayerSlotWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelGameState.h"
#include "Engine/Texture2D.h"
#include "UI/ParcelFriendListWidget.h"
#include "Core/SessionSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/ParcelMapSelectWidget.h"
#include "Character/ParcelStaminaComponent.h"
#include "Core/HealthComponent.h"
#include "Character/ParcelStaminaComponent.h"
#include "Core/HealthComponent.h" 
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelInteractionComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Core/InventoryComponent.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UParcelLobbyHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (Canvas_MenuContainer)
    {
        Canvas_MenuContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (Txt_EscPrompt)
    {
        Txt_EscPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    bIsMenuOpen = false;
    bStatDelegatesBound = false;
    
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }
    
    SetupLobbyLayout();
    
    if (Btn_Options)  Btn_Options->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOptionsClicked);
    if (Btn_Friends)  Btn_Friends->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleFriendsClicked);
    if (Btn_Leave)     Btn_Leave->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleLeaveLobbyClicked);
    if (Btn_SelectMap) Btn_SelectMap->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleSelectMapClicked);
    if (Btn_Action)    Btn_Action->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleActionOrStartClicked);

    if (EditableText_ChatInput)
    {
        EditableText_ChatInput->OnTextCommitted.AddDynamic(this, &UParcelLobbyHUDWidget::HandleChatTextCommitted);
        EditableText_ChatInput->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(GetOwningPlayer()))
    {
        ParcelPC->LobbyHUDWidgetInstance = this;
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 플레이어 컨트롤러 상에 HUD 인스턴스 주소 등록 완료."));
    }
    
    if (GetWorld())
    {
        if (AParcelGameState* ParcelGS = GetWorld()->GetGameState<AParcelGameState>())
        {
            ParcelGS->OnLobbyMapChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOnLobbyMapChanged);
            HandleOnLobbyMapChanged(ParcelGS->GetSelectedMapIndex());
        }
        
        RefreshLobbyPlayers();
        
        GetWorld()->GetTimerManager().SetTimer(
            LobbyRefreshTimerHandle, 
            this, 
            &UParcelLobbyHUDWidget::RefreshLobbyPlayers, 
            1.0f, 
            true
        );
    }
    
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
        {
            SessionSubsystem->OnSessionDestroyComplete.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOnSessionDestroyComplete);
        }
    }
    SetIsFocusable(true);
}

void UParcelLobbyHUDWidget::NativeDestruct()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
    }
    
    Super::NativeDestruct();
}

FReply UParcelLobbyHUDWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    FKey PressedKey = InKeyEvent.GetKey();
    if (bIsMenuOpen && (PressedKey == EKeys::P || PressedKey == EKeys::Tab))
    {
        ToggleLobbyMenuExternal();
        return FReply::Handled();
    }

    if (PressedKey == EKeys::Enter)
    {
        if (!bIsMenuOpen && EditableText_ChatInput)
        {
            bool bIsChattingNow = (EditableText_ChatInput->GetVisibility() == ESlateVisibility::Visible);
            SetChatInputInputMode(!bIsChattingNow);
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}

void UParcelLobbyHUDWidget::SetupLobbyLayout()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && PC->HasAuthority())
    {
        if (Btn_SelectMap) Btn_SelectMap->SetVisibility(ESlateVisibility::Visible);
        if (Btn_Action) Btn_Action->SetVisibility(ESlateVisibility::Visible);
        if (Txt_ActionPrompt) Txt_ActionPrompt->SetText(FText::FromString(TEXT("게임 시작")));
    }
    else
    {
        if (Btn_SelectMap) Btn_SelectMap->SetVisibility(ESlateVisibility::Collapsed);
        if (Btn_Action) Btn_Action->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    if (Txt_PlayerCount) Txt_PlayerCount->SetText(FText::FromString(TEXT("현재 인원: 1 / 4")));
    if (Txt_MapName) Txt_MapName->SetText(FText::FromString(TEXT("레벨 선택 대기 중...")));
}

void UParcelLobbyHUDWidget::SetMenuVisibleState(bool bNewState)
{
    bIsMenuOpen = bNewState;

    APlayerController* PC = GetOwningPlayer(); 
    
    if (Canvas_MenuContainer)
    {
        Canvas_MenuContainer->SetVisibility(bIsMenuOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    
    K2_OnMenuStateChanged(bIsMenuOpen);

    if (bIsMenuOpen)
    {
        if (PC)
        {
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(TakeWidget());
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
        UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] 메뉴판 즉시 오픈: 커서 및 버튼 인터랙션 활성화."));
    }
    else
    {
        if (PC)
        {
            FInputModeGameOnly InputMode;
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = false;
            FSlateApplication::Get().SetAllUserFocusToGameViewport();
        }
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 메뉴판 즉시 폐쇄: 캐릭터 회전 및 전방 인풋 해제 완공."));
    }
}

void UParcelLobbyHUDWidget::SetChatInputInputMode(bool bFocusChat)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !EditableText_ChatInput) return;

    if (bFocusChat)
    {
        EditableText_ChatInput->SetVisibility(ESlateVisibility::Visible);
        
        FInputModeUIOnly ChatInputMode;
        ChatInputMode.SetWidgetToFocus(EditableText_ChatInput->TakeWidget());
        PC->SetInputMode(ChatInputMode);
    }
    else
    {
        EditableText_ChatInput->SetVisibility(ESlateVisibility::Collapsed);
        
        FInputModeGameOnly GameInputMode;
        PC->SetInputMode(GameInputMode);
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }
}

void UParcelLobbyHUDWidget::HandleChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    if (!Text.IsEmpty() && CommitMethod == ETextCommit::OnEnter)
    {
        if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(GetOwningPlayer()))
        {
            ParcelPC->Server_SendLobbyChatMessage(Text);
        }
        
        if (EditableText_ChatInput) EditableText_ChatInput->SetText(FText::GetEmpty());
    }
    
    SetChatInputInputMode(false);
}

void UParcelLobbyHUDWidget::AddChatLog(const FString& SenderName, const FText& Message)
{
    if (!ScrollBox_ChatLogs) return;
    
    UTextBlock* NewLogBlock = NewObject<UTextBlock>(this);
    if (NewLogBlock)
    {
        FString FormattedString = FString::Printf(TEXT("[%s] : %s"), *SenderName, *Message.ToString());
        NewLogBlock->SetText(FText::FromString(FormattedString));
        NewLogBlock->SetAutoWrapText(true);
        
        ScrollBox_ChatLogs->AddChild(NewLogBlock);
        ScrollBox_ChatLogs->ScrollToEnd();
    }
}

void UParcelLobbyHUDWidget::HandleOptionsClicked()
{
    PlayButtonClickSound();
    
    if (OptionsWidgetInstance && OptionsWidgetInstance->IsInViewport())
    {
        OptionsWidgetInstance->RemoveFromParent();
        OptionsWidgetInstance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 옵션 위젯 토글 닫기 완료."));
        return;
    }
    
    if (OptionsWidgetClass)
    {
        OptionsWidgetInstance = CreateWidget<UUserWidget>(GetOwningPlayer(), OptionsWidgetClass);
        if (OptionsWidgetInstance)
        {
            OptionsWidgetInstance->AddToViewport(500);
            UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] WBP_Options 옵션 인터페이스 파이프라인 개방 개시"));
        }
    }
}

void UParcelLobbyHUDWidget::HandleFriendsClicked()
{
    PlayButtonClickSound();
    
    if (FriendListWidgetInstance && FriendListWidgetInstance->IsInViewport())
    {
        FriendListWidgetInstance->CloseFriendList();
        FriendListWidgetInstance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 친구 목록 위젯 토글 닫기 완료."));
        return;
    }
    
    if (FriendListWidgetClass)
    {
        FriendListWidgetInstance = CreateWidget<UParcelFriendListWidget>(GetOwningPlayer(), FriendListWidgetClass);
        if (FriendListWidgetInstance)
        {
            FriendListWidgetInstance->AddToViewport(501);
            UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] UParcelFriendListWidget 스팀 친구 인터랙션 창구 오픈"));
        }
    }
}

void UParcelLobbyHUDWidget::HandleLeaveLobbyClicked()
{
    PlayButtonClickSound();
    
    UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] 로비 탈출 명령 감지. 안전 세션 철거 시퀀스 개시."));

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
        {
            SessionSubsystem->DestroySession();
            if (Btn_Leave) Btn_Leave->SetIsEnabled(false);
            
            UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] OSS 서브시스템에 세션 파괴 요청 송신 완료 -> 비동기 응답 대기 중..."));
            return;
        }
    }
    UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenuLevel"), true);
}

void UParcelLobbyHUDWidget::HandleOnSessionDestroyComplete(bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] OSS 세션 철거 완료 보고 수신 (성공 여부: %s) -> 메인 화면으로 전원 송환 처리!"), 
        bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
    
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::OpenLevel(World, TEXT("MainMenuLevel"), true);
    }
}

void UParcelLobbyHUDWidget::HandleSelectMapClicked()
{
    PlayButtonClickSound();
    
    APlayerController* PC = GetOwningPlayer();

    if (PC && PC->HasAuthority() && MapDataTable)
    {
        K2_OnMapSelectMenuOpened(); 
        
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 맵 선택 버튼 정상 가동 -> 블루프린트 서브 창고에 스폰 위임 완료."));
    }
}

void UParcelLobbyHUDWidget::HandleActionOrStartClicked()
{
    PlayButtonClickSound();
    
    APlayerController* PC = GetOwningPlayer();
    
    if (PC && PC->HasAuthority())
    {
        // 기본 폴백(Fallback) 맵 경로 설정
        FString SelectedMapPath = TEXT("/Game/Maps/LV_DF_Stage01");

        // MapDataTable에서 현재 선택된 인덱스의 MapPath 동적 추출
        if (MapDataTable)
        {
            TArray<FParcelMapStageData*> AllMapRows;
            MapDataTable->GetAllRows<FParcelMapStageData>(TEXT("StartGameMapContext"), AllMapRows);

            if (AllMapRows.IsValidIndex(LocalCurrentMapIndex) && AllMapRows[LocalCurrentMapIndex])
            {
                if (!AllMapRows[LocalCurrentMapIndex]->MapPath.IsEmpty())
                {
                    SelectedMapPath = AllMapRows[LocalCurrentMapIndex]->MapPath;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] 선택된 Row(%d)의 MapPath가 비어있어 기본 맵으로 진행합니다."), LocalCurrentMapIndex);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[Lobby HUD] 선택된 맵 인덱스(%d)가 데이터 테이블 범위를 벗어났습니다"), LocalCurrentMapIndex);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] MapDataTable이 할당되어 있지 않아 기본 맵 경로를 사용합니다."));
        }

        // Listen 서버 옵션(?listen) 결합
        FString TravelURL = FString::Printf(TEXT("%s?listen"), *SelectedMapPath);
        
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 방장 확정 ➔ 선택된 맵(%s)으로 강제 트래블(ServerTravel) 개시 URL: %s"), 
            *SelectedMapPath, *TravelURL);

        // 동적 경로로 ServerTravel 실행
        if (UWorld* World = GetWorld())
        {
            World->ServerTravel(TravelURL);
        }
    }
}

void UParcelLobbyHUDWidget::RefreshLobbyPlayers()
{
    if (!ScrollBox_LobbyPlayers || !PlayerSlotClass) return;
    
    ScrollBox_LobbyPlayers->ClearChildren();

    AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] GameState가 아직 복제되지 않아 인원 리프레시를 보류합니다."));
        return;
    }

    int32 CurrentCount = GS->PlayerArray.Num();
    int32 MaxCount = 4;

    if (Txt_PlayerCount)
    {
        FString CountStr = FString::Printf(TEXT("현재 인원: %d / %d"), CurrentCount, MaxCount);
        Txt_PlayerCount->SetText(FText::FromString(CountStr));
    }

    for (int32 i = 0; i < GS->PlayerArray.Num(); ++i)
    {
        APlayerState* PS = GS->PlayerArray[i].Get();
        if (!PS) continue;

        UParcelLobbyPlayerSlotWidget* NewSlot = CreateWidget<UParcelLobbyPlayerSlotWidget>(GetOwningPlayer(), PlayerSlotClass);
        if (NewSlot)
        {
            bool bIsHost = (i == 0) || (PS->GetOwningController() && PS->GetOwningController()->HasAuthority());
            NewSlot->InitializeSlot(PS, bIsHost);
            ScrollBox_LobbyPlayers->AddChild(NewSlot);
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 서버 복제 배열 동기화 완료. 총 %d개의 플레이어 슬롯 재생성 완공."), CurrentCount);
    
    if (!bStatDelegatesBound)
    {
        APawn* LocalPawn = GetOwningPlayerPawn();
        if (LocalPawn)
        {
            // 1. 스태미나 컴포넌트 바인딩
            if (UParcelStaminaComponent* StaminaComp = LocalPawn->FindComponentByClass<UParcelStaminaComponent>())
            {
                StaminaComp->OnStaminaChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeStaminaChanged);
                HandleNativeStaminaChanged(StaminaComp->GetCurrentStamina(), StaminaComp->GetMaxStamina());
            }
            
            // 2. 체력 컴포넌트 바인딩
            if (UHealthComponent* HealthComp = LocalPawn->FindComponentByClass<UHealthComponent>())
            {
                HealthComp->OnHPChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeHPChanged);
                HandleNativeHPChanged(HealthComp->GetHP(), HealthComp->GetMaxHP());
            }
            
            // 3. 상호작용 컴포넌트 바인딩 (E키 UI 프롬프트)
            if (UParcelInteractionComponent* InteractComp = LocalPawn->FindComponentByClass<UParcelInteractionComponent>())
            {
                InteractComp->OnFocusChanged.RemoveDynamic(this, &UParcelLobbyHUDWidget::HandleNativeInteractionFocusChanged);
                InteractComp->OnFocusChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeInteractionFocusChanged);
                
                HandleNativeInteractionFocusChanged(InteractComp->GetCurrentFocusedActor());
            }

            // 4. 히어로 컴포넌트 바인딩 (던지기 차징 게이지)
            if (UParcelHeroComponent* HeroComp = LocalPawn->FindComponentByClass<UParcelHeroComponent>())
            {
                HeroComp->OnThrowChargeChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeThrowChargeChanged);
            }
            
            // 5. 운반 컴포넌트 바인딩
            if (UCharacterCarryComponent* CarryComp = LocalPawn->FindComponentByClass<UCharacterCarryComponent>())
            {
                CarryComp->OnCarriedBoxChanged.RemoveDynamic(this, &UParcelLobbyHUDWidget::HandleNativeCarriedBoxChanged);
                CarryComp->OnCarriedBoxChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeCarriedBoxChanged);
                
                HandleNativeCarriedBoxChanged(CarryComp->GetCarriedBox());
            }
            
            // 6. 인벤토리 바인딩
            if (APlayerState* PS = LocalPawn->GetPlayerState())
            {
                if (UInventoryComponent* InvComp = PS->FindComponentByClass<UInventoryComponent>())
                {
                    InvComp->OnInventoryChanged.RemoveDynamic(this, &UParcelLobbyHUDWidget::HandleNativeInventoryChanged);
                    InvComp->OnInventoryChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeInventoryChanged);
                    
                    HandleNativeInventoryChanged();
                }
            }

            bStatDelegatesBound = true;
            UE_LOG(LogTemp, Log, TEXT("[Lobby HUD Core] 대기실 로컬 캐릭터의 스태미나/체력/상호작용/차징 게이지 인터셉트망 최종 완공!"));
        }
    }
}

void UParcelLobbyHUDWidget::HandleNativeCarriedBoxChanged(ADeliveryBox* NewCarriedBox)
{
    if (CachedCarriedBox.IsValid())
    {
        if (UHealthComponent* OldHealth = CachedCarriedBox->FindComponentByClass<UHealthComponent>())
        {
            OldHealth->OnHPChanged.RemoveDynamic(this, &UParcelLobbyHUDWidget::HandleNativeCarriedBoxHPChanged);
        }
    }

    CachedCarriedBox = NewCarriedBox;

    if (!NewCarriedBox)
    {
        K2_OnCarriedBoxInfoChanged(false, FText::GetEmpty(), FText::GetEmpty(), FGameplayTag(), FText::GetEmpty());
        return;
    }

    if (UHealthComponent* HealthComp = NewCarriedBox->FindComponentByClass<UHealthComponent>())
    {
        HealthComp->OnHPChanged.RemoveDynamic(this, &UParcelLobbyHUDWidget::HandleNativeCarriedBoxHPChanged);
        HealthComp->OnHPChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleNativeCarriedBoxHPChanged);
        HandleNativeCarriedBoxHPChanged(HealthComp->GetHP(), HealthComp->GetMaxHP());
    }
    else
    {
        HandleNativeCarriedBoxHPChanged(0.f, 0.f);
    }
}

void UParcelLobbyHUDWidget::HandleNativeCarriedBoxHPChanged(float CurrentHP, float MaxHP)
{
    if (!CachedCarriedBox.IsValid())
    {
        K2_OnCarriedBoxInfoChanged(false, FText::GetEmpty(), FText::GetEmpty(), FGameplayTag(), FText::GetEmpty());
        return;
    }

    FBoxData CarriedBoxData = CachedCarriedBox->GetBoxData();
    
    FText BoxNameText = FText::FromString(CarriedBoxData.DisplayName);
    FText FormattedName = FText::Format(
        FText::FromString(TEXT("{0} ({1}kg)")), 
        BoxNameText, 
        FText::AsNumber(CarriedBoxData.Weight)
    );
    
    FText DestinationText = FText::FromString(TEXT("목적지 : 미지정 구역"));
    if (CarriedBoxData.TargetZoneTag.IsValid())
    {
        FString ZoneString = CarriedBoxData.TargetZoneTag.ToString();
        ZoneString.ReplaceInline(TEXT("Delivery."), TEXT(""));
        ZoneString.ReplaceInline(TEXT("Zone."), TEXT(""));
        
        DestinationText = FText::Format(
            FText::FromString(TEXT("목적지 : {0} 구역")), 
            FText::FromString(ZoneString)
        );
    }

    FText BoxHPText = FText::FromString(TEXT("내구도 : -"));
    if (UHealthComponent* HealthComp = CachedCarriedBox->FindComponentByClass<UHealthComponent>())
    {
        int32 CurHPVal = FMath::RoundToInt(HealthComp->GetHP());
        int32 MaxHPVal = FMath::RoundToInt(HealthComp->GetMaxHP());
        BoxHPText = FText::Format(
            FText::FromString(TEXT("내구도 : {0} / {1}")),
            FText::AsNumber(CurHPVal),
            FText::AsNumber(MaxHPVal)
        );
    }
    
    K2_OnCarriedBoxInfoChanged(true, FormattedName, DestinationText, CarriedBoxData.BoxTypeTag, BoxHPText);
}

void UParcelLobbyHUDWidget::HandleNativeInteractionFocusChanged(AActor* NewFocusedActor)
{
    if (NewFocusedActor)
    {
        FText PromptText = FText::FromString(TEXT("E 키를 눌러 상호작용"));
        K2_OnCrosshairStateChanged(true, PromptText);
    }
    else
    {
        K2_OnCrosshairStateChanged(false, FText::GetEmpty());
    }
}

void UParcelLobbyHUDWidget::HandleNativeThrowChargeChanged(bool bIsCharging, float ChargeRatio)
{
    K2_OnThrowChargeChanged(bIsCharging, ChargeRatio);
}

void UParcelLobbyHUDWidget::HandleNativeHPChanged(float CurrentHP, float MaxHP)
{
    K2_OnHPChanged(CurrentHP, MaxHP);
}

void UParcelLobbyHUDWidget::HandleNativeStaminaChanged(float CurrentStamina, float MaxStamina)
{
    K2_OnStaminaChanged(CurrentStamina, MaxStamina);
}

void UParcelLobbyHUDWidget::HandleOnLobbyMapChanged(int32 NewMapIndex)
{
    if (!MapDataTable) return;
    
    TArray<FParcelMapStageData*> AllMapRows;
    MapDataTable->GetAllRows<FParcelMapStageData>(TEXT("MapRenderingContext"), AllMapRows);

    if (!AllMapRows.IsValidIndex(NewMapIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("[Lobby HUD] 복제된 맵 인덱스(%d)가 데이터 테이블 범위를 벗어났습니다!"), NewMapIndex);
        return;
    }
    
    FParcelMapStageData* SelectedStageData = AllMapRows[NewMapIndex];
    if (!SelectedStageData) return;

    if (Txt_MapName)
    {
        Txt_MapName->SetText(FText::FromString(SelectedStageData->StageName));
    }
    
    if (Img_MapThumbnail)
    {
        UTexture2D* LoadedThumbnail = SelectedStageData->StageThumbnail.LoadSynchronous();
        if (LoadedThumbnail)
        {
            Img_MapThumbnail->SetBrushFromTexture(LoadedThumbnail);
        }
    }
    
    LocalCurrentMapIndex = NewMapIndex;
    UE_LOG(LogTemp, Log, TEXT("[Lobby HUD Synced] 전 클라이언트 화면에 %s 맵 비주얼 동기화 렌더링 완료"), *SelectedStageData->StageName);
}

void UParcelLobbyHUDWidget::ToggleLobbyMenuExternal()
{
    SetMenuVisibleState(!bIsMenuOpen);
}

void UParcelLobbyHUDWidget::SelectMapByIndex(int32 NewMapIndex)
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->HasAuthority()) return;

    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(PC))
    {
        ParcelPC->Server_RequestChangeLobbyMap(NewMapIndex);
        
        if (PC->HasAuthority())
        {
            HandleOnLobbyMapChanged(NewMapIndex);
            UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 방장 로컬 화면 즉시 동기화 가동 ➔ %d번 맵 반영."), NewMapIndex);
        }
    }
}

void UParcelLobbyHUDWidget::HandleNativeInventoryChanged()
{
    if (APawn* LocalPawn = GetOwningPlayerPawn())
    {
        if (APlayerState* PS = LocalPawn->GetPlayerState())
        {
            if (UInventoryComponent* InvComp = PS->FindComponentByClass<UInventoryComponent>())
            {
                K2_OnInventoryChanged(InvComp->GetItems());
            }
        }
    }
}

bool UParcelLobbyHUDWidget::GetItemDataByTag(FGameplayTag ItemTag, FItemData& OutItemData) const
{
    if (!ItemTable || !ItemTag.IsValid())
    {
        return false;
    }

    TArray<FItemData*> AllRows;
    ItemTable->GetAllRows<FItemData>(TEXT("GetItemDataByTag"), AllRows);
    for (const FItemData* Row : AllRows)
    {
        if (Row && Row->ItemTag == ItemTag)
        {
            OutItemData = *Row;
            return true;
        }
    }
    return false;
}

void UParcelLobbyHUDWidget::PlayButtonClickSound()
{
    if (ButtonClickSound)
    {
        // 뷰포트에 2D로 UI 효과음 출력
        UGameplayStatics::PlaySound2D(this, ButtonClickSound);
    }
}