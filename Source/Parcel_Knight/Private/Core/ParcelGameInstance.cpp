#include "Core/ParcelGameInstance.h"
#include "Core/ParcelGameUserSettings.h"
#include "Core/ParcelSaveGame.h"
#include "Core/SessionSubsystem.h"
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

void UParcelGameInstance::SaveData()
{
	UParcelSaveGame* SaveGame = Cast<UParcelSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UParcelSaveGame::StaticClass()));

	SaveGame->MaxClearedStage  = MaxClearedStage;
	SaveGame->Money            = Money;
	SaveGame->OwnedConsumables = OwnedConsumables;
	SaveGame->OwnedCosmetics   = OwnedCosmetics;
	SaveGame->EquippedLoadout  = EquippedLoadout;
	SaveGame->EquippedSkin     = EquippedSkin;
	SaveGame->EquippedTitle    = EquippedTitle;
	SaveGame->EquippedEffect   = EquippedEffect;

	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
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
	if (Money < Amount) return false;
	Money -= Amount;
	SaveData();
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
