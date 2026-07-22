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
class UInputAction;
class USpringArmComponent;
class USceneCaptureComponent2D;

UCLASS()
class PARCEL_KNIGHT_API AParcelCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
    
    AParcelCharacter();
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
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
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelPlayerStateComponent> PlayerStateComp;
    
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
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelHeroComponent> HeroComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelInteractionComponent> InteractionComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelMovementStatComponent> MovementStatComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCharacterCarryComponent> CarryComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UWidgetComponent> NameplateWidgetComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelStaminaComponent> StaminaComp;
    
    void UpdateOverheadNameplate();
    FTimerHandle NameplateRetryTimerHandle;
    
    UFUNCTION()
    void OnCharacterStateTagsChanged(const FGameplayTagContainer& ActiveTags);
    
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> IA_OpenChat;

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

	// 로컬 플레이어 전용 미니맵 씬 캡처 활성화 및 타인 캐릭터 캡처 제거
	void UpdateMinimapCaptureState();

	// 클라이언트에서 Controller 값이 복제되어 바뀔 때 호출
	virtual void OnRep_Controller() override;

	// 점프 시작 시점과 지면 착지 타이밍 이식
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> MinimapSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> MinimapCaptureComponent;

    UFUNCTION() void HandleCharacterDeath();
};