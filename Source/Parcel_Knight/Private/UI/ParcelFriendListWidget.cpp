#include "UI/ParcelFriendListWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Core/SessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GetFriendsCallbackProxy.h"
#include "UI/ParcelFriendListEntryWidget.h"

namespace ParcelFriendListMessages
{
	const FText NoInvitableSession =
		FText::FromString(TEXT("호스트가 아니거나 초대 가능한 세션이 없습니다."));
}

void UParcelFriendListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &UParcelFriendListWidget::RefreshFriends);
		RefreshButton->OnClicked.AddDynamic(this, &UParcelFriendListWidget::RefreshFriends);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UParcelFriendListWidget::CloseFriendList);
		CloseButton->OnClicked.AddDynamic(this, &UParcelFriendListWidget::CloseFriendList);
	}

	RefreshFriends();
}

void UParcelFriendListWidget::NativeDestruct()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &UParcelFriendListWidget::RefreshFriends);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UParcelFriendListWidget::CloseFriendList);
	}

	FinishFriendsRequest();
	Super::NativeDestruct();
}

void UParcelFriendListWidget::RefreshFriends()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		SetStatusMessage(FText::FromString(TEXT("로컬 플레이어의 Steam 친구 목록만 조회할 수 있습니다.")));
		return;
	}

	FinishFriendsRequest();

	if (FriendEntriesContainer)
	{
		FriendEntriesContainer->ClearChildren();
	}

	SetStatusMessage(FText::FromString(TEXT("Steam 친구 목록을 불러오는 중입니다.")));
	K2_OnLoadingChanged(true);

	ActiveFriendsRequest = UGetFriendsCallbackProxy::GetAndStoreFriendsList(this, PlayerController);
	if (!ActiveFriendsRequest)
	{
		K2_OnLoadingChanged(false);
		SetStatusMessage(FText::FromString(TEXT("Steam 친구 목록 요청을 시작하지 못했습니다.")));
		return;
	}

	ActiveFriendsRequest->OnSuccess.AddDynamic(this, &UParcelFriendListWidget::HandleFriendsLoaded);
	ActiveFriendsRequest->OnFailure.AddDynamic(this, &UParcelFriendListWidget::HandleFriendsLoadFailed);
	ActiveFriendsRequest->Activate();
}

void UParcelFriendListWidget::CloseFriendList()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}

	RemoveFromParent();
}

void UParcelFriendListWidget::InviteFriend(
	const FBPUniqueNetId& FriendUniqueNetId,
	const FString& DisplayName)
{
	APlayerController* PlayerController = GetOwningPlayer();
	USessionSubsystem* SessionSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USessionSubsystem>()
		: nullptr;

	if (!PlayerController || !SessionSubsystem || !SessionSubsystem->CanInviteToCurrentSession())
	{
		SetStatusMessage(ParcelFriendListMessages::NoInvitableSession);
		return;
	}

	if (SessionSubsystem->SendSessionInviteToFriend(PlayerController, FriendUniqueNetId))
	{
		SetStatusMessage(FText::Format(
			FText::FromString(TEXT("{0} 님에게 현재 세션 초대를 보냈습니다.")),
			FText::FromString(DisplayName)));
	}
	else
	{
		SetStatusMessage(FText::Format(
			FText::FromString(TEXT("{0} 님에게 세션 초대를 보내지 못했습니다.")),
			FText::FromString(DisplayName)));
	}
}

void UParcelFriendListWidget::HandleFriendsLoaded(const TArray<FBPFriendInfo>& Friends)
{
	FinishFriendsRequest();
	K2_OnLoadingChanged(false);

	USessionSubsystem* SessionSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USessionSubsystem>()
		: nullptr;
	const bool bCanInvite = SessionSubsystem && SessionSubsystem->CanInviteToCurrentSession();

	if (FriendEntriesContainer && FriendListEntryClass)
	{
		for (const FBPFriendInfo& Friend : Friends)
		{
			UParcelFriendListEntryWidget* Entry = CreateWidget<UParcelFriendListEntryWidget>(
				GetOwningPlayer(),
				FriendListEntryClass);
			if (!Entry)
			{
				continue;
			}

			Entry->InitializeFriend(this, Friend, bCanInvite);
			FriendEntriesContainer->AddChild(Entry);
		}
	}

	if (Friends.IsEmpty())
	{
		SetStatusMessage(FText::FromString(
			TEXT("Steam 친구가 없거나 현재 친구 목록을 가져올 수 없습니다.")));
	}
	else if (!bCanInvite)
	{
		SetStatusMessage(ParcelFriendListMessages::NoInvitableSession);
	}
	else
	{
		SetStatusMessage(FText::Format(
			FText::FromString(TEXT("Steam 친구 {0}명을 불러왔습니다.")),
			FText::AsNumber(Friends.Num())));
	}
}

void UParcelFriendListWidget::HandleFriendsLoadFailed(const TArray<FBPFriendInfo>& Friends)
{
	FinishFriendsRequest();
	K2_OnLoadingChanged(false);
	SetStatusMessage(FText::FromString(
		TEXT("Steam 친구 목록을 불러오지 못했습니다. Steam 로그인과 실행 모드를 확인해 주세요.")));
}

void UParcelFriendListWidget::SetStatusMessage(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}

	K2_OnStatusMessageChanged(Message);
}

void UParcelFriendListWidget::FinishFriendsRequest()
{
	if (ActiveFriendsRequest)
	{
		ActiveFriendsRequest->OnSuccess.RemoveDynamic(this, &UParcelFriendListWidget::HandleFriendsLoaded);
		ActiveFriendsRequest->OnFailure.RemoveDynamic(this, &UParcelFriendListWidget::HandleFriendsLoadFailed);
		ActiveFriendsRequest = nullptr;
	}
}
