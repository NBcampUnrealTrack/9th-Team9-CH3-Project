#include "Core/ParcelGameInstance.h"
#include "Core/ParcelSaveGame.h"
#include "Core/SessionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogParcelGameInstance, Log, All);

const FString UParcelGameInstance::SaveSlotName = TEXT("PlayerSaveSlot");

// ========================= 초기화 =========================

void UParcelGameInstance::Init()
{
	Super::Init();
	LoadData();

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
	if (SessionInviteSessionInterface.IsValid() && SessionInviteAcceptedHandle.IsValid())
	{
		SessionInviteSessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(
			SessionInviteAcceptedHandle);
	}
	SessionInviteAcceptedHandle.Reset();
	SessionInviteSessionInterface.Reset();

	Super::Shutdown();
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

	FOnlineSessionSearchResult SessionToJoin = InviteResult;

	// AdvancedSessions 5.5의 UAdvancedFriendsGameInstance와 동일한 UE 5.5 Steam 보정입니다.
	// listen session 초대 결과에 presence/lobby 플래그가 누락되는 엔진 케이스를 보완합니다.
	if (!SessionToJoin.Session.SessionSettings.bIsDedicated)
	{
		SessionToJoin.Session.SessionSettings.bUsesPresence = true;
		SessionToJoin.Session.SessionSettings.bUseLobbiesIfAvailable = true;
	}

	if (SessionSubsystem)
	{
		if (!SessionSubsystem->JoinSessionResult(SessionToJoin))
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
