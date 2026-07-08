#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "ParcelCharacter.generated.h"

class UParcelMovementStatComponent;
class URagdollComponent;
class UParcelHeroComponent;
class UParcelInteractionComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class USpringArmComponent;
class UCharacterCarryComponent;
class UAnimMontage;

// 플레이어가 조종하는 기본 캐릭터 클래스
// 이동, 시점 회전, 점프, 래그돌 테스트 입력을 처리하고 멀티플레이 복제를 지원
//
// 담당자: 김로운
UCLASS()
class PARCEL_KNIGHT_API AParcelCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AParcelCharacter();
	
	virtual void BeginPlay() override;
	
	// 멀티플레이어 변수 복제를 위한 Rep (For StateTag)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 플레이어 입력 컴포넌트에 Enhanced Input 액션들을 바인딩
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 서버에서 이 캐릭터가 컨트롤러에 빙의될 때 호출, Listen Server의 로컬 플레이어 입력 매핑 등록을 보강
	virtual void PossessedBy(AController* NewController) override;

	// 클라이언트에서 Controller 값이 복제되어 바뀔 때 호출, 원격 접속 클라이언트가 possession 이후 입력 매핑을 놓치지 않게 합니다.
	virtual void OnRep_Controller() override;
	
	// [Server] GameplayTags 기반 상태 제어 인터페이스
	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void AddStateTag(FGameplayTag NewStateTag);

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void RemoveStateTag(FGameplayTag StateTag);

	// 현재 특정 상태인지 조회 (인라인/Pure 함수 최적화)
	UFUNCTION(BlueprintPure, Category = "Character|State")
	FORCEINLINE bool HasStateTag(FGameplayTag StateTag) const { return CharacterStateTags.HasTag(StateTag); }

	// 애님 블루프린트나 UI에서 컨테이너 전체를 조회할 때 사용하는 게터
	UFUNCTION(BlueprintPure, Category = "Character|State")
	FORCEINLINE FGameplayTagContainer GetCharacterStateTags() const { return CharacterStateTags; }
	
	// 다른 요소들이 상호작용 컴포넌트에 접근할 수 있도록 하는 게터
	FORCEINLINE UParcelInteractionComponent* GetParcelInteractionComponent() const { return InteractionComp; }
	FORCEINLINE UParcelMovementStatComponent* GetParcelMovementStatComponent() const { return MovementStatComp; }
	FORCEINLINE UCharacterCarryComponent* GetCharacterCarryComponent() const { return CarryComp; }

	void SetRagdollState(bool bNewIsRagdoll, bool bNewIsGettingUp);

	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void FinishGetUp();

	UFUNCTION(BlueprintPure, Category = "Ragdoll")
	bool GetIsRagdoll() const { return bIsRagdoll; }

	UFUNCTION(BlueprintPure, Category = "Ragdoll")
	bool GetIsGettingUp() const { return bIsGettingUp; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ragdoll")
	TObjectPtr<UAnimMontage> GetUpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> GetUpBackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> GetUpFrontMontage;

protected:
	// Ragdoll Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<URagdollComponent> RagdollComp;

	UPROPERTY(BlueprintReadOnly, Category = "Ragdoll")
	bool bIsRagdoll = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ragdoll")
	bool bIsGettingUp = false;

	FTimerHandle GetUpTimerHandle;
	
	// Hero Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UParcelHeroComponent> HeroComp;
	
	// Interaction Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UParcelInteractionComponent> InteractionComp;
	
	// MovementStat Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UParcelMovementStatComponent> MovementStatComp;
	
	// CharacterCarry Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterCarryComponent> CarryComp;
	
private:
	// 서버에서 상태를 받아 클라이언트의 애니메이션/효과를 켜기 위한 RepNotify 함수
	UFUNCTION()
	void OnRep_CharacterStateTags();

	UPROPERTY(ReplicatedUsing = OnRep_CharacterStateTags, VisibleAnywhere, Category = "Character|State")
	FGameplayTagContainer CharacterStateTags;
};
