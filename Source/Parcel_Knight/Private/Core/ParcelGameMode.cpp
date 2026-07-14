// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameMode.h"

#include "Character/ParcelCharacter.h"
#include "Core/DeliveryRuleComponent.h"
#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ParcelPlayerController.h"
#include "Core/ParcelPlayerState.h"
#include "Core/ParcelGameInstance.h"
#include "Core/ShopComponent.h"
#include "Core/InventoryComponent.h"
#include "Delivery/StageData.h"
#include "ParcelLog.h"
#include "Core/ParcelGameInstance.h"
#include "Delivery/StageData.h"
#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBoxSpawner.h"
#include "EngineUtils.h"

AParcelGameMode::AParcelGameMode()
{
	GameStateClass = AParcelGameState::StaticClass();
	PlayerStateClass = AParcelPlayerState::StaticClass();
	PlayerControllerClass = AParcelPlayerController::StaticClass();
	DefaultPawnClass = AParcelCharacter::StaticClass();

	DeliveryRuleComp = CreateDefaultSubobject<UDeliveryRuleComponent>("DeliveryRuleComponent");

}

void AParcelGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void AParcelGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void AParcelGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	GAMERULE_LOG(Log, TEXT("Match Started — StartRound 호출"));
	
	if (DefaultStageData)
	{
		StartRound(DefaultStageData);
		GAMERULE_LOG(Log, TEXT("성공적으로 StageData를 로드하여 모든 시스템을 활성화했습니다."));
	}
	else
	{
		GAMERULE_LOG(Error, TEXT("DefaultStageData가 지정되지 않았습니다. BP_ParcelGameMode에서 할당해주세요."));
	}
}

void AParcelGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	EndRound();
}

void AParcelGameMode::StartRound(UStageData* InStageData)
{
	if (!InStageData) return;
	
	// 타이머 시작
	if (DeliveryRuleComp)
	{
		DeliveryRuleComp->StartRound(InStageData->TimeLimit, InStageData->TargetScore);
	}
	
	// 서브시스템 초기화 + 월드 스폰 포인트 추적
	if (GetWorld())
	{
		UDeliverySubsystem* DeliverySubsystem = GetWorld()->GetSubsystem<UDeliverySubsystem>();
		if (DeliverySubsystem)
		{
			// 에디터에서 배치해놓은 박스 스포너 액터 추적 및 서브시스템 초기화
			DeliverySubsystem->InitializeStage(InStageData);
		}
	}
}

void AParcelGameMode::EndRound()
{
	DeliveryRuleComp->EndRound();
}

void AParcelGameMode::OnDeliveryCompleted(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryCompleted(Deliverer, ScoreAmount);
}

void AParcelGameMode::OnDeliveryFailed(APlayerController* Deliverer, int32 ScoreAmount)
{
	DeliveryRuleComp->OnDeliveryFailed(Deliverer, ScoreAmount);
}

// ========================= 콘솔 명령어 =========================

void AParcelGameMode::DebugDeliverySuccess()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		OnDeliveryCompleted(PC, 100);
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 성공 시뮬레이션"));
}

void AParcelGameMode::DebugDeliveryFail()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		OnDeliveryFailed(PC);
	GAMERULE_LOG(Log, TEXT("[콘솔] 배달 실패 시뮬레이션"));
}

void AParcelGameMode::DebugAddScore(int32 Amount)
{
	AParcelGameState* GS = GetGameState<AParcelGameState>();
	if (!GS) return;

	GS->GetTeamScoreComponent()->AddTeamScore(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 점수 추가: %d"), Amount);
}

void AParcelGameMode::DebugPrintScore()
{
	AParcelGameState* GS = GetGameState<AParcelGameState>();
	if (!GS) return;

	UTeamScoreComponent* TSC = GS->GetTeamScoreComponent();
	FString Msg = FString::Printf(TEXT("TeamScore: %d | 콤보배율: %.1fx"),
		TSC->GetTeamScore(), TSC->GetComboMultiplier());

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Msg);
}


// ========================= 로드아웃 콘솔 명령어 =========================

void AParcelGameMode::DebugSetLoadoutSlot(int32 SlotIndex, FString ItemTagStr)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid())
	{
		GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr);
		return;
	}

	bool bSuccess = GI->SetLoadoutSlot(SlotIndex, Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] %s → %s"),
		SlotIndex, *ItemTagStr, bSuccess ? TEXT("성공") : TEXT("실패"));
}

void AParcelGameMode::DebugClearLoadoutSlot(int32 SlotIndex)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->ClearLoadoutSlot(SlotIndex);
	GAMERULE_LOG(Log, TEXT("[콘솔] 로드아웃 슬롯[%d] 초기화"), SlotIndex);
}

void AParcelGameMode::DebugPrintLoadout()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	const TArray<FGameplayTag>& Loadout = GI->GetLoadout();
	FString Msg = FString::Printf(TEXT("로드아웃 (최대 %d슬롯, 중복허용: %s):"),
		GI->GetMaxLoadoutSlots(), GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));

	for (int32 i = 0; i < Loadout.Num(); ++i)
	{
		FString SlotStr = Loadout[i].IsValid()
			? Loadout[i].ToString()
			: TEXT("(비어있음)");
		Msg += FString::Printf(TEXT("\n  [%d] %s"), i, *SlotStr);
	}

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Cyan, Msg);
}

void AParcelGameMode::DebugSetMaxSlots(int32 Count)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->SetMaxLoadoutSlots(Count);
	GAMERULE_LOG(Log, TEXT("[콘솔] 최대 슬롯 수 → %d"), Count);
}

void AParcelGameMode::DebugToggleDuplicateLoadout()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->ToggleDuplicateLoadout();
	GAMERULE_LOG(Log, TEXT("[콘솔] 중복 장착 허용: %s"),
		GI->IsAllowDuplicateLoadout() ? TEXT("ON") : TEXT("OFF"));
}

// ========================= 재화 콘솔 명령어 =========================

// [재화] GameInstance에 위임
void AParcelGameMode::DebugAddMoney(int32 Amount)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	GI->AddMoney(Amount);
	GAMERULE_LOG(Log, TEXT("[콘솔] 재화 추가: %d → 현재: %d"), Amount, GI->GetMoney());
}

// ========================= 아이템 콘솔 명령어 =========================

void AParcelGameMode::DebugAddConsumable(FString ItemTagStr)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid())
	{
		GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr);
		return;
	}

	GI->AddOwnedConsumable(Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 소모품 지급: %s"), *ItemTagStr);
}

void AParcelGameMode::DebugAddCosmetic(FString ItemTagStr)
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid())
	{
		GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr);
		return;
	}

	GI->AddOwnedCosmetic(Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 코스메틱 지급: %s"), *ItemTagStr);
}

void AParcelGameMode::DebugPrintOwnedItems()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	// 소모품 출력
	FString Msg = TEXT("=== 보유 소모품 ===");
	for (const FGameplayTag& Tag : GI->GetOwnedConsumables())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (GI->GetOwnedConsumables().IsEmpty())
		Msg += TEXT("\n  (없음)");

	// 코스메틱 출력
	Msg += TEXT("\n=== 보유 코스메틱 ===");
	for (const FGameplayTag& Tag : GI->GetOwnedCosmetics())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (GI->GetOwnedCosmetics().IsEmpty())
		Msg += TEXT("\n  (없음)");

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Orange, Msg);
}

void AParcelGameMode::DebugBuyItem(FString ItemTagStr)
{
	AParcelGameState* GS = GetGameState<AParcelGameState>();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!GS || !PC) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagStr), false);
	if (!Tag.IsValid())
	{
		GAMERULE_LOG(Warning, TEXT("[콘솔] 유효하지 않은 태그: %s"), *ItemTagStr);
		return;
	}

	bool bSuccess = GS->GetShopComponent()->BuyItem(PC, Tag);
	GAMERULE_LOG(Log, TEXT("[콘솔] 구매 시뮬레이션 %s → %s"),
		*ItemTagStr, bSuccess ? TEXT("성공") : TEXT("실패 (잔액부족/중복/미등록)"));
}

void AParcelGameMode::DebugPrintInventory()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	AParcelPlayerState* PS = PC->GetPlayerState<AParcelPlayerState>();
	if (!PS) return;

	UInventoryComponent* Inv = PS->GetInventoryComponent();
	if (!Inv) return;

	FString Msg = TEXT("=== 스테이지 인벤토리 ===");
	for (const FGameplayTag& Tag : Inv->GetItems())
		Msg += FString::Printf(TEXT("\n  %s"), *Tag.ToString());
	if (Inv->GetItems().IsEmpty())
		Msg += TEXT("\n  (없음)");

	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Purple, Msg);
}

// ========================= 재화 콘솔 명령어 =========================

// [재화] 현재 보유 재화를 로그와 화면에 출력
void AParcelGameMode::DebugPrintMoney()
{
	UParcelGameInstance* GI = GetGameInstance<UParcelGameInstance>();
	if (!GI) return;

	FString Msg = FString::Printf(TEXT("보유 재화: %d"), GI->GetMoney());
	GAMERULE_LOG(Log, TEXT("[콘솔] %s"), *Msg);
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
}