#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeliveryZone.generated.h"

UENUM(BlueprintType)
enum class EDestinationType : uint8
{
	None,
	ZoneA,
	ZoneB,
	ZoneC,
	Emergency
};

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class PARCEL_KNIGHT_API ADeliveryZone : public AActor
{
	GENERATED_BODY()
	
public:	
	ADeliveryZone();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery | Zone Settings")
	EDestinationType ZoneType;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UBoxComponent> OverlapVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delivery | Components")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

private:
	UFUNCTION()
	void OnZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
					   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
					   bool bFromSweep, const FHitResult& SweepResult);

	void ProcessDelivery(AActor* InBox);
};