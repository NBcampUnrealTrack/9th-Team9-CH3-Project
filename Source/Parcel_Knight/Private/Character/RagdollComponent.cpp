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
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogRagdoll);

URagdollComponent::URagdollComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URagdollComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URagdollComponent, bReplicatedRagdollState);
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

	// Initial replication can arrive before BeginPlay caches the owning character.
	ApplyReplicatedRagdollState(bReplicatedRagdollState);
}

void URagdollComponent::StartRagdoll()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(true);
       return;
    }

	// 권한 경로는 사망·서버 물리 충돌 같은 신뢰 가능한 강제 원인이다.
	const bool bUpgradingClientRagdollToForced = bReplicatedRagdollState && bClientInitiatedRagdoll;
	bClientInitiatedRagdoll = false;
	if (bUpgradingClientRagdollToForced && GetWorld())
	{
		AirRecoveryWaitElapsed = 0.0f;
		GetWorld()->GetTimerManager().SetTimer(
			AutoRecoveryTimerHandle,
			this,
			&URagdollComponent::AttemptAutoRecovery,
			AutoRecoveryDelay,
			false);
	}

	SetRagdollState_ServerOnly(true);
}

void URagdollComponent::StopRagdoll()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(false);
       return;
    }
	bClientInitiatedRagdoll = false;
	SetRagdollState_ServerOnly(false);
}

void URagdollComponent::ToggleRagdoll()
{
    const bool bCurrentlyRagdoll = IsRagdoll();

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
       ServerSetRagdoll(!bCurrentlyRagdoll);
       return;
    }

	HandleClientRagdollRequest_ServerOnly(!bCurrentlyRagdoll);
}

bool URagdollComponent::TryConsumeToggleCooldown()
{
    if (bRagdollOnCooldown) return false;

    bRagdollOnCooldown = true;
    GetWorld()->GetTimerManager().SetTimer(
        RagdollCooldownTimerHandle,
        this,
        &URagdollComponent::ResetRagdollCooldown_ServerOnly,
        RagdollCooldown,
        false
    );
    return true;
}

bool URagdollComponent::IsRagdoll() const
{
	return bRagdollAppliedLocally;
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
	HandleClientRagdollRequest_ServerOnly(bNewIsRagdoll);
}

void URagdollComponent::Multicast_SetRagdoll_Implementation(bool bNewIsRagdoll)
{
	ApplyReplicatedRagdollState(bNewIsRagdoll);
}

void URagdollComponent::OnRep_RagdollState()
{
	ApplyReplicatedRagdollState(bReplicatedRagdollState);
}

void URagdollComponent::HandleClientRagdollRequest_ServerOnly(bool bRequestedRagdoll)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !OwnerCharacter)
	{
		return;
	}

	if (bRequestedRagdoll == bReplicatedRagdollState)
	{
		return;
	}

	if (UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return;
		}
	}

	if (bRequestedRagdoll)
	{
		if (bRagdollOnCooldown)
		{
			return;
		}

		bClientInitiatedRagdoll = true;
		SetRagdollState_ServerOnly(true);
		return;
	}

	if (!bClientInitiatedRagdoll || !IsRagdollCloseToGround())
	{
		return;
	}

	if (UDFStatusEffectComponent* StatusComponent = OwnerCharacter->FindComponentByClass<UDFStatusEffectComponent>())
	{
		if (StatusComponent->HasActiveStatusEffect())
		{
			return;
		}
	}

	bClientInitiatedRagdoll = false;
	SetRagdollState_ServerOnly(false);
}

void URagdollComponent::SetRagdollState_ServerOnly(bool bNewIsRagdoll)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bReplicatedRagdollState == bNewIsRagdoll)
	{
		return;
	}

	bReplicatedRagdollState = bNewIsRagdoll;
	GetOwner()->ForceNetUpdate();
	Multicast_SetRagdoll(bNewIsRagdoll);
}

void URagdollComponent::ApplyReplicatedRagdollState(bool bNewIsRagdoll)
{
	if (bRagdollAppliedLocally == bNewIsRagdoll)
	{
		return;
	}

	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh || (bNewIsRagdoll && !Mesh->GetPhysicsAsset()))
	{
		return;
	}

	if (!bNewIsRagdoll)
	{
		if (!OwnerCharacter->GetCapsuleComponent())
		{
			return;
		}

		if (const UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
		{
			if (HealthComponent->IsDead())
			{
				return;
			}
		}
	}

	bRagdollAppliedLocally = bNewIsRagdoll;
	if (bNewIsRagdoll)
	{
		ApplyStartRagdoll();
	}
	else
	{
		ApplyStopRagdoll();
	}
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

    // 공중에 있으면 땅에 닿을 때까지 0.5초마다 재시도 (MaxAirRecoveryWait를 넘기면 공중이어도 강제 기상)
    if (!IsRagdollCloseToGround())
    {
        AirRecoveryWaitElapsed += 0.5f;
        if (AirRecoveryWaitElapsed < MaxAirRecoveryWait)
        {
            GetWorld()->GetTimerManager().SetTimer(
                AutoRecoveryTimerHandle,
                this,
                &URagdollComponent::AttemptAutoRecovery,
                0.5f,
                false
            );
            return;
        }

        RAGDOLL_LOG(Log, TEXT("공중 대기 시간(%.1f초) 초과로 강제 기상합니다."), MaxAirRecoveryWait);
    }

    AirRecoveryWaitElapsed = 0.f;
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

	if (GetOwner()->HasAuthority() && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoveryLockTimerHandle);
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
    Mesh->SetSimulatePhysics(true);
    Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"), true, true);
    Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"), 1.0f, false, true);
    Mesh->bBlendPhysics = true;
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
        AirRecoveryWaitElapsed = 0.f;
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
        // 이전 래그돌의 지연된 카메라 복귀가 예약돼 있다면 취소 (다시 래그돌 중인데 1인칭으로 튀는 것 방지)
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(CameraRecoveryTimerHandle);
        }

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
    // 캡슐 콜리전을 먼저 복구해야 FindTeleportSpot이 바닥/벽과의 겹침을 실제로 감지해서 보정할 수 있다.
    // (콜리전이 NoCollision인 채로 텔레포트하면 겹쳐도 감지가 안 되고, 그대로 파묻힌 채 콜리전만 나중에 켜짐)
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    FVector NewActorLocation = FVector(PelvisLocation.X, PelvisLocation.Y, GroundZ + CapsuleHalfHeight);
    if (GetWorld())
    {
        // 지면/벽에 파묻힌 위치로 계산됐어도 겹치지 않는 가장 가까운 위치로 보정 시도
        GetWorld()->FindTeleportSpot(OwnerCharacter, NewActorLocation, OwnerCharacter->GetActorRotation());
    }
    OwnerCharacter->SetActorLocation(NewActorLocation, false, nullptr, ETeleportType::TeleportPhysics);

    Mesh->SetRelativeLocation(DefaultMeshRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Mesh->SetRelativeRotation(DefaultMeshRelativeRotation, false, nullptr, ETeleportType::TeleportPhysics);

    // 5. 이동 컴포넌트 복구 (RecoveryLockDuration 초 후 이동 가능)
    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
        Movement->DisableMovement();
    }

    if (GetOwner()->HasAuthority())
    {
        GetWorld()->GetTimerManager().SetTimer(
            RecoveryLockTimerHandle,
            this,
            &URagdollComponent::FinishRecoveryLock_ServerOnly,
            RecoveryLockDuration,
            false
        );

        bRagdollOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            RagdollCooldownTimerHandle,
            this,
            &URagdollComponent::ResetRagdollCooldown_ServerOnly,
            RagdollCooldown,
            false
        );
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

    // 8. 카메라 복구 — 로컬 플레이어만 적용.
    // 캐릭터에 다시 붙여서 3인칭으로 따라가게 하는 건 즉시 처리(카메라가 허공에 멈춰있지 않도록),
    // 1인칭으로의 최종 전환은 이동 잠금이 풀리는 시점(RecoveryLockDuration)에 맞춰 지연시킨다.
    if (OwnerCharacter->IsLocallyControlled())
    {
        if (UParcelHeroComponent* HeroComp = OwnerCharacter->FindComponentByClass<UParcelHeroComponent>())
        {
            HeroComp->ReattachCameraAfterRagdoll();
        }

        GetWorld()->GetTimerManager().SetTimer(
            CameraRecoveryTimerHandle,
            this,
            &URagdollComponent::FinishCameraRecovery_LocalOnly,
            RecoveryLockDuration,
            false
        );
    }

    // 9. 기상 애니메이션 실행 (앞면 디폴트)
    PlayGetUpAnimation(true);
    
    RAGDOLL_LOG(Log, TEXT("Ragdoll stopped successfully."));
}

void URagdollComponent::FinishRecoveryLock_ServerOnly()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !OwnerCharacter || bReplicatedRagdollState)
	{
		return;
	}

	if (const UHealthComponent* HealthComponent = OwnerCharacter->FindComponentByClass<UHealthComponent>())
	{
		if (HealthComponent->IsDead())
		{
			return;
		}
	}

	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void URagdollComponent::ResetRagdollCooldown_ServerOnly()
{
	bRagdollOnCooldown = false;
}

void URagdollComponent::FinishCameraRecovery_LocalOnly()
{
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	if (UParcelHeroComponent* HeroComp = OwnerCharacter->FindComponentByClass<UParcelHeroComponent>())
	{
		HeroComp->ExitRagdollCameraMode();
	}
}
