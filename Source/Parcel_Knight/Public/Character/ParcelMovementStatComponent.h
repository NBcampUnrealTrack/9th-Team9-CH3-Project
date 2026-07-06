#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ParcelMovementStatComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UParcelMovementStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UParcelMovementStatComponent();

	// 멀티플레이어 동기화
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void BeginPlay() override;
	
	// [Server/Client] 상태 변화에 따른 속도 재계산 인터페이스 (예측 동기화 지원)
	void UpdateDynamicSpeedModifier(bool bInSprinting, float InCarryMultiplier);
	
	// [Server] 스탯 변경을 위한 세터
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Stats|Movement")
	void SetBaseMaxWalkSpeed(float NewSpeed);

	// 스탯 조회를 위한 게터
	FORCEINLINE float GetMaxWalkSpeed() const { return MaxWalkSpeed; }
	FORCEINLINE float GetJumpZVelocity() const { return JumpZVelocity; }
	FORCEINLINE float GetAirControl() const { return AirControl; }

private:
	// 클라이언트 값이 복제되어 바뀔 때 무브먼트 컴포넌트에 동기화해주는 RepNotify 함수
	UFUNCTION() void OnRep_MaxWalkSpeed();
	UFUNCTION() void OnRep_JumpZVelocity();
	UFUNCTION() void OnRep_AirControl();
	UFUNCTION() void OnRep_RotationRate();
	
	void ApplyStatsToMovement();

	// 속도 계산을 위한 기본 속도 스탯
	UPROPERTY(EditDefaultsOnly, Category = "Stats|Movement")
	float BaseMaxWalkSpeed;
	
	UPROPERTY(EditDefaultsOnly, Category = "Stats|Movement")
	float SprintSpeedMultiplier;
	
	// 런타임에 계산되어 복제되는 속도 데이터
	UPROPERTY(ReplicatedUsing = OnRep_MaxWalkSpeed, VisibleAnywhere, Category = "Stats|Movement")
	float MaxWalkSpeed;

	UPROPERTY(ReplicatedUsing = OnRep_JumpZVelocity, EditAnywhere, Category = "Stats|Movement")
	float JumpZVelocity;

	UPROPERTY(ReplicatedUsing = OnRep_AirControl, EditAnywhere, Category = "Stats|Movement")
	float AirControl;

	UPROPERTY(ReplicatedUsing = OnRep_RotationRate, EditAnywhere, Category = "Stats|Movement")
	FRotator RotationRate;
	
	// 로컬 예측 및 서버 계산용 동적 상태 기록 상태 변수
	bool bIsSprinting;
	float CurrentCarryMultiplier;
};