#pragma once

#include "CoreMinimal.h"

// UActorComponent를 상속받기 위해 필요한 헤더입니다.
#include "Components/ActorComponent.h"

// UnrealHeaderTool이 생성하는 리플렉션 코드를 포함합니다.
#include "RagdollComponent.generated.h"

class ACharacter;
class UAnimMontage;

// 캐릭터의 래그돌 시작/종료 상태를 관리하고 네트워크로 복제하는 컴포넌트입니다.
// 클라이언트에서 호출해도 서버 RPC를 통해 서버가 최종 상태를 결정합니다.
//
// 담당자: 김로운
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API URagdollComponent : public UActorComponent
{
	GENERATED_BODY()

public:
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

	// 캐릭터가 바라보는 방향에 따라 앞/뒤 일어나기 몽타주를 재생합니다.
	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void PlayGetUpAnimation(bool bFront);

	// Plays the default land roll montage.
	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	void PlayLandRollAnimation();

protected:
	virtual void BeginPlay() override;

	// =========================
	// Get Up Animations
	// =========================

	// 땅을 보고 엎드렸을 때 사용하는 기본 일어나기 몽타주입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Get Up Animations", meta = (DisplayName = "Get Up Front Default"))
	TObjectPtr<UAnimMontage> GetUpFrontDefault;

	// 하늘을 보고 누웠을 때 사용하는 기본 일어나기 몽타주입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Get Up Animations", meta = (DisplayName = "Get Up Back Default"))
	TObjectPtr<UAnimMontage> GetUpBackDefault;

	// =========================
	// Roll Animations
	// =========================

	// Default land roll montage.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll Animations", meta = (DisplayName = "Land Roll Default"))
	TObjectPtr<UAnimMontage> LandRollDefault;

	// 스켈레탈 메시의 기본 상대 위치와 회전값을 저장할 변수
	UPROPERTY()
	FVector DefaultMeshRelativeLocation;

	UPROPERTY()
	FRotator DefaultMeshRelativeRotation;

private:
	// 앞/뒤 상태에 맞는 일어나기 몽타주를 반환합니다.
	UAnimMontage* GetSelectedGetUpMontage(bool bFront) const;

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
