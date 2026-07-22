#include "Core/InventoryComponent.h"
#include "ParcelLog.h"
#include "Core/ParcelGameInstance.h"
#include "Core/HealthComponent.h"
#include "Core/ParcelGameplayTags.h"
#include "Data/ItemData.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY(LogItem);

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
}

// ========================= 초기화 =========================

void UInventoryComponent::InitFromGameInstance(UParcelGameInstance* GI)
{
	if (!GI) return;

	Items = GI->GetLoadout();

	// 비어있는 슬롯(EmptyTag) 제거
	Items.RemoveAll([](const FGameplayTag& Tag) { return !Tag.IsValid(); });
}

// ========================= 조회 =========================

bool UInventoryComponent::HasItem(FGameplayTag ItemTag) const
{
	return Items.Contains(ItemTag);
}

const TArray<FGameplayTag>& UInventoryComponent::GetItems() const
{
	return Items;
}

bool UInventoryComponent::SetValidatedLoadout(
	const TArray<FGameplayTag>& RequestedItems,
	int32 MaxItems)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ConsumableDataTable)
	{
		ITEM_LOG(
			Warning,
			TEXT("[Inv] Loadout rejected: Authority=%d DataTable=%s"),
			GetOwner() && GetOwner()->HasAuthority(),
			*GetNameSafe(ConsumableDataTable));
		return false;
	}

	const int32 SafeMaxItems = FMath::Clamp(MaxItems, 0, 3);
	if (RequestedItems.Num() > SafeMaxItems)
	{
		ITEM_LOG(Warning, TEXT("[Inv] Loadout rejected: requested=%d max=%d"), RequestedItems.Num(), SafeMaxItems);
		return false;
	}

	TArray<FGameplayTag> ValidatedItems;
	static const FGameplayTag ConsumableRoot =
		FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables"));
	for (const FGameplayTag& RequestedItem : RequestedItems)
	{
		if (!RequestedItem.IsValid()
			|| !RequestedItem.MatchesTag(ConsumableRoot)
			|| ValidatedItems.Contains(RequestedItem)
			|| !FindItemData(RequestedItem))
		{
			ITEM_LOG(
				Warning,
				TEXT("[Inv] Loadout rejected: invalid, duplicate, or missing table item=%s"),
				*RequestedItem.ToString());
			return false;
		}

		ValidatedItems.Add(RequestedItem);
	}

	if (Items == ValidatedItems)
	{
		return true;
	}

	Items = MoveTemp(ValidatedItems);
	OnInventoryChanged.Broadcast();

	if (const APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		ApplyPassiveEffects(PlayerState->GetPawn());
	}

	ITEM_LOG(Log, TEXT("[Inv] Server accepted %d loadout item(s)."), Items.Num());
	return true;
}

// ========================= 추가·사용 =========================

void UInventoryComponent::AddItem(FGameplayTag ItemTag)
{
	if (!GetOwner()->HasAuthority() || !ItemTag.IsValid()) return;
	Items.AddUnique(ItemTag);
}

FItemData* UInventoryComponent::FindItemData(FGameplayTag ItemTag) const
{
	if (!ConsumableDataTable) return nullptr;
	TArray<FItemData*> AllRows;
	ConsumableDataTable->GetAllRows<FItemData>(TEXT("FindItemData"), AllRows);
	for (FItemData* Row : AllRows)
	{
		if (Row && Row->ItemTag == ItemTag) return Row;
	}
	return nullptr;
}

bool UInventoryComponent::CanUseItem(FGameplayTag ItemTag) const
{
	if (!HasItem(ItemTag)) return false;
	const FItemData* Data = FindItemData(ItemTag);
	if (!Data || Data->Cooldown <= 0.f) return true;
	const float* LastUsed = ItemLastUsedTime.Find(ItemTag);
	if (!LastUsed) return true;
	return GetWorld()->GetTimeSeconds() - *LastUsed >= Data->Cooldown;
}

void UInventoryComponent::ApplyEffects(const FItemData* Data, APawn* Pawn)
{
	if (!Pawn) return;

	for (const FItemEffect& Effect : Data->Effects)
	{
		if (Effect.EffectTag == ParcelGameplayTags::Effect_Stat_HP)
		{
			if (UHealthComponent* HC = Pawn->FindComponentByClass<UHealthComponent>())
				HC->IncreaseMaxHP(Effect.Value);
		}
	}
}

void UInventoryComponent::ApplyPassiveEffects(APawn* Pawn)
{
	if (!GetOwner()->HasAuthority() || !Pawn) return;

	static const FGameplayTag TAG_Consumable = FGameplayTag::RequestGameplayTag(TEXT("Item.Consumables"));

	for (const FGameplayTag& ItemTag : Items)
	{
		if (!ItemTag.MatchesTag(TAG_Consumable)) continue;

		FItemData* Data = FindItemData(ItemTag);
		if (!Data || Data->ActivationType != EItemActivationType::Passive) continue;

		ApplyEffects(Data, Pawn);
	}
}

bool UInventoryComponent::UseItem(FGameplayTag ItemTag)
{
	if (!GetOwner()->HasAuthority())
	{
		ITEM_LOG(Warning, TEXT("[Inv] UseItem(%s) 실패: 권한 없음"), *ItemTag.ToString());
		return false;
	}
	if (!CanUseItem(ItemTag))
	{
		ITEM_LOG(Warning, TEXT("[Inv] UseItem(%s) 실패: 미보유 또는 쿨타임 중"), *ItemTag.ToString());
		return false;
	}

	FItemData* Data = FindItemData(ItemTag);
	if (!Data)
	{
		ITEM_LOG(Warning, TEXT("[Inv] UseItem(%s) 실패: DataTable에서 데이터 없음 (DT=%s)"),
			*ItemTag.ToString(),
			ConsumableDataTable ? *ConsumableDataTable->GetName() : TEXT("null (미할당)"));
		return false;
	}

	if (Data->ActivationType == EItemActivationType::Passive)
	{
		ITEM_LOG(Warning, TEXT("[Inv] UseItem(%s) 실패: 패시브 아이템은 키 입력으로 사용 불가"), *ItemTag.ToString());
		return false;
	}

	if (Data->Cooldown > 0.f)
		ItemLastUsedTime.Add(ItemTag, GetWorld()->GetTimeSeconds());

	APlayerState* PS = Cast<APlayerState>(GetOwner());
	ApplyEffects(Data, PS ? PS->GetPawn() : nullptr);
	Multicast_PlayItemFX(ItemTag);

	if (Data->bIsPermanent)
	{
		ITEM_LOG(Log, TEXT("[Inv] UseItem(%s) 성공 (영구 아이템)"), *ItemTag.ToString());
		return true;
	}

	Items.Remove(ItemTag);
	ITEM_LOG(Log, TEXT("[Inv] UseItem(%s) 성공, 인벤토리에서 제거"), *ItemTag.ToString());
	return true;
}

void UInventoryComponent::Server_UseItem_Implementation(FGameplayTag ItemTag)
{
	UseItem(ItemTag);
}

void UInventoryComponent::Multicast_PlayItemFX_Implementation(FGameplayTag ItemTag)
{
	const FItemData* Data = FindItemData(ItemTag);
	if (!Data) return;

	APawn* Pawn = nullptr;
	if (const APlayerState* PS = Cast<APlayerState>(GetOwner()))
		Pawn = PS->GetPawn();

	const FVector Location = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;

	if (!Data->UseSound.IsNull())
		if (USoundBase* Sound = Data->UseSound.LoadSynchronous())
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);

	if (!Data->UseEffect.IsNull())
		if (UNiagaraSystem* FX = Data->UseEffect.LoadSynchronous())
		{
			if (Pawn)
				UNiagaraFunctionLibrary::SpawnSystemAttached(FX, Pawn->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
			else
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FX, Location);
		}
}

// ========================= 복제 콜백 =========================

void UInventoryComponent::OnRep_Items()
{
	// 클라이언트에서 Items가 갱신되면 UI에 알림
	OnInventoryChanged.Broadcast();
}
