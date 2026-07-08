// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelPlayerState.h"
#include "Core/PlayerStatComponent.h"

AParcelPlayerState::AParcelPlayerState()
{
	PlayerStatComp = CreateDefaultSubobject<UPlayerStatComponent>("PlayerStatComponent");
}

//---------------Get 함수 + 점수 증가------------------------------------------------


int32 AParcelPlayerState::GetPersonalScore() const
{
	return PlayerStatComp->GetPersonalScore();
}

int32 AParcelPlayerState::GetComboCount() const
{
	return PlayerStatComp->GetComboCount();
}

int32 AParcelPlayerState::GetSuccessCount() const
{
	return PlayerStatComp->GetSuccessCount();
}


void AParcelPlayerState::AddScore(int32 Amount)
{
	PlayerStatComp->AddScore(Amount);
}

//---------------배송 성공,실패 판정------------------------------------------------

void AParcelPlayerState::OnDeliverySuccess()
{
	PlayerStatComp->OnDeliverySuccess();
}

void AParcelPlayerState::OnDeliveryFail()
{
	PlayerStatComp->OnDeliveryFail();
}
