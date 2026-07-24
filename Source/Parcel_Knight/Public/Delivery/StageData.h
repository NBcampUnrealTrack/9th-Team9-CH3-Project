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

	// 이 스테이지의 순서 번호 (1, 2, 3 ...) — 잠금 해제 판정에 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules", meta = (ClampMin = "1"))
	int32 StageIndex = 1;

	// 스테이지 제한 시간 (초 단위). 일단 300초로 했습니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules", meta = (ClampMin = "10.0"))
	float TimeLimit = 300.0f;

	// 등급 산정 기준 점수 — 최종점수/기준점수 비율로 Grade.A~F 결정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules", meta = (ClampMin = "1"))
	int32 TargetScore = 15;

	// 등급 산정 비율 기준 (최종점수 / 기준점수) — 이상이면 해당 등급
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules")
	float GradeA_Threshold = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules")
	float GradeB_Threshold = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules")
	float GradeC_Threshold = 0.6f;

	// 등급별 보상 금액 — 라운드 종료 시 전원 지급
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	int32 RewardMoney_A = 5000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	int32 RewardMoney_B = 3000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	int32 RewardMoney_C = 1000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rewards")
	int32 RewardMoney_F = 0;

	// 스테이지에서 자동 스폰 될 택배 상자 리스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Rules")
	TArray<FStageBoxSpawnInfo> BoxDataList;

public:
	// 고유 런타임 식별 ID를 발급해주는 오버라이드 함수
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("StageData", GetFName());
	}

	// 이 스테이지에서 사용할 택배 상자 데이터 테이블 (에디터에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Resources")
	TObjectPtr<UDataTable> BoxDataTable;

};
