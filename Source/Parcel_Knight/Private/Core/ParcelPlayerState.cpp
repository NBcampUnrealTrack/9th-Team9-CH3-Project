// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelPlayerState.h"
#include "Core/PlayerStatComponent.h"
#include "Core/InventoryComponent.h"
#include "Core/CustomizationComponent.h"
#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameMode.h"
#include "Core/RespawnComponent.h"

// ========================= 초기화 =========================

AParcelPlayerState::AParcelPlayerState()
{
	PlayerStatComp    = CreateDefaultSubobject<UPlayerStatComponent>("PlayerStatComponent");
	InventoryComp     = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	CustomizationComp = CreateDefaultSubobject<UCustomizationComponent>("CustomizationComponent");
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
			CustomizationComp->InitFromGameInstance(GI);
		}
	}
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

int32 AParcelPlayerState::GetDeathCount() const
{
	return PlayerStatComp->GetDeathCount();
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

// ========================= 사망 / 부활 =========================

void AParcelPlayerState::HandleDeath()
{
	PlayerStatComp->OnDeath();

	if (AParcelGameMode* GM = GetWorld()->GetAuthGameMode<AParcelGameMode>())
	{
		URespawnComponent* RC = GM->GetRespawnComponent();
		RC->RespawnPlayerAfterDelay(GetPlayerController(), RC->ReviveDelay);
	}
}

void AParcelPlayerState::Client_GrantReward_Implementation(int32 RewardAmount)
{
	if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		GI->AddMoney(RewardAmount);
}

// ========================= 커스터마이징 =========================

UInventoryComponent* AParcelPlayerState::GetInventoryComponent() const
{
	return InventoryComp;
}

UCustomizationComponent* AParcelPlayerState::GetCustomizationComponent() const
{
	return CustomizationComp;
}

void AParcelPlayerState::EquipSkin(FGameplayTag SkinTag)
{
	if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		GI->EquipSkin(SkinTag);

	CustomizationComp->EquipSkin(SkinTag);
}

void AParcelPlayerState::EquipTitle(FGameplayTag TitleTag)
{
	if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		GI->EquipTitle(TitleTag);

	CustomizationComp->EquipTitle(TitleTag);
}

void AParcelPlayerState::EquipEffect(FGameplayTag EffectTag)
{
	if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		GI->EquipEffect(EffectTag);

	CustomizationComp->EquipEffect(EffectTag);
}
