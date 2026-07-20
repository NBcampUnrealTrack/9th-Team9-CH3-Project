#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
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

public: 
    virtual void OnRep_Controller() override;
    virtual void OnJumped_Implementation() override;
    virtual void Landed(const FHitResult& Hit) override;
    
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

protected:
    UFUNCTION() void HandleCharacterDeath();
};