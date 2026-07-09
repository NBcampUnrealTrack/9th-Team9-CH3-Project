#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h" // GameplayTag 사용을 위해 필수 포함
#include "RagdollComponent.generated.h"

class ACharacter;
class UAnimMontage;

// 캐릭터의 래그돌 시작/종료 상태를 관리하는 컴포넌트입니다.
// 담당자: 김로운
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API URagdollComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URagdollComponent();

    // 래그돌을 시작합니다.
    UFUNCTION(BlueprintCallable, Category = "Ragdoll")
    void StartRagdoll();

    // 래그돌을 종료합니다.
    UFUNCTION(BlueprintCallable, Category = "Ragdoll")
    void StopRagdoll();

    // 현재 래그돌 상태에 따라 시작 또는 종료를 전환합니다.
    UFUNCTION(BlueprintCallable, Category = "Ragdoll")
    void ToggleRagdoll();

    // 현재 래그돌 상태를 GameplayTag 기반으로 반환합니다.
    UFUNCTION(BlueprintPure, Category = "Ragdoll")
    bool IsRagdoll() const;

    // 지면과 가까운지 확인하는 래그돌 해제 조건 감지용 함수
    UFUNCTION(BlueprintPure, Category = "Ragdoll")
    bool IsRagdollCloseToGround() const;

    // 캐릭터가 바라보는 방향에 따라 앞/뒤 일어나기 몽타주를 재생합니다.
    UFUNCTION(BlueprintCallable, Category = "Ragdoll")
    void PlayGetUpAnimation(bool bFront);

    // 기본 착지 구르기 몽타주를 재생합니다.
    UFUNCTION(BlueprintCallable, Category = "Ragdoll")
    void PlayLandRollAnimation();

protected:
    virtual void BeginPlay() override;

    // =========================
    // Get Up Animations
    // =========================
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Get Up Animations", meta = (DisplayName = "Get Up Front Default"))
    TObjectPtr<UAnimMontage> GetUpFrontDefault;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Get Up Animations", meta = (DisplayName = "Get Up Back Default"))
    TObjectPtr<UAnimMontage> GetUpBackDefault;

    // =========================
    // Roll Animations
    // =========================
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll Animations", meta = (DisplayName = "Land Roll Default"))
    TObjectPtr<UAnimMontage> LandRollDefault;

    // 스켈레탈 메시의 기본 상대 위치와 회전값을 저장할 변수 (복구용)
    UPROPERTY()
    FVector DefaultMeshRelativeLocation;

    UPROPERTY()
    FRotator DefaultMeshRelativeRotation;

    // Ragdoll 해제 시 지면 감지를 위한 트레이스 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll|Settings")
    float RagdollStopGroundTraceDistance = 120.f;

private:
    // 앞/뒤 상태에 맞는 일어나기 몽타주를 반환합니다.
    UAnimMontage* GetSelectedGetUpMontage(bool bFront) const;

    // 클라이언트가 래그돌 상태 변경을 서버에 요청할 때 사용하는 RPC입니다.
    UFUNCTION(Server, Reliable)
    void ServerSetRagdoll(bool bNewIsRagdoll);

    // 실제 래그돌 내부 처리 로직
    void ApplyStartRagdoll();
    void ApplyStopRagdoll();

    // 이 컴포넌트를 소유한 캐릭터 캐싱 변수
    UPROPERTY()
    ACharacter* OwnerCharacter;
};