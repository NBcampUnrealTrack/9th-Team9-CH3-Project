#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ParcelInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionFocusChanged, AActor*, NewFocusedActor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UParcelInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public: 
	UParcelInteractionComponent();
	
	virtual void BeginPlay() override;
	
	void PrimaryInteract();

	// 현재 조준 중인 대상을 외부에서 가져갈 수 있는 게터
	FORCEINLINE AActor* GetCurrentFocusedActor() const { return CurrentFocusedActor; }

	// [UI] 델리게이트 변수
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionFocusChanged OnFocusChanged;

protected:

	// [Server] 상호작용을 안전하게 검증하고 실행할 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestPrimaryInteract(AActor* TargetActor);

private:
	// 0.1초마다 타이머에 의해 주기적으로 호출하는 시선 검사 함수
	void CheckTraceTarget();
	
	// 가비지 컬렉터로부터 포인터를 보호하기 위한 매크로
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentFocusedActor;

	// 조준 레이저 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	float TraceDistance;

	// 0.1초 주기 검사를 관리할 타이머 핸들
	FTimerHandle TraceTimerHandle;

	/** 타이머가 실행될 주기 (0.1초) */
	const float TraceInterval = 0.1f;
};