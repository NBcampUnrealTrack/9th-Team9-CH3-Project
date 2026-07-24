#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHealthInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 체력 시스템 인터페이스
 *
 * 플레이어·적·함정 등 체력을 가진 모든 액터에 적용한다.
 * 체력 관련 로직을 클래스 종류와 무관하게 일관되게 호출하기 위해 인터페이스로 분리.
 * 
 * 담당자: 한수현
 */
class PARCEL_KNIGHT_API IHealthInterface
{
	GENERATED_BODY()

public:
	// [All] HP 조회
	virtual float GetHP() const = 0;
	// [All] 최대 HP 조회
	virtual float GetMaxHP() const = 0;
	
	/**
	 * @param Amount 회복량 — 구현 측에서 MaxHP 클램프 필요
	 * [Server Only] 클라이언트에서 호출 시 HasAuthority() 체크 후 무시
	 */
	virtual void AddHP(float Amount) = 0;
	
	/**
 * @param Amount 감소량 — 결과는 Replicated HP로 클라이언트에 자동 전파
 * [Server Only] 클라이언트에서 호출 시 HasAuthority() 체크 후 무시
 */
	virtual void TakeDamage(float Amount) = 0;
	
	/**
 * HP가 0이 됐을 때 호출 — 구현 측에서 Multicast RPC로 사망 연출 전파
 * [Server Only] 래그돌·리스폰 등 사망 처리는 서버에서만 시작
 */
	virtual void OnDeath() = 0;
	
	// [All] 사망 여부
	virtual bool IsDead() const = 0;
};
