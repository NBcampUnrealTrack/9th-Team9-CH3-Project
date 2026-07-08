// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/PlayerStatComponent.h"
#include "Net/UnrealNetwork.h"
#include "ParcelLog.h"

// ========================= 초기화 =========================

UPlayerStatComponent::UPlayerStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPlayerStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerStatComponent, PersonalScore);
	DOREPLIFETIME(UPlayerStatComponent, SuccessCount);
	DOREPLIFETIME(UPlayerStatComponent, FailCount);
}

// ========================= 조회 =========================

int32 UPlayerStatComponent::GetPersonalScore() const
{
	return PersonalScore;
}

int32 UPlayerStatComponent::GetSuccessCount() const
{
	return SuccessCount;
}

int32 UPlayerStatComponent::GetFailCount() const
{
	return FailCount;
}

// ========================= 점수 =========================

void UPlayerStatComponent::AddScore(int32 Amount)
{
	if (!GetOwner()->HasAuthority()) return;
	PersonalScore += Amount;
}

// ========================= 배달 판정 =========================

void UPlayerStatComponent::OnDeliverySuccess()
{
	if (!GetOwner()->HasAuthority()) return;
	SuccessCount++;
	GAMERULE_LOG(Log, TEXT("[서버] 배달 성공 — SuccessCount: %d"), SuccessCount);
}

void UPlayerStatComponent::OnDeliveryFail()
{
	if (!GetOwner()->HasAuthority()) return;
	FailCount++;
	GAMERULE_LOG(Log, TEXT("[서버] 배달 실패 — FailCount: %d"), FailCount);
}
