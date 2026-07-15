#include "Core/CustomizationComponent.h"
#include "Core/ParcelGameInstance.h"
#include "Net/UnrealNetwork.h"

UCustomizationComponent::UCustomizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCustomizationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCustomizationComponent, CurrentSkin);
	DOREPLIFETIME(UCustomizationComponent, CurrentTitle);
	DOREPLIFETIME(UCustomizationComponent, CurrentEffect);
}

// ========================= 초기화 =========================

void UCustomizationComponent::InitFromGameInstance(UParcelGameInstance* GI)
{
	if (!GI) return;

	// GameInstance의 장착 정보를 런타임 캐시에 복사
	CurrentSkin   = GI->GetEquippedSkin();
	CurrentTitle  = GI->GetEquippedTitle();
	CurrentEffect = GI->GetEquippedEffect();
}

// ========================= 장착 =========================

void UCustomizationComponent::EquipSkin(FGameplayTag SkinTag)
{
	CurrentSkin = SkinTag;
	// TODO: ApplySkin 호출 — 캐릭터 메시 구조 확정 후 추가
}

void UCustomizationComponent::EquipTitle(FGameplayTag TitleTag)
{
	CurrentTitle = TitleTag;
	// TODO: ApplyTitle 호출 — WBP_HUD 구현 후 추가
}

void UCustomizationComponent::EquipEffect(FGameplayTag EffectTag)
{
	CurrentEffect = EffectTag;
	// TODO: ApplyEffect 호출 — 나이아가라 에셋 제작 후 추가
}

// ========================= 조회 =========================

FGameplayTag UCustomizationComponent::GetEquippedSkin()   const { return CurrentSkin; }
FGameplayTag UCustomizationComponent::GetEquippedTitle()  const { return CurrentTitle; }
FGameplayTag UCustomizationComponent::GetEquippedEffect() const { return CurrentEffect; }
