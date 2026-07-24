#pragma once

#include "CoreMinimal.h"
#include "UI/ParcelSessionWidget.h"
#include "ParcelRoomListWidget.generated.h"

class UScrollBox;
class UButton;
class UParcelRoomListEntry;
class USoundBase;

/**
 * UParcelRoomListWidget
 * 담당자 : JYW
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelRoomListWidget : public UParcelSessionWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RoomList")
	void RefreshRoomList();

	/** Clears the visible room entries without starting a new search. */
	UFUNCTION(BlueprintCallable, Category = "RoomList")
	void ClearRoomListEntries();
	
	void SetSelectedEntry(UParcelRoomListEntry* NewEntry);
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "MainMenu|Sound")
	TObjectPtr<USoundBase> ButtonClickSound;
	
	// UMG 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Rooms;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_RefreshRooms;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_BackToMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_JoinRoom;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_CreateRoom;
	
	// 데이터 및 팩토리
	UPROPERTY(EditDefaultsOnly, Category = "RoomList|UI")
	TSubclassOf<UParcelRoomListEntry> RoomEntryClass;

	// 블루프린트
	UFUNCTION(BlueprintImplementableEvent, Category = "RoomList|Events")
	void K2_OnBackToMainMenuStarted();
	
private:
	void PlayButtonClickSound();
	
	UFUNCTION()
	void HandleSessionSearchStarted();

	// 버튼 클릭 핸들러
	UFUNCTION() void HandleRefreshRoomsClicked();
	UFUNCTION() void HandleBackToMenuClicked();
	UFUNCTION() void HandleJoinRoomClicked();
	UFUNCTION() void HandleCreateRoomClicked();
	
	UPROPERTY(Transient)
	TObjectPtr<UParcelRoomListEntry> CurrentlySelectedEntry = nullptr;	
};
