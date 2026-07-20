#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlueprintDataDefinitions.h"
#include "ParcelFriendListWidget.generated.h"

class UButton;
class UGetFriendsCallbackProxy;
class UPanelWidget;
class UParcelFriendListEntryWidget;
class UTextBlock;

/**
 * WBP_FriendList의 C++ 베이스.
 * 비동기 Steam 친구 조회, 행 생성, 초대 결과 메시지만 담당합니다.
 */
UCLASS(Abstract, Blueprintable)
class PARCEL_KNIGHT_API UParcelFriendListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Friend")
	void RefreshFriends();

	UFUNCTION(BlueprintCallable, Category = "Friend")
	void CloseFriendList();

	// C++-only presentation state supplied by the owning HUD before opening.
	void SetRestoreUIInputMode(bool bShouldRestoreUIInputMode);

	void InviteFriend(const FBPUniqueNetId& FriendUniqueNetId, const FString& DisplayName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Friend")
	TSubclassOf<UParcelFriendListEntryWidget> FriendListEntryClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> FriendEntriesContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Friend", meta = (DisplayName = "On Status Message Changed"))
	void K2_OnStatusMessageChanged(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Friend", meta = (DisplayName = "On Friend List Loading Changed"))
	void K2_OnLoadingChanged(bool bIsLoading);

private:
	UFUNCTION()
	void HandleFriendsLoaded(const TArray<FBPFriendInfo>& Friends);

	UFUNCTION()
	void HandleFriendsLoadFailed(const TArray<FBPFriendInfo>& Friends);

	UFUNCTION()
	void HandleSessionStatusMessage(const FText& Message, bool bIsError);

	UFUNCTION()
	void HandleSessionAvailabilityChanged(bool bWasSuccessful);

	void SetStatusMessage(const FText& Message);
	void FinishFriendsRequest();
	void RefreshInviteAvailability();

	UPROPERTY()
	TObjectPtr<UGetFriendsCallbackProxy> ActiveFriendsRequest;

	bool bRestoreUIInputMode = false;
};
