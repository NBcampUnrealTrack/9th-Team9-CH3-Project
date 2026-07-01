#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StageData.generated.h"

USTRUCT(BlueprintType)
struct FStageBoxSpawnInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Config")
	FGameplayTag BoxTypeTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage Config", meta = (ClampMin = "1"))
	int32 SpawnCount = 1;
};

/** Primary Data Asset */

UCLASS(BlueprintType)
class PARCEL_KNIGHT_API UStageData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	/** 스테이지 제한 시간 (초 단위). 일단 300초로 했습니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules", meta = (ClampMin = "10.0"))
	float TimeLimit = 300.0f;
	
	/** 스테이지 클리어 점수. 일단 15점으로 해볼게요. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules", meta = (ClampMin = "1"))
	int32 TargetScore = 15;
	
	/** 스테이지에서 자동 스폰 될 택배 상자 리스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules")
	TArray<FStageBoxSpawnInfo> BoxDataList;
	
public:
	/** 고유 런타임 식별 ID를 발급해주는 오버라이드 함수 */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("StageData", GetFName());
	}
};