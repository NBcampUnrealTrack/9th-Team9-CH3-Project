#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DeliveryZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ADeliveryBox;
class UTextRenderComponent;
class USoundBase;
class USoundAttenuation;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryZone : public AActor
{
	GENERATED_BODY()
    
public: 
	ADeliveryZone();

public:
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery | Zone Settings")
	FGameplayTag ZoneTag;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UStaticMeshComponent> TruckMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UTextRenderComponent> ZoneTextVisualizer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UBoxComponent> OverlapVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

public:
	// 배송 성공 시 재생할 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery | Sound Settings")
	TObjectPtr<USoundBase> SuccessSound;

	// 오배송(실패) 시 재생할 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery | Sound Settings")
	TObjectPtr<USoundBase> FailureSound;

	// 배송 성공/실패 사운드용 거리 감쇄 설정 (소리 크기 및 전파 범위 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Delivery | Sound Settings")
	TObjectPtr<USoundAttenuation> DeliverySoundAttenuation;

private:
	// Server : Overlap 이벤트 콜백 함수
	UFUNCTION()
	void OnZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
				   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
				   bool bFromSweep, const FHitResult& SweepResult);

	void ProcessDelivery(ADeliveryBox* Box);

	// 모든 클라이언트에서 성공/실패 3D 사운드를 재생하기 위한 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeliverySound(bool bSuccess);
};