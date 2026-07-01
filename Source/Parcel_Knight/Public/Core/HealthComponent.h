// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HealthInterface.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);


/**
 * 체력 관리 컴포넌트
 * IHealthInterface를 구현하며, 플레이어·적·파손 가능 오브젝트 등에 부착된다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UHealthComponent : public UActorComponent, public IHealthInterface
{
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 사망 시 발동 — 플레이어/박스 등 소유자가 각자의 사망 처리를 바인딩
	UPROPERTY(BlueprintAssignable)
	FOnDeathSignature OnDeathDelegate;
	
	// IHealthInterface 구현
	virtual float GetHP() const override;
	virtual float GetMaxHP() const override;
	virtual void AddHP(float Amount) override;
	virtual void TakeDamage(float Amount) override;
	virtual void OnDeath() override;
	virtual bool IsDead() const override;

private:
	// 복제 — 클라이언트 체력바 갱신용
	UPROPERTY(Replicated)
	float HP;

	// 초기/최대 체력 — 추후 DataAsset 등에서 설정 가능
	UPROPERTY(EditDefaultsOnly)
	float MaxHP;

	/*
	 *사망 여부 — 중복 사망 처리 방지용
	 *NOTE: 사망코드는 대상 클래스에서 구현
	 */
	UPROPERTY(Replicated)
	bool bIsDead;
	
};
