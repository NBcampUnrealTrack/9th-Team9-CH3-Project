// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/DeliveryRuleComponent.h"

#include "Core/ParcelGameState.h"
#include "Core/ParcelPlayerState.h"
#include "Core/TeamScoreComponent.h"
#include "ParcelLog.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogGameRule);

// ========================= 초기화 =========================

UDeliveryRuleComponent::UDeliveryRuleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ========================= 라운드 관리 =========================

void UDeliveryRuleComponent::StartRound(float InTimeLimit, int32 InTargetScore)
{
	TimeLimit = InTimeLimit;
	TargetScore = InTargetScore;

	GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent()->InitRemainingTime(TimeLimit);

	GAMERULE_LOG(Log, TEXT("라운드 시작 시간제한: %.0f / 목표 점수: %d"), InTimeLimit, InTargetScore);

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
		TimeLimit,
		false
	);
}

void UDeliveryRuleComponent::EndRound()
{
	GetWorld()->GetTimerManager().ClearTimer(TimeUpHandle);
	GetWorld()->GetTimerManager().ClearTimer(RoundTimerHandle);
}

// ========================= 배달 판정 =========================

void UDeliveryRuleComponent::OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount)
{
	UTeamScoreComponent* TeamScoreComp = GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent();

	TeamScoreComp->OnDeliverySuccess();
	TeamScoreComp->AddTeamScore(ScoreAmount);

	if (AParcelPlayerState* PS = Deliverer->GetPlayerState<AParcelPlayerState>())
	{
		PS->OnDeliverySuccess();
		PS->AddScore(ScoreAmount);
	}

	GAMERULE_LOG(Log, TEXT("[서버] 배달 성공 — Score: %d"), ScoreAmount);
}

void UDeliveryRuleComponent::OnDeliveryFailed(APlayerController* Deliverer)
{
	GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent()->OnDeliveryFail();

	if (AParcelPlayerState* PS = Deliverer->GetPlayerState<AParcelPlayerState>())
		PS->OnDeliveryFail();

	GAMERULE_LOG(Log, TEXT("[서버] 배달 실패 — 콤보 리셋"));
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
	GAMERULE_LOG(Warning, TEXT("시간 만료"));
	UTeamScoreComponent* TeamScoreComp = GetWorld()->GetGameState<AParcelGameState>()
		->GetTeamScoreComponent();

	TeamScoreComp->AddTeamScore(-TimeUpScore);

	float Ratio = (float)TeamScoreComp->GetTeamScore() / (float)TargetScore;
	EGrade Grade;
	if      (Ratio >= 1.2f) Grade = EGrade::A;
	else if (Ratio >= 0.9f) Grade = EGrade::B;
	else if (Ratio >= 0.6f) Grade = EGrade::C;
	else                    Grade = EGrade::F;
	TeamScoreComp->SetGrade(Grade);

	if (AGameMode* GM = GetOwner<AGameMode>())
		GM->EndMatch();
}
