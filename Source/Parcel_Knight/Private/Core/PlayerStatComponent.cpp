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
	float comboscore = 1.0f;
	//최대 10회까지 콤보 보너스 축적 가능, 스코어 배율 1x ~ 2x
	comboscore += FMath::Clamp(ComboCount*0.1, 0,1); 
	if (!GetOwner()->HasAuthority()) return;
	PersonalScore += Amount * comboscore;
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
