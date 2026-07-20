#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeliveryBoxSpawner.generated.h"

class UBoxComponent;
class UArrowComponent;
class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryBoxSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ADeliveryBoxSpawner();

	virtual void Tick(float DeltaTime) override;
	
	void ActivateSpawner(float InInterval);

private:
	// Gamemode가 신호를 주면 리스폰 타이머를 가동시킬 진입점
	void TriggerRandomSpawn();
	
	FTimerHandle SpawnTimerHandle;

	// 모든 클라이언트에서 상자 스폰 3D 효과음을 재생하기 위한 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySpawnSound();

protected:
	virtual void BeginPlay() override;

	// 상자 자동 스폰 주기 (초 단위, 기본 5초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery Spawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 5.0f;

	// 컨베이어 벨트 구동 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery Spawner | Conveyor")
	float ConveyorSpeed = 160.0f;

	// 컨베이어 벨트 밀어주기 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery Spawner | Conveyor")
	bool bEnableConveyor = true;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SpawnerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ConveyorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ConveyorVolume;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnBoundsVisualizer;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> ForwardArrowVisualizer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> ConveyorSoundComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery Spawner | Sound")
	TObjectPtr<USoundBase> ConveyorSoundAsset;

	// 컨베이어 벨트 작동음 및 스폰음용 거리 감쇄 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery Spawner | Sound")
	TObjectPtr<USoundAttenuation> ConveyorSoundAttenuation;

	// 상자 스폰 시 재생할 효과음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery Spawner | Sound")
	TObjectPtr<USoundBase> BoxSpawnSound;
};