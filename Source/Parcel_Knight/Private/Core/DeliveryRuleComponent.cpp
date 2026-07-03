// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/DeliveryRuleComponent.h"

#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "GameFramework/GameMode.h"

UDeliveryRuleComponent::UDeliveryRuleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDeliveryRuleComponent::StartRound(float InTimeLimit)
{
	TimeLimit = InTimeLimit;
	
	GetWorld()->GetGameState<AParcelGameState>()
	->GetTeamScoreComponent()->InitRemainingTime(TimeLimit);
	
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


void UDeliveryRuleComponent::OnEverySecond()
{
	GetWorld()->GetGameState<AParcelGameState>()
	          ->GetTeamScoreComponent()->AddTeamScore(-DecreaseScore);
	GetWorld()->GetGameState<AParcelGameState>()
	          ->GetTeamScoreComponent()->DecreaseRemainingTime(1.0f);
}

void UDeliveryRuleComponent::OnTimeUp()
{
	GetWorld()->GetGameState<AParcelGameState>()
	          ->GetTeamScoreComponent()->AddTeamScore(-TimeUpScore);
	
	//GameMode의 EndMatch()를 호출해서 매치 종료
	if (AGameMode* GM = GetOwner<AGameMode>())
	{
		GM->EndMatch();
	}
}
