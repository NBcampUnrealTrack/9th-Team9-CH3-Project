#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameUserSettings.h"
#include "Core/ParcelSaveGame.h"
#include "Core/SessionSubsystem.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelGameInstance, Log, All);

const FString UParcelGameInstance::SaveSlotName = TEXT("PlayerSaveSlot");

// ========================= 초기화 =========================

void UParcelGameInstance::Init()
{
	Super::Init();
	LoadData();

	if (PostLoadMapWithWorldHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapWithWorldHandle);
	}
	PostLoadMapWithWorldHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UParcelGameInstance::HandlePostLoadMapWithWorld);
	ApplyLocalUserSettings(this);

	if (SessionInviteAcceptedHandle.IsValid() && SessionInviteSessionInterface.IsValid())
	{
		SessionInviteSessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(
			SessionInviteAcceptedHandle);
	}
	SessionInviteAcceptedHandle.Reset();
	SessionInviteSessionInterface.Reset();

	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		const IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SessionInviteSessionInterface = Sessions;
			SessionInviteAcceptedHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
				FOnSessionUserInviteAcceptedDelegate::CreateUObject(
					this,
					&UParcelGameInstance::HandleSessionInviteAccepted));
		}
	}

	if (!SessionInviteAcceptedHandle.IsValid())
	{
		UE_LOG(LogParcelGameInstance, Error, TEXT("Session invite-accepted delegate could not be registered."));
	}
}

void UParcelGameInstance::Shutdown()
{
	if (PostLoadMapWithWorldHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapWithWorldHandle);
		PostLoadMapWithWorldHandle.Reset();
	}

	if (SessionInviteSessionInterface.IsValid() && SessionInviteAcceptedHandle.IsValid())
	{
		SessionInviteSessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(
			SessionInviteAcceptedHandle);
	}
	SessionInviteAcceptedHandle.Reset();
	SessionInviteSessionInterface.Reset();

	Super::Shutdown();
}

void UParcelGameInstance::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == this)
	{
		ApplyLocalUserSettings(LoadedWorld);
	}
}

void UParcelGameInstance::ApplyLocalUserSettings(const UObject* WorldContextObject)
{
	if (UParcelGameUserSettings* Settings = UParcelGameUserSettings::GetParcelGameUserSettings())
	{
		Settings->ApplyMasterVolume(WorldContextObject);
	}
	else
	{
		UE_LOG(
			LogParcelGameInstance,
			Error,
			TEXT("ParcelGameUserSettings is unavailable. Check GameUserSettingsClassName in DefaultEngine.ini."));
	}
}

void UParcelGameInstance::ReturnToMainMenu()
{
	if (USessionSubsystem* SessionSubsystem = GetSubsystem<USessionSubsystem>())
	{
		SessionSubsystem->LeaveSession();
		return;
	}

	Super::ReturnToMainMenu();
}

void UParcelGameInstance::HandleSessionInviteAccepted(
	bool bWasSuccessful,
	int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	USessionSubsystem* SessionSubsystem = GetSubsystem<USessionSubsystem>();
	const bool bInviteIsValid = bWasSuccessful && UserId.IsValid() && InviteResult.IsValid();
	if (SessionSubsystem)
	{
		SessionSubsystem->NotifySessionInviteAccepted(bInviteIsValid);
	}

	if (!bInviteIsValid)
	{
		UE_LOG(
			LogParcelGameInstance,
			Error,
			TEXT("Steam session invite acceptance failed: success=%d user=%d result=%d controller=%d."),
			bWasSuccessful,
			UserId.IsValid(),
			InviteResult.IsValid(),
			ControllerId);
		return;
	}

	UE_LOG(
		LogParcelGameInstance,
		Log,
		TEXT("Steam session invite accepted by controller %d; forwarding GameSession result."),
		ControllerId);

	if (SessionSubsystem)
	{
		if (!SessionSubsystem->JoinSessionResult(InviteResult))
		{
			UE_LOG(LogParcelGameInstance, Error, TEXT("Steam invite was accepted, but JoinSession could not be started."));
		}
	}
	else
	{
		UE_LOG(LogParcelGameInstance, Error, TEXT("Steam invite was accepted, but SessionSubsystem is unavailable."));
	}
}

// ========================= 저장 / 불러오기 =========================

bool UParcelGameInstance::SaveData()
{
	UParcelSaveGame* SaveGame = Cast<UParcelSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UParcelSaveGame::StaticClass()));
	if (!SaveGame)
	{
		UE_LOG(LogParcelGameInstance, Error, TEXT("Failed to allocate ParcelSaveGame."));
		return false;
	}

	SaveGame->MaxClearedStage  = MaxClearedStage;
	SaveGame->Money            = Money;
	SaveGame->OwnedConsumables = OwnedConsumables;
	SaveGame->OwnedCosmetics   = OwnedCosmetics;
	SaveGame->EquippedLoadout  = EquippedLoadout;
	SaveGame->EquippedSkin     = EquippedSkin;
	SaveGame->EquippedTitle    = EquippedTitle;
	SaveGame->EquippedEffect   = EquippedEffect;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
	if (!bSaved)
	{
		UE_LOG(LogParcelGameInstance, Error, TEXT("Failed to save local profile to slot %s."), *SaveSlotName);
	}
	return bSaved;
}

void UParcelGameInstance::LoadData()
{
	UParcelSaveGame* SaveGame = Cast<UParcelSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

	// 저장 파일이 없으면 기본값으로 초기화
	if (!SaveGame) return;

	MaxClearedStage  = SaveGame->MaxClearedStage;
	Money            = SaveGame->Money;
	OwnedConsumables = SaveGame->OwnedConsumables;
	OwnedCosmetics   = SaveGame->OwnedCosmetics;
	EquippedLoadout  = SaveGame->EquippedLoadout;
	EquippedSkin     = SaveGame->EquippedSkin;
	EquippedTitle    = SaveGame->EquippedTitle;
	EquippedEffect   = SaveGame->EquippedEffect;
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
	if (Amount <= 0)
	{
		UE_LOG(LogParcelGameInstance, Warning, TEXT("SpendMoney rejected non-positive amount %d."), Amount);
		return false;
	}

	const int64 NewBalance = static_cast<int64>(Money) - static_cast<int64>(Amount);
	if (NewBalance < 0 || NewBalance > MAX_int32)
	{
		UE_LOG(
			LogParcelGameInstance,
			Warning,
			TEXT("SpendMoney rejected amount %d for current balance %d."),
			Amount,
			Money);
		return false;
	}

	const int32 PreviousBalance = Money;
	Money = static_cast<int32>(NewBalance);
	if (!SaveData())
	{
		Money = PreviousBalance;
		UE_LOG(LogParcelGameInstance, Error, TEXT("SpendMoney rolled back because the local profile could not be saved."));
		return false;
	}

	return true;
}

// ========================= 보유 아이템 =========================

bool UParcelGameInstance::HasOwnedConsumable(FGameplayTag ItemTag) const
{
	return OwnedConsumables.Contains(ItemTag);
}

bool UParcelGameInstance::HasOwnedCosmetic(FGameplayTag ItemTag) const
{
	return OwnedCosmetics.Contains(ItemTag);
}

void UParcelGameInstance::AddOwnedConsumable(FGameplayTag ItemTag)
{
	OwnedConsumables.AddUnique(ItemTag);
	SaveData();
}

void UParcelGameInstance::AddOwnedCosmetic(FGameplayTag ItemTag)
{
	OwnedCosmetics.AddUnique(ItemTag);
	SaveData();
}

const TArray<FGameplayTag>& UParcelGameInstance::GetOwnedConsumables() const
{
	return OwnedConsumables;
}

const TArray<FGameplayTag>& UParcelGameInstance::GetOwnedCosmetics() const
{
	return OwnedCosmetics;
}

bool UParcelGameInstance::TryPurchaseConsumable(
	UDataTable* ItemTable,
	FGameplayTag ItemTag,
	FText& OutStatusMessage)
{
	OutStatusMessage = FText::GetEmpty();

	if (!ItemTable || ItemTable->GetRowStruct() != FItemData::StaticStruct())
	{
		OutStatusMessage = FText::FromString(TEXT("아이템 데이터 테이블이 올바르지 않습니다."));
		return false;
	}

	if (!ItemTag.IsValid())
	{
		OutStatusMessage = FText::FromString(TEXT("유효하지 않은 아이템입니다."));
		return false;
	}

	const FItemData* ItemData = nullptr;
	for (const FName RowName : ItemTable->GetRowNames())
	{
		const FItemData* Candidate = ItemTable->FindRow<FItemData>(RowName, TEXT("TryPurchaseConsumable"));
		if (Candidate && Candidate->ItemTag == ItemTag)
		{
			ItemData = Candidate;
			break;
		}
	}

	if (!ItemData)
	{
		OutStatusMessage = FText::FromString(TEXT("상점에 등록되지 않은 아이템입니다."));
		return false;
	}

	static const FGameplayTag ConsumableRoot =
		FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables"));
	if (!ItemTag.MatchesTag(ConsumableRoot))
	{
		OutStatusMessage = FText::FromString(TEXT("출전 아이템 상점에서 구매할 수 없는 아이템입니다."));
		return false;
	}

	if (ItemData->Price < 0)
	{
		OutStatusMessage = FText::FromString(TEXT("아이템 가격 설정이 올바르지 않습니다."));
		return false;
	}

	if (HasOwnedConsumable(ItemTag))
	{
		OutStatusMessage = FText::FromString(TEXT("이미 보유한 아이템입니다."));
		return false;
	}

	if (Money < ItemData->Price)
	{
		OutStatusMessage = FText::FromString(TEXT("보유 재화가 부족합니다."));
		return false;
	}

	Money -= ItemData->Price;
	OwnedConsumables.Add(ItemTag);
	SaveData();

	OutStatusMessage = FText::Format(
		FText::FromString(TEXT("{0} 구매가 완료되었습니다.")),
		ItemData->DisplayName);
	UE_LOG(
		LogParcelGameInstance,
		Log,
		TEXT("Local profile purchase succeeded: Item=%s Price=%d Money=%d"),
		*ItemTag.ToString(),
		ItemData->Price,
		Money);
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

bool UParcelGameInstance::ToggleLoadoutItem(
	FGameplayTag ItemTag,
	int32 RequestedMaxItems,
	FText& OutStatusMessage)
{
	OutStatusMessage = FText::GetEmpty();
	const int32 EffectiveMaxItems = FMath::Clamp(RequestedMaxItems, 0, MaxLoadoutSlots);

	if (!ItemTag.IsValid() || !OwnedConsumables.Contains(ItemTag))
	{
		OutStatusMessage = FText::FromString(TEXT("보유하지 않은 아이템은 선택할 수 없습니다."));
		return false;
	}

	EquippedLoadout.RemoveAll([](const FGameplayTag& ExistingTag)
	{
		return !ExistingTag.IsValid();
	});

	if (EquippedLoadout.Contains(ItemTag))
	{
		EquippedLoadout.Remove(ItemTag);
		SaveData();
		return true;
	}

	if (EquippedLoadout.Num() >= EffectiveMaxItems)
	{
		OutStatusMessage = FText::Format(
			FText::FromString(TEXT("게임에 가져갈 수 있는 아이템은 최대 {0}개입니다.")),
			FText::AsNumber(EffectiveMaxItems));
		return false;
	}

	EquippedLoadout.Add(ItemTag);
	SaveData();
	return true;
}

bool UParcelGameInstance::ValidateSavedLoadout(UDataTable* ItemTable, int32 RequestedMaxItems)
{
	if (!ItemTable || ItemTable->GetRowStruct() != FItemData::StaticStruct())
	{
		return false;
	}

	const int32 EffectiveMaxItems = FMath::Clamp(RequestedMaxItems, 0, MaxLoadoutSlots);
	TSet<FGameplayTag> TableItems;
	static const FGameplayTag ConsumableRoot =
		FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables"));
	for (const FName RowName : ItemTable->GetRowNames())
	{
		if (const FItemData* Row = ItemTable->FindRow<FItemData>(RowName, TEXT("ValidateSavedLoadout")))
		{
			if (Row->ItemTag.IsValid() && Row->ItemTag.MatchesTag(ConsumableRoot))
			{
				TableItems.Add(Row->ItemTag);
			}
		}
	}

	TArray<FGameplayTag> ValidatedLoadout;
	for (const FGameplayTag& ExistingTag : EquippedLoadout)
	{
		if (ValidatedLoadout.Num() >= EffectiveMaxItems)
		{
			break;
		}

		if (ExistingTag.IsValid()
			&& OwnedConsumables.Contains(ExistingTag)
			&& TableItems.Contains(ExistingTag))
		{
			ValidatedLoadout.AddUnique(ExistingTag);
		}
	}

	if (ValidatedLoadout == EquippedLoadout)
	{
		return true;
	}

	EquippedLoadout = MoveTemp(ValidatedLoadout);
	SaveData();
	UE_LOG(LogParcelGameInstance, Warning, TEXT("Removed invalid entries from the saved loadout."));
	return true;
}

// ========================= 스테이지 진행도 =========================

int32 UParcelGameInstance::GetMaxClearedStage() const
{
	return MaxClearedStage;
}

void UParcelGameInstance::UpdateMaxClearedStage(int32 ClearedStageIndex)
{
	if (ClearedStageIndex > MaxClearedStage)
	{
		MaxClearedStage = ClearedStageIndex;
		SaveData();
		UE_LOG(LogParcelGameInstance, Log, TEXT("스테이지 %d 클리어 — 최고 기록 갱신 후 저장"), ClearedStageIndex);
	}
}

bool UParcelGameInstance::IsStageUnlocked(int32 StageIndex) const
{
	// 1번은 항상 열려있고, N번은 N-1번을 클리어해야 진입 가능
	return StageIndex == 1 || StageIndex <= MaxClearedStage + 1;
}

// ========================= 스테이지 맵 =========================

void UParcelGameInstance::SetPendingMapPath(const FString& MapPath)
{
	PendingMapPath = MapPath;
}

FString UParcelGameInstance::GetPendingMapPath() const
{
	return PendingMapPath;
}

// ========================= 커스터마이징 =========================

void UParcelGameInstance::EquipSkin(FGameplayTag SkinTag)
{
	// 보유하지 않은 코스메틱은 장착 불가
	if (!HasOwnedCosmetic(SkinTag)) return;
	EquippedSkin = SkinTag;
	SaveData();
}

void UParcelGameInstance::EquipTitle(FGameplayTag TitleTag)
{
	if (!HasOwnedCosmetic(TitleTag)) return;
	EquippedTitle = TitleTag;
	SaveData();
}

void UParcelGameInstance::EquipEffect(FGameplayTag EffectTag)
{
	if (!HasOwnedCosmetic(EffectTag)) return;
	EquippedEffect = EffectTag;
	SaveData();
}

// 미장착이면 Invalid 태그 반환 — 호출자에서 IsValid()로 체크
FGameplayTag UParcelGameInstance::GetEquippedSkin()   const { return EquippedSkin; }
FGameplayTag UParcelGameInstance::GetEquippedTitle()  const { return EquippedTitle; }
FGameplayTag UParcelGameInstance::GetEquippedEffect() const { return EquippedEffect; }
