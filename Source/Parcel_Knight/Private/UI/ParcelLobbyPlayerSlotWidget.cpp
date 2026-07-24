#include "UI/ParcelLobbyPlayerSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "GameFramework/PlayerState.h"
#include "AdvancedSteamFriendsLibrary.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

void UParcelLobbyPlayerSlotWidget::InitializeSlot(APlayerState* InPlayerState, bool bIsHost)
{
    if (!InPlayerState) return;

    CachedPlayerState = InPlayerState;
    
    if (Txt_LobbyPlayerName)
    {
        Txt_LobbyPlayerName->SetText(FText::FromString(InPlayerState->GetPlayerName()));
    }
    
    if (Txt_LobbyPlayerStatus)
    {
        FString ClearStatus = bIsHost ? TEXT("방장 (Host)") : TEXT("대기 중...");
        Txt_LobbyPlayerStatus->SetText(FText::FromString(ClearStatus));
    }
    
    TryLoadSteamAvatar();
    
    K2_OnSlotInitialized(bIsHost);
}

void UParcelLobbyPlayerSlotWidget::UpdateReadyState(bool bInIsReady)
{
    if (Txt_LobbyPlayerStatus)
    {
        FString StatusStr = bInIsReady ? TEXT("준비 완료!") : TEXT("대기 중...");
        Txt_LobbyPlayerStatus->SetText(FText::FromString(StatusStr));
    }
}

void UParcelLobbyPlayerSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UParcelLobbyPlayerSlotWidget::NativeDestruct()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AvatarRetryTimerHandle);
    }
    Super::NativeDestruct();
}

void UParcelLobbyPlayerSlotWidget::TryLoadSteamAvatar()
{
    if (!CachedPlayerState) return;
    
    FBPUniqueNetId UniqueNetId;
    UniqueNetId.UniqueNetId = CachedPlayerState->GetUniqueId().GetUniqueNetId();

    if (!UniqueNetId.IsValid()) return;

    EBlueprintAsyncResultSwitch Result = EBlueprintAsyncResultSwitch::OnFailure;
    
    if (UTexture2D* LoadedAvatar = UAdvancedSteamFriendsLibrary::GetSteamFriendAvatar(
        UniqueNetId, Result, SteamAvatarSize::SteamAvatar_Medium))
    {
        if (Img_LobbyPlayerAvatar)
        {
            Img_LobbyPlayerAvatar->SetBrushFromTexture(LoadedAvatar);
        }
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(AvatarRetryTimerHandle);
        }
        return;
    }
    
    if (Result == EBlueprintAsyncResultSwitch::AsyncLoading && AvatarRetryCount < MaxAvatarRetryCount)
    {
        UAdvancedSteamFriendsLibrary::RequestSteamFriendInfo(UniqueNetId, false);
        ++AvatarRetryCount;

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(
                AvatarRetryTimerHandle, this, &UParcelLobbyPlayerSlotWidget::TryLoadSteamAvatar, 0.3f, false);
        }
    }
}