#pragma once

#include "CoreMinimal.h"
#include "UI/ParcelSessionWidget.h"
#include "ParcelRoomListWidget.generated.h"

class UScrollBox;
class UButton;
class UParcelRoomListEntry;

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
	
	void SetSelectedEntry(UParcelRoomListEntry* NewEntry);
	
protected:
	virtual void NativeConstruct() override;
	
	// UMG 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Rooms;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_RefreshRooms;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_BackToMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_JoinRoom;
	
	// 데이터 및 팩토리
	UPROPERTY(EditDefaultsOnly, Category = "RoomList|UI")
	TSubclassOf<UParcelRoomListEntry> RoomEntryClass;

	// 블루프린트
	UFUNCTION(BlueprintImplementableEvent, Category = "RoomList|Events")
	void K2_OnBackToMainMenuStarted();
	
private:
	// 버튼 클릭 핸들러
	UFUNCTION() void HandleRefreshRoomsClicked();
	UFUNCTION() void HandleBackToMenuClicked();
	UFUNCTION() void HandleJoinRoomClicked();
	
	UPROPERTY(Transient)
	TObjectPtr<UParcelRoomListEntry> CurrentlySelectedEntry = nullptr;	
};