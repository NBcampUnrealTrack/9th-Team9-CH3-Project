#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CustomizationComponent.generated.h"

class UParcelGameInstance;

/**
 * 플레이어의 장착 코스메틱(스킨·칭호·이펙트)을 런타임에 보관하는 컴포넌트
 * PlayerState에 부착되며 세션 내내 유지된다.
 *
 * [흐름]
 *   스테이지 진입 → InitFromGameInstance()로 장착 정보 복사
 *   UI에서 변경  → GameInstance::Equip*() 저장 후 이 컴포넌트의 Equip*() 호출
 *   캐릭터 스폰  → Character가 GetEquipped*()를 읽어 비주얼 적용
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCustomizationComponent();

	// [Server] 스테이지 진입 시 GameInstance의 장착 정보를 이 컴포넌트에 복사
	void InitFromGameInstance(UParcelGameInstance* GI);

	// [All] 스킨 장착 — CurrentSkin 갱신 (GameInstance 저장은 호출자 책임)
	void EquipSkin   (FGameplayTag SkinTag);
	// [All] 칭호 장착
	void EquipTitle  (FGameplayTag TitleTag);
	// [All] 이펙트 장착
	void EquipEffect (FGameplayTag EffectTag);

	// [All] 현재 장착된 스킨 태그 반환 — 미장착이면 Invalid 태그
	FGameplayTag GetEquippedSkin()   const;
	// [All] 현재 장착된 칭호 태그 반환
	FGameplayTag GetEquippedTitle()  const;
	// [All] 현재 장착된 이펙트 태그 반환
	FGameplayTag GetEquippedEffect() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// TODO: ApplySkin(ACharacter*)   — 캐릭터 메시·머티리얼 교체 (캐릭터 메시 구조 확정 후 구현)
	// TODO: ApplyTitle(AHUD*)        — HUD 칭호 위젯 갱신 (WBP_HUD 구현 후)
	// TODO: ApplyEffect(ACharacter*) — 나이아가라 이펙트 스폰 (에셋 제작 후)

private:
	// 현재 장착 중인 코스메틱 태그 — 다른 클라이언트에 복제되어 비주얼 적용에 사용
	UPROPERTY(Replicated)
	FGameplayTag CurrentSkin;

	UPROPERTY(Replicated)
	FGameplayTag CurrentTitle;

	UPROPERTY(Replicated)
	FGameplayTag CurrentEffect;
};
