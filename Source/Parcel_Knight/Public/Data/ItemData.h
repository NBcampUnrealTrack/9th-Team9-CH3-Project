#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "ItemData.generated.h"

UENUM(BlueprintType)
enum class EItemActivationType : uint8
{
	Passive,   // 게임 시작 시 자동 발동 — 슬롯에 표시만
	Active,    // 1/2/3 키로 발동
};

UENUM(BlueprintType)
enum class ETitleType : uint8
{
	TextOnly,  // 기본 색상 칭호 텍스트만 표시
	Colored,   // 칭호 텍스트에 커스텀 색상 적용
};

/**
 * 아이템 효과 하나를 표현하는 구조체
 * DataTable의 Effects 배열 원소 — 에디터에서 + 버튼으로 필요한 만큼만 추가한다.
 */
USTRUCT(BlueprintType)
struct PARCEL_KNIGHT_API FItemEffect
{
	GENERATED_BODY()

	// 효과 종류 — 프로젝트 세팅에서 picker로 선택 (Effect.Stat.HP 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag EffectTag;

	// 효과 수치 (HP 증가량, 속도 증가량 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Value = 0.f;
};

/**
 * 상점 아이템 데이터 Row 구조체
 * DataTable의 행 정의 — 에디터에서 아이템 추가 시 이 구조체를 사용한다.
 * ItemTag 계층으로 타입을 구분한다 (Item.Consumable.* / Item.Cosmetic.*)
 *
 * 담당자: 한수현
 */
USTRUCT(BlueprintType)
struct PARCEL_KNIGHT_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 식별자 — 타입도 겸함 (Item.Consumable.Cart, Item.Cosmetic.Skin.Blue 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag ItemTag;

	// 상점 구매 가격
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Price = 0;

	// 상점/인벤토리에 표시될 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	// 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// 아이콘 — 소프트 레퍼런스로 필요할 때만 로드
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	// 영구 사용 가능 여부 — true면 사용해도 인벤토리에서 제거되지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsPermanent = false;

	// 사용 효과 목록 — + 버튼으로 필요한 효과만 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TArray<FItemEffect> Effects;

	// 재사용 대기 시간 (초) — 0이면 쿨타임 없음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	float Cooldown = 0.f;

	// 소모품 전용 — Passive: 게임 시작 시 자동, Active: 키 입력 시 발동
	// Item.Cosmetic.* 태그 아이템은 이 필드를 무시함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	EItemActivationType ActivationType = EItemActivationType::Active;

	// 사용 시 재생할 나이아가라 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<UNiagaraSystem> UseEffect;

	// 사용 시 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FX")
	TSoftObjectPtr<USoundBase> UseSound;

	// Item.Cosmetic.Title.* 전용 — 칭호 표시 방식
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	ETitleType TitleType = ETitleType::TextOnly;

	// Colored 타입일 때 칭호 텍스트에 적용할 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	FLinearColor TitleColor = FLinearColor::White;
};
