#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeliveryBoxSpawner.generated.h"

class UBoxComponent;
class UArrowComponent;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryBoxSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	ADeliveryBoxSpawner();
	
	void ActivateSpawner(float InInterval);

private:
	// Gamemode가 신호를 주면 리스폰 타이머를 가동시킬 진입점
	void TriggerRandomSpawn();
	
	FTimerHandle SpawnTimerHandle;

protected:
	virtual void BeginPlay() override;

	// 상자 자동 스폰 주기 (초 단위, 기본 5초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery Spawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 5.0f;
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnBoundsVisualizer;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> ForwardArrowVisualizer;
	
};