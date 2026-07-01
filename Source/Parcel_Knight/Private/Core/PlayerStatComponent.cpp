// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/PlayerStatComponent.h"
#include "Net/UnrealNetwork.h"

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
	DOREPLIFETIME(UPlayerStatComponent, ComboCount);
}

// ─────────────────────────────────────
// 조회
// ─────────────────────────────────────

int32 UPlayerStatComponent::GetPersonalScore() const
{
	return PersonalScore;
}

int32 UPlayerStatComponent::GetComboCount() const
{
	return ComboCount;
}

int32 UPlayerStatComponent::GetSuccessCount() const
{
	return SuccessCount;
}

// ─────────────────────────────────────
// 점수 및 콤보 처리
// ─────────────────────────────────────

void UPlayerStatComponent::AddScore(int32 Amount)
{
	if (!GetOwner()->HasAuthority()) return;
	PersonalScore += Amount;
	//TODO: 차후 콤보 관련한 배수 혹은 추가 점수 로직 필요
}

void UPlayerStatComponent::OnDeliverySuccess()
{
	if (!GetOwner()->HasAuthority()) return;
	SuccessCount++;
	ComboCount++;
}

void UPlayerStatComponent::OnDeliveryFail()
{
	if (!GetOwner()->HasAuthority()) return;
	ComboCount = 0;
	//TODO: 점수 감소 로직 및 콤보 관련 이야기 필요
}
