#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "ParcelLobbyHUDWidget.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class UEditableText;
class UImage;
class UCanvasPanel;
class UWidgetAnimation;
class UParcelLobbyPlayerSlotWidget;
class ADeliveryBox;

/**
 * 맵 스테이지 구조체
 */
USTRUCT(BlueprintType)
struct FParcelMapStageData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapData")
    FString StageName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapData")
    FString MapPath;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapData")
    TSoftObjectPtr<UTexture2D> StageThumbnail;
};

/**
 * UParcelLobbyHUDWidget
 * 담당자 : JYW
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelLobbyHUDWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    
    virtual void NativeDestruct() override;
    
    virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    
    void SetupLobbyLayout();
    
    void SetMenuVisibleState(bool bNewState);
    
    UFUNCTION() void HandleOptionsClicked();
    UFUNCTION() void HandleFriendsClicked();
    UFUNCTION() void HandleLeaveLobbyClicked();
    UFUNCTION() void HandleSelectMapClicked();
    UFUNCTION() void HandleActionOrStartClicked();
    UFUNCTION() void HandleChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    UFUNCTION() void HandleOnSessionDestroyComplete(bool bWasSuccessful);
    UFUNCTION() void HandleOnLobbyMapChanged(int32 NewMapIndex);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Carry")
    void K2_OnCarriedBoxInfoChanged(bool bIsCarrying, const FText& BoxTypeName, const FText& DestinationText, FGameplayTag BoxTypeTag, const FText& BoxHPText);

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UCanvasPanel> Canvas_MenuContainer;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_EscPrompt;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Options;
    UPROPERTY(BlueprintReadOnly, meta = (BlueprintReadOnly, BindWidget)) TObjectPtr<UButton> Btn_Friends;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Leave;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_SelectMap;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_Action;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_ActionPrompt;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_PlayerCount;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_MapName;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UImage> Img_MapThumbnail;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UScrollBox> ScrollBox_LobbyPlayers;
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UScrollBox> ScrollBox_ChatLogs;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UEditableText> EditableText_ChatInput;

private:
    bool bIsMenuOpen = false;
    bool bIsReady = false;
    
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|UI")
    TSubclassOf<UParcelLobbyPlayerSlotWidget> PlayerSlotClass;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Data")
    TObjectPtr<UDataTable> MapDataTable;
    
private:
    int32 LocalCurrentMapIndex = 0;
    
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Subsystem")
    TSubclassOf<UUserWidget> OptionsWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Subsystem")
    TSubclassOf<class UParcelFriendListWidget> FriendListWidgetClass;
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Stats")
    void K2_OnHPChanged(float CurrentHP, float MaxHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Stats")
    void K2_OnStaminaChanged(float CurrentStamina, float MaxStamina);

private:
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> OptionsWidgetInstance = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<class UParcelFriendListWidget> FriendListWidgetInstance = nullptr;
    
    FTimerHandle LobbyRefreshTimerHandle;
    
    bool bStatDelegatesBound = false;

    UFUNCTION() void HandleNativeHPChanged(float CurrentHP, float MaxHP);
    UFUNCTION() void HandleNativeStaminaChanged(float CurrentStamina, float MaxStamina);
    UFUNCTION() void HandleNativeInteractionFocusChanged(AActor* NewFocusedActor);
    UFUNCTION() void HandleNativeThrowChargeChanged(bool bIsCharging, float ChargeRatio);
    
    UFUNCTION() void HandleNativeCarriedBoxChanged(ADeliveryBox* NewCarriedBox);
    UFUNCTION() void HandleNativeCarriedBoxHPChanged(float CurrentHP, float MaxHP);

    UPROPERTY()
    TWeakObjectPtr<ADeliveryBox> CachedCarriedBox;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|UI")
    void K2_OnMenuStateChanged(bool bIsOpen);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Interaction")
    void K2_OnCrosshairStateChanged(bool bIsAimingInteractable, const FText& PromptText);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Throw")
    void K2_OnThrowChargeChanged(bool bIsCharging, float ChargeRatio);
    
public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void RefreshLobbyPlayers();
    
public:
    void AddChatLog(const FString& SenderName, const FText& Message);
    
    void SetChatInputInputMode(bool bFocusChat);
    
public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void ToggleLobbyMenuExternal();
    
protected:

    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|UI")
    void K2_OnMapSelectMenuOpened();

public:

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SelectMapByIndex(int32 NewMapIndex);
 
};