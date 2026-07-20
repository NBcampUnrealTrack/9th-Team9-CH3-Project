#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelMapSelectWidget.generated.h"

class UScrollBox;
class UButton;
class UDataTable;
class UParcelLobbyHUDWidget;
class UParcelMapEntryWidget;

/**
 * UParcelMapSelectWidget
 * 🆕 [신규 부품 구현] 데이터 테이블을 파싱하여 스크롤 뷰를 빌드업하는 전체 메인 위젯
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelMapSelectWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 동적으로 담아낼 주머니 스크롤 박스와 닫기 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UScrollBox> ScrollBox_MapList;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_CloseMapMenu;

	// 에디터에서 낱개 항목으로 사용할 WBP_MapEntry 위젯을 지정할 슬롯
	UPROPERTY(EditDefaultsOnly, Category = "MapSelect|UI")
	TSubclassOf<UParcelMapEntryWidget> MapEntryClass;

private:
	UFUNCTION() void HandleCloseClicked();

public:
	UFUNCTION(BlueprintCallable, Category = "MapSelect|UI")
	void BuildMapListPopulate(UDataTable* TargetDataTable, UParcelLobbyHUDWidget* OwnerHUD);
};