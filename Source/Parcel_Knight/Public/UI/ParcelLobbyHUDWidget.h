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
    
    virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    
    void SetupLobbyLayout();
    
    void SetMenuVisibleState(bool bNewState);
    
    void SetChatInputInputMode(bool bFocusChat);
    
    UFUNCTION() void HandleOptionsClicked();
    UFUNCTION() void HandleFriendsClicked();
    UFUNCTION() void HandleLeaveLobbyClicked();
    UFUNCTION() void HandleSelectMapClicked();
    UFUNCTION() void HandleActionOrStartClicked();
    UFUNCTION() void HandleChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    UFUNCTION() void HandleOnSessionDestroyComplete(bool bWasSuccessful);

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
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|UI")
    void K2_OnMenuStateChanged(bool bIsOpen);

public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void RefreshLobbyPlayers();
    
public:
    void AddChatLog(const FString& SenderName, const FText& Message);
    
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Data")
    TObjectPtr<UDataTable> MapDataTable;
    
    void HandleOnLobbyMapChanged(int32 NewMapIndex);

private:
    int32 LocalCurrentMapIndex = 0;
    
protected:
    
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Subsystem")
    TSubclassOf<UUserWidget> OptionsWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Subsystem")
    TSubclassOf<class UParcelFriendListWidget> FriendListWidgetClass;

private:
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> OptionsWidgetInstance = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<class UParcelFriendListWidget> FriendListWidgetInstance = nullptr;
};