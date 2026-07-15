// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Character/CharacterCarryComponent.h"
#include "Delivery/PhysicsJudgeManager.h"
#include "Delivery/DeliveryBox.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	MaxHP = 100.f;
	HP = 100.f;
	bIsDead = false;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, HP);
	DOREPLIFETIME(UHealthComponent, MaxHP);
	DOREPLIFETIME(UHealthComponent, bIsDead);
}

float UHealthComponent::GetHP() const
{
	return HP;
}

float UHealthComponent::GetMaxHP() const
{
	return MaxHP;
}

void UHealthComponent::AddHP(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		HP = FMath::Clamp((HP+Amount), 0.0f, MaxHP);
		OnHPChanged.Broadcast(HP, MaxHP);
	}
}

void UHealthComponent::TakeDamage(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		HP = FMath::Clamp((HP-Amount), 0.0f, MaxHP);
		OnHPChanged.Broadcast(HP, MaxHP);

		// 플레이어가 데미지를 입었을 때, 손에 운반 중인 상자(CarriedBox)가 있다면 동일하게 데미지를 전파합니다.
		if (Amount > 0.0f)
		{
			if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
			{
				if (UCharacterCarryComponent* CarryComponent = OwnerChar->FindComponentByClass<UCharacterCarryComponent>())
				{
					if (CarryComponent->IsCarrying() && CarryComponent->GetCarriedBox())
					{
						if (UWorld* World = GetWorld())
						{
							if (UPhysicsJudgeManager* JudgeManager = World->GetSubsystem<UPhysicsJudgeManager>())
							{
								JudgeManager->EvaluateTrapImpact(CarryComponent->GetCarriedBox(), Amount);
							}
						}
					}
				}
			}
		}

		if (!bIsDead && HP <= 0.0f)
			OnDeath();
	}
}

void UHealthComponent::OnRep_HP()
{
	// 랜선 타고 체력 데이터가 바뀌어 들어왔으니 UI에게 알림!
	OnHPChanged.Broadcast(HP, MaxHP);
}

void UHealthComponent::OnDeath()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	bIsDead = true;
	OnDeathDelegate.Broadcast();
	Multicast_OnDeath();
}

void UHealthComponent::Multicast_OnDeath_Implementation()
{
	// 사망 연출 전파 — 이펙트·사운드는 추후 구현
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

void UHealthComponent::InitializeHP(float InMaxHP)
{
	if (GetOwner()->HasAuthority())
	{
		MaxHP = InMaxHP;
		HP = InMaxHP;
		bIsDead = false;
		OnHPChanged.Broadcast(HP, MaxHP);
	}
}
