#include "Core/ParcelGameInstance.h"
#include "Core/ParcelSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UParcelGameInstance::SaveSlotName = TEXT("PlayerSaveSlot");

// ========================= 초기화 =========================

void UParcelGameInstance::Init()
{
	Super::Init();
	LoadData();
}

// ========================= 저장 / 불러오기 =========================

void UParcelGameInstance::SaveData()
{
	UParcelSaveGame* SaveGame = Cast<UParcelSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UParcelSaveGame::StaticClass()));

	SaveGame->Money            = Money;
	SaveGame->OwnedConsumables = OwnedConsumables;
	SaveGame->OwnedCosmetics   = OwnedCosmetics;
	SaveGame->EquippedLoadout  = EquippedLoadout;

	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}

void UParcelGameInstance::LoadData()
{
	UParcelSaveGame* SaveGame = Cast<UParcelSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

	// 저장 파일이 없으면 기본값으로 초기화
	if (!SaveGame) return;

	Money            = SaveGame->Money;
	OwnedConsumables = SaveGame->OwnedConsumables;
	OwnedCosmetics   = SaveGame->OwnedCosmetics;
	EquippedLoadout  = SaveGame->EquippedLoadout;
}

// ========================= 재화 =========================

int32 UParcelGameInstance::GetMoney() const
{
	return Money;
}

void UParcelGameInstance::AddMoney(int32 Amount)
{
	Money += Amount;
	SaveData();
}

bool UParcelGameInstance::SpendMoney(int32 Amount)
{
	if (Money < Amount) return false;
	Money -= Amount;
	SaveData();
	return true;
}

// ========================= 로드아웃 =========================

const TArray<FGameplayTag>& UParcelGameInstance::GetLoadout() const
{
	return EquippedLoadout;
}

bool UParcelGameInstance::SetLoadoutSlot(int32 SlotIndex, FGameplayTag ItemTag)
{
	if (SlotIndex < 0 || SlotIndex >= MaxLoadoutSlots) return false;
	if (!OwnedConsumables.Contains(ItemTag)) return false;
	if (!bAllowDuplicateLoadout && EquippedLoadout.Contains(ItemTag)) return false;

	// 슬롯이 아직 없으면 빈 태그로 채워서 인덱스 확보
	while (EquippedLoadout.Num() <= SlotIndex)
		EquippedLoadout.Add(FGameplayTag::EmptyTag);

	EquippedLoadout[SlotIndex] = ItemTag;
	SaveData();
	return true;
}

void UParcelGameInstance::ClearLoadoutSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= EquippedLoadout.Num()) return;

	EquippedLoadout[SlotIndex] = FGameplayTag::EmptyTag;
	SaveData();
}

int32 UParcelGameInstance::GetMaxLoadoutSlots() const
{
	return MaxLoadoutSlots;
}

void UParcelGameInstance::SetMaxLoadoutSlots(int32 Count)
{
	MaxLoadoutSlots = FMath::Max(0, Count);
	// 슬롯 수가 줄어들면 초과 슬롯 제거
	if (EquippedLoadout.Num() > MaxLoadoutSlots)
		EquippedLoadout.SetNum(MaxLoadoutSlots);
	SaveData();
}

void UParcelGameInstance::ToggleDuplicateLoadout()
{
	bAllowDuplicateLoadout = !bAllowDuplicateLoadout;
}

bool UParcelGameInstance::IsAllowDuplicateLoadout() const
{
	return bAllowDuplicateLoadout;
}
