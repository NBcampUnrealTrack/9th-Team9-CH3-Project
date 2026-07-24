#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ParcelStaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChangedSignature, float, CurrentStamina, float, MaxStamina);

/**
 *
 * 담당자: JYW
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UParcelStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UParcelStaminaComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Stamina")
    FOnStaminaChangedSignature OnStaminaChanged;

    // 게터
    UFUNCTION(BlueprintPure, Category = "Stamina")
    FORCEINLINE float GetCurrentStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    FORCEINLINE float GetMaxStamina() const { return MaxStamina; }

private:
    UFUNCTION()
    void OnRep_CurrentStamina();

    // [Server]
    void StartExhaustion();
    void StopExhaustion();
    
    UPROPERTY(Replicated, EditDefaultsOnly, Category = "Stamina|Settings")
    float MaxStamina;

    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Settings", meta = (ToolTip = "스태미나가 가득 찬 상태에서 바닥날 때까지 걸리는 시간 (초)"))
    float SprintDrainDuration;

    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Settings", meta = (ToolTip = "탈진 상태가 유지되는 시간 (초)"))
    float ExhaustionDuration;

    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Settings", meta = (ToolTip = "스태미나가 0에서 다시 가득 차기까지 걸리는 자연 회복 시간 (초)"))
    float RegenDuration;

    // 런타임 연산용 수치
    FORCEINLINE float GetDrainRate() const { return SprintDrainDuration > 0.f ? MaxStamina / SprintDrainDuration : 0.f; }
    FORCEINLINE float GetRegenRate() const { return RegenDuration > 0.f ? MaxStamina / RegenDuration : 0.f; }
    
    // 복제
    UPROPERTY(ReplicatedUsing = OnRep_CurrentStamina, VisibleAnywhere, Category = "Stamina")
    float CurrentStamina;

    FTimerHandle ExhaustionTimerHandle;
};