#pragma once

// 엔진 기본 타입과 매크로를 포함합니다.
#include "CoreMinimal.h"

// UActorComponent를 상속받기 위해 필요한 헤더입니다.
#include "Components/ActorComponent.h"

// UnrealHeaderTool이 생성하는 리플렉션 코드를 포함합니다.
#include "RagdollComponent.generated.h"

// 래그돌 컴포넌트 전용 로그 카테고리입니다.
DECLARE_LOG_CATEGORY_EXTERN(LogRagdoll, Log, All);

// 헤더 의존성을 줄이기 위한 전방 선언입니다.
class ACharacter;

// 캐릭터의 래그돌 시작/종료 상태를 관리하고 네트워크로 복제하는 컴포넌트입니다.
// 클라이언트에서 호출해도 서버 RPC를 통해 서버가 최종 상태를 결정합니다.
//
// 담당자: 김로운
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API URagdollComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Tick 설정과 컴포넌트 복제 기본값을 초기화합니다.
	URagdollComponent();

	// 복제할 프로퍼티를 Unreal 네트워크 시스템에 등록합니다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 래그돌을 시작합니다.
	// 클라이언트에서 호출하면 서버에 요청하고, 서버에서는 바로 적용합니다.
	UFUNCTION(BlueprintCallable)
	void StartRagdoll();

	// 래그돌을 종료합니다.
	// 클라이언트에서 호출하면 서버에 요청하고, 서버에서는 바로 적용합니다.
	UFUNCTION(BlueprintCallable)
	void StopRagdoll();

	// 현재 래그돌 상태에 따라 시작 또는 종료를 전환합니다.
	UFUNCTION(BlueprintCallable)
	void ToggleRagdoll();

	// 현재 래그돌 상태를 반환합니다.
	UFUNCTION(BlueprintCallable)
	bool IsRagdoll() const;

protected:
	// 게임 시작 시 소유 액터를 캐릭터로 캐싱합니다.
	virtual void BeginPlay() override;

private:
	// 클라이언트가 래그돌 상태 변경을 서버에 요청할 때 사용하는 RPC입니다.
	UFUNCTION(Server, Reliable)
	void ServerSetRagdoll(bool bNewIsRagdoll);

	// bIsRagdoll 값이 클라이언트에 복제될 때 호출됩니다.
	UFUNCTION()
	void OnRep_IsRagdoll();

	// 실제 래그돌 시작 처리를 적용합니다.
	void ApplyStartRagdoll();

	// 실제 래그돌 종료 처리를 적용합니다.
	void ApplyStopRagdoll();

	// 이 컴포넌트를 소유한 캐릭터입니다.
	UPROPERTY()
	ACharacter* OwnerCharacter;

	// 서버에서 결정하고 클라이언트로 복제되는 래그돌 상태입니다.
	UPROPERTY(ReplicatedUsing = OnRep_IsRagdoll)
	bool bIsRagdoll = false;
};
