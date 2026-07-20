#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Engine/DataTable.h"
#include "ParcelCharacter.generated.h"

class UParcelPlayerStateComponent;
class UParcelMovementStatComponent;
class URagdollComponent;
class UParcelHeroComponent;
class UParcelInteractionComponent;
class UCharacterCarryComponent;
class UDFStatusEffectComponent;
class UDFKnockbackComponent;
class UAnimMontage;
class UWidgetComponent;
class UParcelStaminaComponent;

// 플레이어가 조종하는 기본 캐릭터 클래스
// 이동, 시점 회전, 점프, 래그돌 테스트 입력을 처리하고 멀티플레이 복제를 지원
//
// 담당자: 김로운
UCLASS()
class PARCEL_KNIGHT_API AParcelCharacter : public ACharacter
{
    GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	AParcelCharacter();
    virtual void BeginPlay() override;
    
    // 플레이어 입력 컴포넌트에 Enhanced Input 액션들을 바인딩
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // 서버에서 이 캐릭터가 컨트롤러에 빙의될 때 호출, Listen Server의 로컬 플레이어 입력 매핑 등록을 보강
	virtual void PossessedBy(AController* NewController) override;
	
	// [UI] 3D 아이디 위젯
	virtual void OnRep_PlayerState() override;

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
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	
	// Player State Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelPlayerStateComponent> PlayerStateComp;
    
	// Ragdoll Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<URagdollComponent> RagdollComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDFStatusEffectComponent> StatusEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDFKnockbackComponent> KnockbackComponent;

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
	
	// Nameplate Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> NameplateWidgetComp;
	
	// Stamina Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
	TObjectPtr<UParcelStaminaComponent> StaminaComp;
	
	void UpdateOverheadNameplate();
	
	FTimerHandle NameplateRetryTimerHandle;
	
	UFUNCTION()
	void OnCharacterStateTagsChanged(const FGameplayTagContainer& ActiveTags);

	UFUNCTION()
	void OnCharacterDeath();

public:
	// [Server] 슬롯 인덱스의 아이템 사용 — 소모품(Active)/코스메틱 분기 처리
	UFUNCTION(Server, Reliable)
	void Server_UseSlot(int32 SlotIndex);

	// 칭호 태그로 DataTable 조회 후 네임플레이트에 반영 — CustomizationComponent OnRep 및 PossessedBy에서 호출
	void ApplyTitle(FGameplayTag TitleTag);

	// 에디터에서 할당 — DT_CosmeticItems 할당
	UPROPERTY(EditDefaultsOnly, Category = "Title")
	TObjectPtr<UDataTable> CosmeticDataTable;

	// 컴포넌트 게터 — 외부 컴포넌트·AI·UI에서 접근용
	UFUNCTION(BlueprintPure, Category = "Character|Components")
	FORCEINLINE UParcelPlayerStateComponent* GetParcelPlayerStateComponent() const { return PlayerStateComp; }
	UFUNCTION(BlueprintPure, Category = "Character|Components")
	FORCEINLINE UParcelInteractionComponent* GetParcelInteractionComponent() const { return InteractionComp; }
	UFUNCTION(BlueprintPure, Category = "Character|Components")
	FORCEINLINE UParcelMovementStatComponent* GetParcelMovementStatComponent() const { return MovementStatComp; }
	UFUNCTION(BlueprintPure, Category = "Character|Components")
	FORCEINLINE UCharacterCarryComponent* GetCharacterCarryComponent() const { return CarryComp; }
	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE URagdollComponent* GetRagdollComponent() const { return RagdollComp; }
	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UParcelHeroComponent* GetParcelHeroComponent() const { return HeroComp; }
	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UDFStatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }
	UFUNCTION(BlueprintPure, Category = "Components")
	FORCEINLINE UDFKnockbackComponent* GetKnockbackComponent() const { return KnockbackComponent; }
	UFUNCTION(BlueprintPure, Category = "Character|Components")
	FORCEINLINE UParcelStaminaComponent* GetParcelStaminaComponent() const { return StaminaComp; }

private:
	// 총 히트스캔 — Server_UseSlot에서 Gun 아이템일 때 호출
	void DoGunLineTrace();

	// 클라이언트에서 Controller 값이 복제되어 바뀔 때 호출
	virtual void OnRep_Controller() override;

	// 점프 시작 시점과 지면 착지 타이밍 이식
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;
protected:
	// 사망 로직 보완
	UFUNCTION()
	void HandleCharacterDeath();
};
