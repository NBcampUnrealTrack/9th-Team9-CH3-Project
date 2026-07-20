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
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ParcelFriendListWidget.h"
#include "Core/SessionSubsystem.h"

void UParcelLobbyHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 1. 캔버스 및 초기 메뉴 상태 숨김 가드 세팅
    if (Canvas_MenuContainer)
    {
        Canvas_MenuContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (Txt_EscPrompt)
    {
        Txt_EscPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    bIsMenuOpen = false;
    
    // 2. 초기 로비 인풋 모드 설정 (마우스 커서 숨기기)
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }
    
    // 3. 로비 레이아웃 (방장/손님 권한별 버튼 가시성) 초기 1회 설정
    SetupLobbyLayout();
    
    // 4. 조작용 단추 클릭 이벤트 동적 연결
    if (Btn_Options)   Btn_Options->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOptionsClicked);
    if (Btn_Friends)   Btn_Friends->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleFriendsClicked);
    if (Btn_Leave)     Btn_Leave->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleLeaveLobbyClicked);
    if (Btn_SelectMap) Btn_SelectMap->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleSelectMapClicked);
    if (Btn_Action)    Btn_Action->OnClicked.AddDynamic(this, &UParcelLobbyHUDWidget::HandleActionOrStartClicked);

    // 5. 엔터키 감지 채팅 입력기 초기화
    if (EditableText_ChatInput)
    {
        EditableText_ChatInput->OnTextCommitted.AddDynamic(this, &UParcelLobbyHUDWidget::HandleChatTextCommitted);
        EditableText_ChatInput->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 6. 플레이어 컨트롤러에 내 HUD 위젯 주소 명함 건네기 (채팅 배달용)
    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(GetOwningPlayer()))
    {
        ParcelPC->LobbyHUDWidgetInstance = this;
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 플레이어 컨트롤러 상에 HUD 인스턴스 주소 등록 완료."));
    }
    
    // 7. GameState를 파싱하여 멀티플레이어 맵 레이턴시 동기화 랜선 직결
    if (GetWorld())
    {
        if (AParcelGameState* ParcelGS = GetWorld()->GetGameState<AParcelGameState>())
        {
            ParcelGS->OnLobbyMapChanged.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOnLobbyMapChanged);
            
            // 클라이언트를 위해 현재 서버 공인 맵 인덱스로 화면 동기화
            HandleOnLobbyMapChanged(ParcelGS->GetSelectedMapIndex());
        }
    }
    
    // 8. Destroy 세션 서브시스템 연결
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USessionSubsystem* SessionSubsystem = GI->GetSubsystem<USessionSubsystem>())
        {
            SessionSubsystem->OnSessionDestroyComplete.AddDynamic(this, &UParcelLobbyHUDWidget::HandleOnSessionDestroyComplete);
        }
    }
    SetIsFocusable(true);
}

FReply UParcelLobbyHUDWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    FKey PressedKey = InKeyEvent.GetKey();

    if (PressedKey == EKeys::Escape)
    {
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] ESC 키 감지. 현재 메뉴 상태: %s"), bIsMenuOpen ? TEXT("열림") : TEXT("닫힘"));
        
        SetMenuVisibleState(!bIsMenuOpen);
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
        if (Txt_ActionPrompt) Txt_ActionPrompt->SetText(FText::FromString(TEXT("게임 시작 (Host)")));
    }
    else
    {
        if (Btn_SelectMap) Btn_SelectMap->SetVisibility(ESlateVisibility::Collapsed);
        if (Txt_ActionPrompt) Txt_ActionPrompt->SetText(FText::FromString(TEXT("준비 완료 (Client)")));
    }
    
    if (Txt_PlayerCount) Txt_PlayerCount->SetText(FText::FromString(TEXT("현재 인원: 1 / 4")));
    if (Txt_MapName) Txt_MapName->SetText(FText::FromString(TEXT("컨베이어 창고 스테이지 01")));
}

void UParcelLobbyHUDWidget::SetMenuVisibleState(bool bNewState)
{
    bIsMenuOpen = bNewState;
    APlayerController* PC = GetOwningPlayer();
    
    K2_OnMenuStateChanged(bIsMenuOpen);

    if (bIsMenuOpen)
    {
        if (PC)
        {
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(TakeWidget());
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
        UE_LOG(LogTemp, Warning, TEXT("[Lobby HUD] 메뉴판 오픈 가동: UI 포커스 구속 및 커서 활성화."));
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
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 메뉴판 폐쇄 가동: 시점 자유 조작 복구 복원 완료."));
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
    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->HasAuthority() || !MapDataTable) return;
    
    TArray<FParcelMapStageData*> AllMapRows;
    MapDataTable->GetAllRows<FParcelMapStageData>(TEXT("MapParsingContext"), AllMapRows);
    
    if (AllMapRows.IsEmpty()) return;

    int32 CurrentSyncedIndex = 0;
    if (AParcelGameState* ParcelGS = GetWorld() ? GetWorld()->GetGameState<AParcelGameState>() : nullptr)
    {
        CurrentSyncedIndex = ParcelGS->GetSelectedMapIndex();
    }
    
    LocalCurrentMapIndex = (CurrentSyncedIndex + 1) % AllMapRows.Num();
    
    if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(PC))
    {
        ParcelPC->Server_RequestChangeLobbyMap(LocalCurrentMapIndex);
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 방장 조작 센서 터짐 -> 서버에 %d번 맵 인덱스 동기화 요청"), LocalCurrentMapIndex);
    }
}

void UParcelLobbyHUDWidget::HandleActionOrStartClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC && PC->HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 방장 전용: 리슨서버 전원 인게임 배달 구역(/Game/Maps/LV_DF_Stage01)으로 강제 트래블 개시"));
        GetWorld()->ServerTravel(TEXT("/Game/Maps/LV_DF_Stage01?listen"));
    }
    else
    {
        bIsReady = !bIsReady;
        if (Txt_ActionPrompt)
        {
            FString PromptStr = bIsReady ? TEXT("준비 취소") : TEXT("준비 완료 (Client)");
            Txt_ActionPrompt->SetText(FText::FromString(PromptStr));
        }
        UE_LOG(LogTemp, Log, TEXT("[Lobby HUD] 클라이언트 준비 토글 변경: %s"), bIsReady ? TEXT("Ready") : TEXT("Not Ready"));
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