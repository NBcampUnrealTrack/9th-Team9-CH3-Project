// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/DeliveryRuleComponent.h"

#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerState.h"
#include "Core/TeamScoreComponent.h"
#include "Delivery/StageData.h"
#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBoxSpawner.h"
#include "ParcelLog.h"
#include "EngineUtils.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogGameRule);

// ========================= 초기화 =========================

UDeliveryRuleComponent::UDeliveryRuleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ========================= 라운드 관리 =========================

void UDeliveryRuleComponent::StartRound(UStageData* InStageData)
{
	if (!InStageData) return;
	CurrentStageData = InStageData;

	GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent()->InitRemainingTime(CurrentStageData->TimeLimit);

	GAMERULE_LOG(Log, TEXT("라운드 시작 시간제한: %.0f / 목표 점수: %d"), CurrentStageData->TimeLimit, CurrentStageData->TargetScore);

	// 1초마다 반복 — 시간 경과 점수 감소
	GetWorld()->GetTimerManager().SetTimer(
		RoundTimerHandle,
		this,
		&UDeliveryRuleComponent::OnEverySecond,
		1.0f,
		true
	);

	// TimeLimit 후 단발 — 시간 만료 처리
	GetWorld()->GetTimerManager().SetTimer(
		TimeUpHandle,
		this,
		&UDeliveryRuleComponent::OnTimeUp,
		CurrentStageData->TimeLimit,
		false
	);

	// 서브시스템 초기화
	UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>();
	if (DeliverySubsystem)
	{
		DeliverySubsystem->InitializeStage(CurrentStageData);
	}
}

void UDeliveryRuleComponent::EndRound()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeUpHandle);
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
}

// ========================= 배달 판정 =========================

void UDeliveryRuleComponent::OnDeliveryCompleted(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount)
{
	UTeamScoreComponent* TeamScoreComp = GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent();

	if (TeamScoreComp)
	{
		TeamScoreComp->OnDeliverySuccess();
		TeamScoreComp->AddTeamScore(ScoreAmount);
	}

	if (Deliverer)
	{
		if (AParcelPlayerState* PS = Deliverer->GetPlayerState<AParcelPlayerState>())
		{
			PS->OnDeliverySuccess();
			PS->AddScore(ScoreAmount);
		}
	}
	
	if (Deliverer)
	{
		FString PlayerName = Deliverer->PlayerState ? Deliverer->PlayerState->GetPlayerName() : TEXT("알 수 없는 배달원");
		if (AParcelGameState* ParcelGS = GetWorld()->GetGameState<AParcelGameState>())
		{
			ParcelGS->Multicast_NotifyDeliveryLog(PlayerName, BoxName, true);
		}
	}

	GAMERULE_LOG(Log, TEXT("[서버] 배달 성공 — Score: %d"), ScoreAmount);
}

void UDeliveryRuleComponent::OnDeliveryFailed(APlayerController* Deliverer, const FString& BoxName, int32 ScoreAmount)
{
	UTeamScoreComponent* TeamScoreComp = GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent();

	if (TeamScoreComp)
	{
		TeamScoreComp->OnDeliveryFail();
		if (ScoreAmount != 0)
		{
			TeamScoreComp->AddTeamScore(ScoreAmount); // 오배송 페널티 감점 적용
		}
	}

	if (Deliverer)
	{
		if (AParcelPlayerState* PS = Deliverer->GetPlayerState<AParcelPlayerState>())
		{
			PS->OnDeliveryFail();
			if (ScoreAmount != 0)
			{
				PS->AddScore(ScoreAmount);
			}
		}
	}
	
	if (Deliverer)
	{
		FString PlayerName = Deliverer->PlayerState ? Deliverer->PlayerState->GetPlayerName() : TEXT("알 수 없는 배달원");
		if (AParcelGameState* ParcelGS = GetWorld()->GetGameState<AParcelGameState>())
		{
			ParcelGS->Multicast_NotifyDeliveryLog(PlayerName, BoxName, false);
		}
	}

	GAMERULE_LOG(Log, TEXT("[서버] 배달 실패 — 콤보 리셋 및 페널티 점수: %d"), ScoreAmount);
}

// ========================= 내부 타이머 =========================

void UDeliveryRuleComponent::OnEverySecond()
{
	GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent()->AddTeamScore(-DecreaseScore);
	GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent()->DecreaseRemainingTime(1.0f);
	GAMERULE_LOG(Verbose, TEXT("매초 점수 감소"));
}

void UDeliveryRuleComponent::OnTimeUp()
{
	GAMERULE_LOG(Warning, TEXT("시간 만료 — 최종 등급 산정"));
	UTeamScoreComponent* TeamScoreComp = GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent();

	// 제한시간 내 획득 점수 / 기준 점수 비율로 등급 결정
	float Ratio = (float)TeamScoreComp->GetTeamScore() / (float)CurrentStageData->TargetScore;
	FGameplayTag Grade;
	if      (Ratio >= CurrentStageData->GradeA_Threshold) Grade = FGameplayTag::RequestGameplayTag("Grade.A");
	else if (Ratio >= CurrentStageData->GradeB_Threshold) Grade = FGameplayTag::RequestGameplayTag("Grade.B");
	else if (Ratio >= CurrentStageData->GradeC_Threshold) Grade = FGameplayTag::RequestGameplayTag("Grade.C");
	else                                                   Grade = FGameplayTag::RequestGameplayTag("Grade.F");
	TeamScoreComp->SetGrade(Grade);

	// B 이상이면 스테이지 클리어 — GameInstance에 최고 기록 갱신 요청 (서버에서만 실행되므로 즉시 저장)
	static const FGameplayTag GradeA = FGameplayTag::RequestGameplayTag("Grade.A");
	static const FGameplayTag GradeB = FGameplayTag::RequestGameplayTag("Grade.B");
	if (Grade == GradeA || Grade == GradeB)
	{
		if (UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>())
		{
			GI->UpdateMaxClearedStage(CurrentStageData->StageIndex);
		}
	}

	// 등급별 보상 금액 결정 후 전원 지급
	int32 Reward = 0;
	if      (Grade == FGameplayTag::RequestGameplayTag("Grade.A")) Reward = CurrentStageData->RewardMoney_A;
	else if (Grade == FGameplayTag::RequestGameplayTag("Grade.B")) Reward = CurrentStageData->RewardMoney_B;
	else if (Grade == FGameplayTag::RequestGameplayTag("Grade.C")) Reward = CurrentStageData->RewardMoney_C;
	else                                                            Reward = CurrentStageData->RewardMoney_F;

	GAMERULE_LOG(Log, TEXT("보상 지급 — %s: %d"), *Grade.ToString(), Reward);

	for (APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
	{
		if (AParcelPlayerState* PPS = Cast<AParcelPlayerState>(PS))
			PPS->Client_GrantReward(Reward);
	}

	if (AGameMode* GM = GetOwner<AGameMode>())
		GM->EndMatch();
}
