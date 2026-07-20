#include "UI/ParcelFriendListEntryWidget.h"

#include "AdvancedSteamFriendsLibrary.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "UI/ParcelFriendListWidget.h"

void UParcelFriendListEntryWidget::InitializeFriend(
	UParcelFriendListWidget* InOwnerList,
	const FBPFriendInfo& InFriendInfo,
	bool bInCanInvite)
{
	OwnerList = InOwnerList;
	FriendInfo = InFriendInfo;
	bCanInvite = bInCanInvite && FriendInfo.UniqueNetId.IsValid();

	FriendDisplayName = FText::FromString(FriendInfo.DisplayName);
	bIsOnline = FriendInfo.PresenceInfo.bIsOnline ||
		FriendInfo.OnlineState != EBPOnlinePresenceState::Offline;
	bIsPlayingThisGame = FriendInfo.bIsPlayingSameGame ||
		FriendInfo.PresenceInfo.bIsPlayingThisGame;
	OnlineStatusText = bIsOnline
		? FText::FromString(TEXT("온라인"))
		: FText::FromString(TEXT("오프라인"));

	RefreshVisualData();
	TryLoadAvatar();
}

void UParcelFriendListEntryWidget::SetInviteEnabled(bool bInCanInvite)
{
	bCanInvite = bInCanInvite && FriendInfo.UniqueNetId.IsValid();
	RefreshVisualData();
}

void UParcelFriendListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InviteButton)
	{
		InviteButton->OnClicked.RemoveDynamic(this, &UParcelFriendListEntryWidget::HandleInviteClicked);
		InviteButton->OnClicked.AddDynamic(this, &UParcelFriendListEntryWidget::HandleInviteClicked);
		InviteButton->SetIsEnabled(bCanInvite);
	}

	RefreshVisualData();
}

void UParcelFriendListEntryWidget::NativeDestruct()
{
	if (InviteButton)
	{
		InviteButton->OnClicked.RemoveDynamic(this, &UParcelFriendListEntryWidget::HandleInviteClicked);
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AvatarRetryTimerHandle);
	}

	Super::NativeDestruct();
}

void UParcelFriendListEntryWidget::HandleInviteClicked()
{
	if (OwnerList)
	{
		OwnerList->InviteFriend(FriendInfo.UniqueNetId, FriendInfo.DisplayName);
	}
}

void UParcelFriendListEntryWidget::RefreshVisualData()
{
	if (InviteButton)
	{
		InviteButton->SetIsEnabled(bCanInvite);
	}

	K2_OnFriendDataUpdated(
		FriendDisplayName,
		OnlineStatusText,
		bIsOnline,
		bIsPlayingThisGame,
		AvatarTexture,
		bCanInvite);
}

void UParcelFriendListEntryWidget::TryLoadAvatar()
{
	if (!FriendInfo.UniqueNetId.IsValid())
	{
		return;
	}

	EBlueprintAsyncResultSwitch Result = EBlueprintAsyncResultSwitch::OnFailure;
	if (UTexture2D* LoadedAvatar = UAdvancedSteamFriendsLibrary::GetSteamFriendAvatar(
		FriendInfo.UniqueNetId,
		Result,
		SteamAvatarSize::SteamAvatar_Medium))
	{
		AvatarTexture = LoadedAvatar;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(AvatarRetryTimerHandle);
		}
		RefreshVisualData();
		return;
	}

	if (Result == EBlueprintAsyncResultSwitch::AsyncLoading && AvatarRetryCount < MaxAvatarRetryCount)
	{
		UAdvancedSteamFriendsLibrary::RequestSteamFriendInfo(FriendInfo.UniqueNetId, false);
		++AvatarRetryCount;

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				AvatarRetryTimerHandle,
				this,
				&UParcelFriendListEntryWidget::TryLoadAvatar,
				0.25f,
				false);
		}
	}
}
