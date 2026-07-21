#include "Character/RagdollComponent.h"
#include "ParcelLog.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/ParcelCharacter.h"
#include "Character/ParcelHeroComponent.h"
#include "Character/ParcelPlayerStateComponent.h"
#include "Character/CharacterCarryComponent.h"
#include "Components/DFStatusEffectComponent.h"
#include "Core/HealthComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogRagdoll);

URagdollComponent::URagdollComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URagdollComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
       RAGDOLL_LOG(Warning, TEXT("Owner is not a Character."));
       return;
    }

    // 복구용 기본 트랜스폼 데이터 저장
    if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
    {
       DefaultMeshRelativeLocation = Mesh->GetRelativeLocation();
       DefaultMeshRelativeRotation = Mesh->GetRelativeRotation();
    }
}

void URagdollComponent::StartRagdoll()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(true);
       return;
    }
    Multicast_SetRagdoll(true);
}

void URagdollComponent::StopRagdoll()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(false);
       return;
    }
    Multicast_SetRagdoll(false);
}

void URagdollComponent::ToggleRagdoll()
{
    bool bCurrentlyRagdoll = IsRagdoll();

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(!bCurrentlyRagdoll);
       return;
    }
    Multicast_SetRagdoll(!bCurrentlyRagdoll);
}

bool URagdollComponent::IsRagdoll() const
{
    // 완전히 게임플레이 태그 판단 방식으로 일원화 (bIsRagdoll 변수 제거)
    if (const AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(GetOwner()))
    {
       if (const UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
       {
          return StateComp->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll")));
       }
    }
    return false;
}

bool URagdollComponent::IsRagdollCloseToGround() const
{
    if (!OwnerCharacter || !GetWorld()) return true;

    const USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh();
    if (!MeshComponent) return true;

    const FVector TraceStart = MeshComponent->GetSocketLocation(TEXT("pelvis"));
    const FVector TraceEnd = TraceStart - (FVector::UpVector * RagdollStopGroundTraceDistance);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    // 바닥이 트레이스 범위 내에 부딪히는지 여부 반환
    return GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);
}

void URagdollComponent::PlayGetUpAnimation(bool bFront)
{
    if (!OwnerCharacter) return;

    if (UAnimMontage* SelectedMontage = GetSelectedGetUpMontage(bFront))
    {
       OwnerCharacter->PlayAnimMontage(SelectedMontage);
    }
}

void URagdollComponent::PlayLandRollAnimation()
{
    if (!OwnerCharacter) return;

    if (UAnimMontage* SelectedMontage = LandRollDefault)
    {
       OwnerCharacter->PlayAnimMontage(SelectedMontage);
    }
}

UAnimMontage* URagdollComponent::GetSelectedGetUpMontage(bool bFront) const
{
    return bFront ? GetUpFrontDefault : GetUpBackDefault;
}

void URagdollComponent::ServerSetRagdoll_Implementation(bool bNewIsRagdoll)
{
    Multicast_SetRagdoll(bNewIsRagdoll);
}

void URagdollComponent::Multicast_SetRagdoll_Implementation(bool bNewIsRagdoll)
{
    if (bNewIsRagdoll) ApplyStartRagdoll();
    else               ApplyStopRagdoll();
}

void URagdollComponent::AttemptAutoRecovery()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !OwnerCharacter) return;

    // 사망 상태면 자동 기상 스킵 — 부활 타이머가 대신 처리
    if (UHealthComponent* HC = OwnerCharacter->FindComponentByClass<UHealthComponent>())
    {
        if (HC->IsDead()) return;
    }

    // 상태이상이 활성화 중이면 2초 후 재시도
    if (UDFStatusEffectComponent* StatusComp = OwnerCharacter->FindComponentByClass<UDFStatusEffectComponent>())
    {
        if (StatusComp->HasActiveStatusEffect())
        {
            GetWorld()->GetTimerManager().SetTimer(
                AutoRecoveryTimerHandle,
                this,
                &URagdollComponent::AttemptAutoRecovery,
                2.0f,
                false
            );
            return;
        }
    }

    StopRagdoll();
}

void URagdollComponent::ApplyStartRagdoll()
{
    if (!OwnerCharacter)
    {
       RAGDOLL_LOG(Warning, TEXT("OwnerCharacter is missing."));
       return;
    }

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh || !Mesh->GetPhysicsAsset())
    {
       RAGDOLL_LOG(Warning, TEXT("Mesh or Physics Asset is missing."));
       return;
    }

    // 1. 컴포넌트 기능 제어 및 콜리전 설정
    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
       Movement->DisableMovement();
    }

    if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
    {
       Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 2. 메시 물리 시뮬레이션 활성화
    Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);
    Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 1.0f, false, true);
    Mesh->WakeAllRigidBodies();

    // 3. [서버 권한] 게임플레이 태그 적용 및 스프린트 강제 해제 방어 코드
    if (GetOwner()->HasAuthority())
    {
        if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
        {
            if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
            {
                StateComp->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll")));
                StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Sprinting")));
            }
        }

        // 래그돌 진입 시 들고 있던 상자 강제 드롭
        if (UCharacterCarryComponent* CarryComp = OwnerCharacter->FindComponentByClass<UCharacterCarryComponent>())
        {
            if (CarryComp->IsCarrying())
                CarryComp->Drop();
        }

        // 상태이상이 없으면 AutoRecoveryDelay 초 후 강제 기상 시도
        GetWorld()->GetTimerManager().SetTimer(
            AutoRecoveryTimerHandle,
            this,
            &URagdollComponent::AttemptAutoRecovery,
            AutoRecoveryDelay,
            false
        );
    }

    // 4. AnimBP 상태 갱신 — Multicast로 모든 클라이언트에서 호출되므로 직접 설정
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
    {
        ParcelChar->SetRagdollState(true, false);
    }

    // 5. 카메라 제어권 변경 — 로컬 플레이어만 적용
    if (OwnerCharacter->IsLocallyControlled())
    {
        if (UParcelHeroComponent* HeroComp = OwnerCharacter->FindComponentByClass<UParcelHeroComponent>())
            HeroComp->EnterRagdollCameraMode();
    }

    RAGDOLL_LOG(Log, TEXT("Ragdoll started successfully."));
}

void URagdollComponent::ApplyStopRagdoll()
{
    if (!OwnerCharacter) return;

    // 사망 상태면 래그돌 해제 불가 — 부활이 대신 처리
    if (UHealthComponent* HC = OwnerCharacter->FindComponentByClass<UHealthComponent>())
    {
        if (HC->IsDead()) return;
    }

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
    if (!Mesh || !Capsule) return;

    // 1. 물리 종료 전 골반 위치 파악 및 지면 보정 연산
    FVector PelvisLocation = Mesh->GetSocketLocation(TEXT("pelvis"));
    float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

    FVector TraceStart = PelvisLocation;
    FVector TraceEnd = PelvisLocation + FVector(0.f, 0.f, -500.f);
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    float GroundZ = PelvisLocation.Z;
    if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
       GroundZ = HitResult.ImpactPoint.Z;
    }

    // 2. 기상을 위해 현재 포즈 스냅샷 저장
    if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
    {
       AnimInstance->SavePoseSnapshot(TEXT("RagdollPose"));
    }

    // 3. 물리 시뮬레이션 일제히 비활성화
    Mesh->SetSimulatePhysics(false);
    Mesh->SetAllBodiesSimulatePhysics(false);
    Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), false, true);
    Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 0.0f, false, true);
    Mesh->bBlendPhysics = false;
    Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // 4. 캐릭터 기본 기상 위치 텔레포트 및 메시 원복
    FVector NewActorLocation = FVector(PelvisLocation.X, PelvisLocation.Y, GroundZ + CapsuleHalfHeight);
    OwnerCharacter->SetActorLocation(NewActorLocation, false, nullptr, ETeleportType::TeleportPhysics);

    Mesh->SetRelativeLocation(DefaultMeshRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Mesh->SetRelativeRotation(DefaultMeshRelativeRotation, false, nullptr, ETeleportType::TeleportPhysics);

    // 5. 콜리전 재설정 및 이동 컴포넌트 걷기 모드로 복구
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
       Movement->SetMovementMode(MOVE_Walking);
    }
    
    // 6. [서버 권한] 기절 상태 완료 태그 제거 및 자동 기상 타이머 정리
    if (GetOwner()->HasAuthority())
    {
       if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
       {
          if (UParcelPlayerStateComponent* StateComp = ParcelChar->GetParcelPlayerStateComponent())
          {
             StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Ragdoll")));
             // 래그돌 중 Landed()가 호출되지 않아 InAir 태그가 잔류하는 경우 강제 제거
             StateComp->RemoveStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.InAir")));
          }
       }
       GetWorld()->GetTimerManager().ClearTimer(AutoRecoveryTimerHandle);
    }

    // 7. AnimBP 상태 갱신
    if (AParcelCharacter* ParcelChar = Cast<AParcelCharacter>(OwnerCharacter))
    {
        ParcelChar->SetRagdollState(false, true);
    }

    // 8. 카메라 원위치 복구 — 로컬 플레이어만 적용
    if (OwnerCharacter->IsLocallyControlled())
    {
        if (UParcelHeroComponent* HeroComp = OwnerCharacter->FindComponentByClass<UParcelHeroComponent>())
            HeroComp->ExitRagdollCameraMode();
    }

    // 9. 기상 애니메이션 실행 (앞면 디폴트)
    PlayGetUpAnimation(true);
    
    RAGDOLL_LOG(Log, TEXT("Ragdoll stopped successfully."));
}