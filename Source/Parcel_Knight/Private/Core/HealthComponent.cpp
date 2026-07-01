// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, HP);
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
	}
}

void UHealthComponent::TakeDamage(float Amount)
{
	if (GetOwner()->HasAuthority())
	{
		HP = FMath::Clamp((HP-Amount), 0.0f, MaxHP);
		if (!bIsDead && HP <= 0.0f)
		OnDeath();
	}
}

void UHealthComponent::OnDeath()
{
	bIsDead = true;
	OnDeathDelegate.Broadcast();
	// TODO: 사망 연출 전파는 추후 Multicast RPC로 확장(2단계)
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}
