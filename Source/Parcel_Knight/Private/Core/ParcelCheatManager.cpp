// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelCheatManager.h"
#include "Core/ParcelGameMode.h"
#include "Core/ParcelGameState.h"
#include "Core/ParcelGameInstance.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ShopComponent.h"
#include "Core/ParcelPlayerState.h"
#include "Core/InventoryComponent.h"
#include "GameplayTagContainer.h"
#include "ParcelLog.h"

// ========================= 배달 =========================

void UParcelCheatManager::DebugDeliverySuccess()
{
	AParcelGameMode* GM = GetWorld()->GetAuthGameMode<AParcelGameMode>();
	if (!GM) return;
	GM->OnDeliveryCompleted(GetOuterAPlayerController(), TEXT("디버그용 상자"), 100);
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 성공 시뮬레이션"));
}

void UParcelCheatManager::DebugDeliveryFail()
{
	AParcelGameMode* GM = GetWorld()->GetAuthGameMode<AParcelGameMode>();
	if (!GM) return;
	GM->OnDeliveryFailed(GetOuterAPlayerController(), TEXT("디버그용 파손 상자"));
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 실패 시뮬레이션"));
}

void UParcelCheatManager::DebugAddScore(int32 Amount)
{
	AParcelGameState* GS = GetWorld()->GetGameState<AParcelGameState>();
	if (!GS) return;
	GS->GetTeamScoreComponent()->AddTeamScore(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 점수 추가: %d"), Amount);
}

void UParcelCheatManager::DebugPrintScore()
{
	AParcelGameState* GS = GetWorld()->GetGameState<AParcelGameState>();
	if (!GS) return;
	UTeamScoreComponent* TSC = GS->GetTeamScoreComponent();
	FString Msg = FString::Printf(TEXT("TeamScore: %d | 콤보배율: %.1fx"), TSC->GetTeamScore(), TSC->GetComboMultiplier());
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
}

// ========================= 재화 =========================

void UParcelCheatManager::DebugAddMoney(int32 Amount)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	GI->AddMoney(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 재화 추가: %d → 현재: %d"), Amount, GI->GetMoney());
}

void UParcelCheatManager::DebugPrintMoney()
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	FString Msg = FString::Printf(TEXT("보유 재화: %d"), GI->GetMoney());
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
}

// ========================= 로드아웃 =========================

void UParcelCheatManager::DebugSetLoadoutSlot(int32 SlotIndex, FString ItemTagStr)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid()) { GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr); return; }
	bool bSuccess = GI->SetLoadoutSlot(SlotIndex, Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] %s → %s"), SlotIndex, *ItemTagStr, bSuccess ? TEXT("성공") : TEXT("실패"));
}

void UParcelCheatManager::DebugClearLoadoutSlot(int32 SlotIndex)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	GI->ClearLoadoutSlot(SlotIndex);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] 초기화"), SlotIndex);
}

void UParcelCheatManager::DebugPrintLoadout()
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	const TArray<FGameplayTag>& Loadout = GI->GetLoadout();
	FString Msg = FString::Printf(TEXT("로드아웃 (최대 %d슬롯, 중복허용: %s):"),
		GI->GetMaxLoadoutSlots(), GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));
	for (int32 i = 0; i < Loadout.Num(); ++i)
		Msg += FString::Printf(TEXT("\n  [%d] %s"), i, Loadout[i].IsValid() ? *Loadout[i].ToString() : TEXT("(비어있음)"));
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Msg);
}

void UParcelCheatManager::DebugSetMaxSlots(int32 Count)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	GI->SetMaxLoadoutSlots(Count);
	GAMERULE_LOG(Log, TEXT("[콘솔] 최대 슬롯 수 → %d"), Count);
}

void UParcelCheatManager::DebugToggleDuplicateLoadout()
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	GI->ToggleDuplicateLoadout();
	GAMERULE_LOG(Log, TEXT("[콘솔] 중복 장착 허용: %s"), GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));
}

// ========================= 아이템 =========================

void UParcelCheatManager::DebugAddConsumable(FString ItemTagStr)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid()) { GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr); return; }
	GI->AddOwnedConsumable(Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 소모품 지급: %s"), *ItemTagStr);
}

void UParcelCheatManager::DebugAddCosmetic(FString ItemTagStr)
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid()) { GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr); return; }
	GI->AddOwnedCosmetic(Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 코스메틱 지급: %s"), *ItemTagStr);
}

void UParcelCheatManager::DebugPrintOwnedItems()
{
	UParcelGameInstance* GI = GetWorld()->GetGameInstance<UParcelGameInstance>();
	if (!GI) return;
	FString Msg = TEXT("=== 보유 소모품 ===");
	for (const FGameplayTag& Tag : GI->GetOwnedConsumables())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (GI->GetOwnedConsumables().IsEmpty()) Msg += TEXT("\n  (없음)");
	Msg += TEXT("\n=== 보유 코스메틱 ===");
	for (const FGameplayTag& Tag : GI->GetOwnedCosmetics())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (GI->GetOwnedCosmetics().IsEmpty()) Msg += TEXT("\n  (없음)");
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Orange, Msg);
}

void UParcelCheatManager::DebugBuyItem(FString ItemTagStr)
{
	AParcelGameState* GS = GetWorld()->GetGameState<AParcelGameState>();
	if (!GS) return;
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid()) { GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr); return; }
	bool bSuccess = GS->GetShopComponent()->BuyItem(GetOuterAPlayerController(), Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 구매 시뮬레이션 %s → %s"), *ItemTagStr, bSuccess ? TEXT("성공") : TEXT("실패 (잔액부족/중복/미등록)"));
}

void UParcelCheatManager::DebugPrintInventory()
{
	if (!GetOuterAPlayerController()) return;
	AParcelPlayerState* PS = GetOuterAPlayerController()->GetPlayerState<AParcelPlayerState>();
	if (!PS) return;
	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!Inv) return;
	FString Msg = TEXT("=== 스테이지 인벤토리 ===");
	for (const FGameplayTag& Tag : Inv->GetItems())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (Inv->GetItems().IsEmpty()) Msg += TEXT("\n  (없음)");
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Purple, Msg);
}
