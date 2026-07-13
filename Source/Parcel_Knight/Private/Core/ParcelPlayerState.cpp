// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelPlayerState.h"
#include "Core/PlayerStatComponent.h"
#include "Core/InventoryComponent.h"
#include "Core/ParcelGameInstance.h"

// ========================= 초기화 =========================

AParcelPlayerState::AParcelPlayerState()
{
	PlayerStatComp = CreateDefaultSubobject<UPlayerStatComponent>("PlayerStatComponent");
	InventoryComp  = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
}

void AParcelPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 초기화 — Items는 복제되어 클라이언트에 자동 전달됨
	// NOTE: 현재는 호스트(리슨서버) 전용. 원격 클라이언트의 로드아웃은
	//       PlayerController의 Server RPC로 전달받아야 함 (TODO)
	if (HasAuthority())
	{
		if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		{
			InventoryComp->InitFromGameInstance(GI);
		}
	}
}

UInventoryComponent* AParcelPlayerState::GetInventoryComponent() const
{
	return InventoryComp;
}

// ========================= 조회 =========================

int32 AParcelPlayerState::GetPersonalScore() const
{
	return PlayerStatComp->GetPersonalScore();
}

int32 AParcelPlayerState::GetSuccessCount() const
{
	return PlayerStatComp->GetSuccessCount();
}

int32 AParcelPlayerState::GetFailCount() const
{
	return PlayerStatComp->GetFailCount();
}

// ========================= 점수 =========================

void AParcelPlayerState::AddScore(int32 Amount)
{
	PlayerStatComp->AddScore(Amount);
}

// ========================= 배달 판정 =========================

void AParcelPlayerState::OnDeliverySuccess()
{
	PlayerStatComp->OnDeliverySuccess();
}

void AParcelPlayerState::OnDeliveryFail()
{
	PlayerStatComp->OnDeliveryFail();
}
