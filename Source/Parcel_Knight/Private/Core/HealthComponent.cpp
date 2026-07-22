// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	BaseMaxHP = 100.f;
	MaxHP = BaseMaxHP;
	HP = 0.f;
	bIsDead = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InitializeHP(MaxHP);
	}
}

float UHealthComponent::GetBaseMaxHP() const
{
	return BaseMaxHP;
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
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead) return;
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

void UHealthComponent::IncreaseMaxHP(float Amount)
{
	if (!GetOwner()->HasAuthority()) return;
	MaxHP += Amount;
	HP = FMath::Clamp(HP + Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(HP, MaxHP);
}

void UHealthComponent::SetMaxHPPreservingRatio(float InMaxHP)
{
	if (!GetOwner()->HasAuthority()) return;

	const float HealthRatio = MaxHP > KINDA_SMALL_NUMBER
		? FMath::Clamp(HP / MaxHP, 0.f, 1.f)
		: (bIsDead ? 0.f : 1.f);

	MaxHP = FMath::Max(1.f, InMaxHP);
	HP = bIsDead ? 0.f : MaxHP * HealthRatio;
	OnHPChanged.Broadcast(HP, MaxHP);
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

