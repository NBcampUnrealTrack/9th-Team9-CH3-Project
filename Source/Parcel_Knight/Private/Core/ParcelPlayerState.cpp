// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelPlayerState.h"
#include "Core/PlayerStatComponent.h"
#include "ParcelLog.h"
#include "Core/InventoryComponent.h"
#include "Core/CustomizationComponent.h"
#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameMode.h"
#include "Core/ParcelPlayerController.h"
#include "Core/RespawnComponent.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelGameplayTags.h"
#include "Net/UnrealNetwork.h"

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

	// The listen-server's local player may read this process's profile directly.
	// Remote clients submit their profile through ParcelPlayerController so the server
	// can validate every entry before it reaches the replicated inventory.
	if (HasAuthority())
	{
		if (UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>())
		{
			APlayerController* OwningController = Cast<APlayerController>(GetOwner());
			if (OwningController && OwningController->IsLocalController())
			{
				InventoryComp->InitFromGameInstance(GI);

				// Only the listen-server's own local player may seed customization directly
				// from this process's GameInstance. Remote clients submit their own choice
				// through ParcelPlayerController::Server_SubmitCustomization instead —
				// otherwise every PlayerState would end up wearing the host's skin.
				CustomizationComp->InitFromGameInstance(GI);
			}
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
	if (!HasAuthority()) return;

	PlayerStatComp->OnDeath();
	Client_StartSpectating();

	if (AParcelGameMode* GM = GetWorld()->GetAuthGameMode<AParcelGameMode>())
	{
		URespawnComponent* RC = GM->GetRespawnComponent();
		AController* PC = GetPlayerController();
		if (!PC)
		{
			GAMERULE_LOG(Warning, TEXT("[부활] GetPlayerController() null — 타이머 미설정"));
			return;
		}
		float ReduceSeconds = 0.f;
		if (InventoryComp)
		{
			ReduceSeconds = InventoryComp->GetPassiveEffectSum(ParcelGameplayTags::Effect_Stat_RespawnTimeReduction);
		}
		const float FinalDelay = FMath::Max(0.f, RC->ReviveDelay - ReduceSeconds);

		// [UI] [Server] : 로컬 플레이어 컨트롤러를 찾아서 클라이언트 RPC 호출 — 실제 부활 시간을 그대로 넘겨서
		// 사망 화면 카운트다운이 하드코딩된 값이 아니라 감소분까지 반영된 값으로 표시되게 한다.
		if (AParcelPlayerController* ParcelPC = Cast<AParcelPlayerController>(PC))
		{
			ParcelPC->Client_NotifyDeath(FMath::CeilToInt(FinalDelay));
		}

		RC->RespawnPlayerAfterDelay(PC, FinalDelay);
		GAMERULE_LOG(Log, TEXT("[부활] 타이머 설정 (%.1f초 후, 기본 %.1f초 - 감소 %.1f초)"), FinalDelay, RC->ReviveDelay, ReduceSeconds);
	}
}

void AParcelPlayerState::Client_StartSpectating_Implementation()
{
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

void AParcelPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AParcelPlayerState, bIsHostPlayer);
}