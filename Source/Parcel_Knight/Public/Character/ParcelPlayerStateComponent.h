#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ParcelPlayerStateComponent.generated.h"

// 상태 변화를 구독할 수 있는 멀티캐스트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateTagChanged, FGameplayTag, ChangedTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateTagsChangedSignature, const FGameplayTagContainer&, ActiveTags);

/**
 * 캐릭터의 런타임 상태 Tags를 관리하고 Replication을 처리하는 상태 창고 컴포넌트
 * 담당자 : JYW
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UParcelPlayerStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UParcelPlayerStateComponent();

	// 멀티플레이어 컴포넌트 복제를 위한 Rep
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 상태 추가/제거 시 트리거될 이벤트 (블루프린트 UI 연동용)
	UPROPERTY(BlueprintAssignable, Category = "Character|State")
	FOnPlayerStateTagChanged OnStateTagAdded;

	UPROPERTY(BlueprintAssignable, Category = "Character|State")
	FOnPlayerStateTagChanged OnStateTagRemoved;
	
	UPROPERTY(BlueprintAssignable, Category = "Character|State")
	FOnCharacterStateTagsChangedSignature OnCharacterStateTagsChanged;

	// 태그 제어 및 조회
	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void AddStateTag(FGameplayTag NewStateTag);

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void RemoveStateTag(FGameplayTag StateTag);

	UFUNCTION(BlueprintPure, Category = "Character|State")
	bool HasStateTag(FGameplayTag StateTag) const;

	UFUNCTION(BlueprintPure, Category = "Character|State")
	FORCEINLINE FGameplayTagContainer GetCharacterStateTags() const { return CharacterStateTags; }

private:
	// 서버가 태그를 복제해 주면 원격 클라이언트에서 연출/애니메이션 분기용으로 호출
	UFUNCTION()
	void OnRep_CharacterStateTags();

	UPROPERTY(ReplicatedUsing = OnRep_CharacterStateTags, VisibleAnywhere, Category = "Character|State")
	FGameplayTagContainer CharacterStateTags;
};