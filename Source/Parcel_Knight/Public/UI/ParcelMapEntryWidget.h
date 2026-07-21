#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParcelMapEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UParcelLobbyHUDWidget;
struct FParcelMapStageData;

/**
 * UParcelMapEntryWidget
 * 🆕 [신규 부품 구현] 맵 선택창의 개별 레벨 슬롯 카드
 */
UCLASS()
class PARCEL_KNIGHT_API UParcelMapEntryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 낱개 카드가 들고 있어야 할 UMG 하드웨어 부품들
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> Btn_SelectThisMap;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_EntryMapName;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UImage> Img_EntryThumbnail;

private:
	int32 AssociatedMapIndex = 0;
    
	UPROPERTY(Transient)
	TObjectPtr<UParcelLobbyHUDWidget> CachedLobbyHUD = nullptr;

	UFUNCTION() void HandleSelectThisMapClicked();

public:
	// 뇌(전체 리스트 창)가 이 부품을 스폰할 때 데이터를 주입해 줄 기믹 함수
	void InitializeEntry(int32 InIndex, const FParcelMapStageData& StageData, UParcelLobbyHUDWidget* InLobbyHUD);
};