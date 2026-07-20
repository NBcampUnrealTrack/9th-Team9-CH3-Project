#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlueprintDataDefinitions.h"
#include "ParcelFriendListEntryWidget.generated.h"

class UButton;
class UParcelFriendListWidget;
class UTexture2D;

/**
 * WBP_FriendListEntry의 C++ 베이스.
 * 친구 데이터와 INVITE 버튼 이벤트만 담당하며 시각 디자인은 Blueprint에서 구성합니다.
 */
UCLASS(Abstract, Blueprintable)
class PARCEL_KNIGHT_API UParcelFriendListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeFriend(
		UParcelFriendListWidget* InOwnerList,
		const FBPFriendInfo& InFriendInfo,
		bool bInCanInvite);

	void SetInviteEnabled(bool bInCanInvite);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> InviteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	FText FriendDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	FText OnlineStatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	bool bIsOnline = false;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	bool bIsPlayingThisGame = false;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	bool bCanInvite = false;

	UPROPERTY(BlueprintReadOnly, Category = "Friend")
	TObjectPtr<UTexture2D> AvatarTexture;

	UFUNCTION(BlueprintImplementableEvent, Category = "Friend", meta = (DisplayName = "On Friend Data Updated"))
	void K2_OnFriendDataUpdated(
		const FText& DisplayName,
		const FText& StatusText,
		bool bOnline,
		bool bPlayingThisGame,
		UTexture2D* Avatar,
		bool bInviteEnabled);

private:
	UFUNCTION()
	void HandleInviteClicked();

	void RefreshVisualData();
	void TryLoadAvatar();

	FBPFriendInfo FriendInfo;

	UPROPERTY()
	TObjectPtr<UParcelFriendListWidget> OwnerList;

	FTimerHandle AvatarRetryTimerHandle;
	int32 AvatarRetryCount = 0;
	static constexpr int32 MaxAvatarRetryCount = 20;
};
