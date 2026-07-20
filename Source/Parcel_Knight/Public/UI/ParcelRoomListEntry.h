#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelRoomListEntry.generated.h"

class UTextBlock;
class USessionSubsystem;

/**
 * UParcelRoomListEntry
 * 담당자 : JYW
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelRoomListEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RoomList")
	void InitializeEntry(int32 InSessionIndex, UParcelRoomListWidget* InOwnerList);
	
	UFUNCTION(BlueprintCallable, Category = "RoomList")
	void SelectThisRoom();
	
	UFUNCTION(BlueprintCallable, Category = "RoomList")
	void JoinThisRoom();
	
	FORCEINLINE int32 GetSessionIndex() const { return MySessionIndex; }
	
	UFUNCTION(BlueprintImplementableEvent, Category = "RoomList|Visual")
	void K2_SetHighlightState(bool bIsHighlighted);

protected:
	virtual void NativeConstruct() override;

	// UMG 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_RoomName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_PlayerCount;

private:
	int32 MySessionIndex = -1;

	UPROPERTY(Transient)
	TObjectPtr<UParcelRoomListWidget> OwnerRoomListWidget;
};