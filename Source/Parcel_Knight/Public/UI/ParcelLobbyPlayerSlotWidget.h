#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelLobbyPlayerSlotWidget.generated.h"

class UTextBlock;
class UImage;
class APlayerState;
class UTexture2D;

/**
 * UParcelLobbyPlayerSlotWidget
 * 담당자 : JYW
 */
UCLASS(Abstract, Blueprintable)
class PARCEL_KNIGHT_API UParcelLobbyPlayerSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeSlot(APlayerState* InPlayerState, bool bIsHost);
	
	void UpdateReadyState(bool bInIsReady);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// UMG 위젯 바인딩
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
	TObjectPtr<UTextBlock> Txt_LobbyPlayerName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
	TObjectPtr<UTextBlock> Txt_LobbyPlayerStatus;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) 
	TObjectPtr<UImage> Img_LobbyPlayerAvatar;

	// 블루프린트 연출용 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "LobbySlot")
	void K2_OnSlotInitialized(bool bIsHost);

private:
	void TryLoadSteamAvatar();

	UPROPERTY(Transient)
	TObjectPtr<APlayerState> CachedPlayerState = nullptr;

	FTimerHandle AvatarRetryTimerHandle;
	int32 AvatarRetryCount = 0;
	static constexpr int32 MaxAvatarRetryCount = 10;
};