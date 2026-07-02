// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CharacterCarryComponent.h"
#include "Components/ActorComponent.h" 
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Delivery/Carryable.h"
#include "Delivery/PhysicsJudgeManager.h"

UCharacterCarryComponent::UCharacterCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CarriedBox = nullptr;
	bIsCarrying = false;
	MoveSpeedMultiplier = 1.0f;
}

void UCharacterCarryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCharacterCarryComponent::Pickup(ADeliveryBox* InBox)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !InBox) return;

	CarriedBox = InBox;
	bIsCarrying = true;
	MoveSpeedMultiplier = InBox->GetBoxData().MoveSpeedMultiplier;

	// 상자를 캐릭터 스켈레탈 메시의 HandSocket에 부착
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	InBox->AttachToComponent(OwnerCharacter->GetMesh(), AttachmentRules, TEXT("HandSocket"));

	// 캐릭터 이동 속도 제한 적용
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 450.f * MoveSpeedMultiplier; // 기본 걷기 속도 450.f 기준으로 속도 적용
	}
}

void UCharacterCarryComponent::Drop()
{
	if (!CarriedBox) return;

	// 서버 권한인 경우 상자 자체의 상태 복구 및 물리 시뮬레이션 재활성화 호출
	if (GetOwner()->HasAuthority())
	{
		if (ICarryable* Carryable = Cast<ICarryable>(CarriedBox))
		{
			Carryable->OnDropped();
		}
	}

	// 캐릭터 부착 해제
	CarriedBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 캐릭터 이동 속도 원상복구
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = 450.f; // 기본 걷기 속도 복구
		}
	}

	CarriedBox = nullptr;
	bIsCarrying = false;
}

void UCharacterCarryComponent::Throw(FVector Force)
{
	if (!CarriedBox) return;

	if (GetOwner()->HasAuthority())
	{
		AActor* DroppedBox = CarriedBox;

		// 1. 소유권 해제 및 물리 시뮬레이션 복구
		if (ICarryable* Carryable = Cast<ICarryable>(DroppedBox))
		{
			Carryable->OnDropped();
		}

		// 2. 캐릭터 부착 해제
		DroppedBox->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 3. 상자의 루트 물리 컴포넌트에 힘(임펄스) 가하기
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(DroppedBox->GetRootComponent()))
		{
			RootPrim->AddImpulse(Force, NAME_None, true); // true = 질량을 무시하고 속도 변화로 던짐
		}

		// 4. 캐릭터 이동 속도 원상복구
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter)
		{
			if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = 450.f;
			}
		}

		CarriedBox = nullptr;
		bIsCarrying = false;
	}
	else
	{
		// 클라이언트는 서버 RPC 요청
		Server_Throw(Force);
	}
}

void UCharacterCarryComponent::ForceDropByTrap(float TrapDamage)
{
	if (!CarriedBox) return;

	ADeliveryBox* BoxActor = CarriedBox;

	// 1. 상자를 떨어뜨림
	Drop();

	// 2. 떨어뜨린 상자에게 함정 대미지를 부여하여 파손 판정 매니저가 처리하도록 위임 (서버 전용)
	if (GetOwner()->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPhysicsJudgeManager* JudgeManager = World->GetSubsystem<UPhysicsJudgeManager>())
			{
				JudgeManager->EvaluateTrapImpact(BoxActor, TrapDamage);
			}
		}
	}
}

bool UCharacterCarryComponent::Server_Throw_Validate(FVector Force)
{
	return true;
}

void UCharacterCarryComponent::Server_Throw_Implementation(FVector Force)
{
	Throw(Force);
}
