#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ItemData.generated.h"

/**
 * 상점 아이템 데이터 Row 구조체
 * DataTable의 행 정의 — 에디터에서 아이템 추가 시 이 구조체를 사용한다.
 * ItemTag 계층으로 타입을 구분한다 (Item.Consumable.* / Item.Cosmetic.*)
 *
 * 담당자: 한수현
 */
USTRUCT(BlueprintType)
struct PARCEL_KNIGHT_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 식별자 — 타입도 겸함 (Item.Consumable.Cart, Item.Cosmetic.Skin.Blue 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag ItemTag;

	// 상점 구매 가격
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Price = 0;

	// 상점/인벤토리에 표시될 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// 아이콘 — 소프트 레퍼런스로 필요할 때만 로드
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	// 영구 사용 가능 여부 — true면 사용해도 인벤토리에서 제거되지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsPermanent = false;
};
