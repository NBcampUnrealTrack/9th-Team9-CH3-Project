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
	
	// [Server] 스탯 변경을 위한 세터
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Stats|Movement")
	void SetMaxWalkSpeed(float NewSpeed);

	// 스탯 조회를 위한 게터
	FORCEINLINE float GetMaxWalkSpeed() const { return MaxWalkSpeed; }
	FORCEINLINE float GetJumpZVelocity() const { return JumpZVelocity; }
	FORCEINLINE float GetAirControl() const { return AirControl; }
	FORCEINLINE FRotator GetRotationRate() const { return RotationRate; }

private:
	// 클라이언트 값이 복제되어 바뀔 때 무브먼트 컴포넌트에 동기화해주는 RepNotify 함수
	UFUNCTION() void OnRep_MaxWalkSpeed();
	UFUNCTION() void OnRep_JumpZVelocity();
	UFUNCTION() void OnRep_AirControl();
	UFUNCTION() void OnRep_RotationRate();

	// 실제 캐릭터 무브먼트 컴포넌트에 현재 스탯값들을 주입하는 내부 함수
	void ApplyStatsToMovement();
	
	// 서버가 통제하고 클라이언트에 복제되는 캐릭터 무브먼트 데이터
	UPROPERTY(ReplicatedUsing = OnRep_MaxWalkSpeed, EditAnywhere, Category = "Stats|Movement")
	float MaxWalkSpeed;

	UPROPERTY(ReplicatedUsing = OnRep_JumpZVelocity, EditAnywhere, Category = "Stats|Movement")
	float JumpZVelocity;

	UPROPERTY(ReplicatedUsing = OnRep_AirControl, EditAnywhere, Category = "Stats|Movement")
	float AirControl;

	UPROPERTY(ReplicatedUsing = OnRep_RotationRate, EditAnywhere, Category = "Stats|Movement")
	FRotator RotationRate;
};