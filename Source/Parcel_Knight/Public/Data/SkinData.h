#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "SkinData.generated.h"

/**
 * 캐릭터 스킨 데이터 Row 구조체
 * DataTable의 행 정의 — 에디터에서 데이터 테이블 생성 시 이 구조체를 사용한다.
 *
 * 담당자: 한수현
 */
USTRUCT(BlueprintType)
struct PARCEL_KNIGHT_API FSkinData : public FTableRowBase
{
	GENERATED_BODY()

	// 스킨 식별 태그 (Item.Cosmetic.Skin.Red, Item.Cosmetic.Skin.Gold 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	FGameplayTag SkinTag;

	// 상점/커스텀 UI 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	FText DisplayName;

	// 스킨 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	FText Description;

	// UI 아이콘 — 소프트 레퍼런스로 필요할 때만 로드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	TSoftObjectPtr<UTexture2D> Icon;

	// 캐릭터 메시에 입힐 머티리얼 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skin")
	TSoftObjectPtr<UMaterialInterface> SkinMaterial;
};
