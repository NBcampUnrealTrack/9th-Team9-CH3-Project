#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DeliveryTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDelivery, Log, All);

class UStaticMesh;

USTRUCT(BlueprintType)
struct FBoxData : public FTableRowBase
{
	GENERATED_BODY()

	/** 택배 고유 종류 태그. 에디터 내에서 RowName과 일치시켜 관리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	FGameplayTag BoxTypeTag;

	/** UI 및 화면에 표시할 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	FString DisplayName;

	/** 배송 성공 시 기본 점수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	int32 BaseScore = 1;

	/** 물리 연산 및 무게감에 적용할 무게 값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	float Weight = 1.0f;

	/** 해당 상자를 들었을 때 플레이어에게 적용할 이동 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	float MoveSpeedMultiplier = 1.0f;

	/** 파손 판정을 유도할 물리 충격량 임계값. 파손주의 박스는 임계값을 150.0f 정도로 낮게. 튼튼한 택배는 800.0f 정도로*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	float DamageThreshold = 500.0f;

	/** 파손되었을 때 차감될 점수 페널티 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	int32 DamagePenalty = -1;

	/** 택배 상자의 외형 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery Box Data")
	TObjectPtr<UStaticMesh> BoxMeshAsset;
};