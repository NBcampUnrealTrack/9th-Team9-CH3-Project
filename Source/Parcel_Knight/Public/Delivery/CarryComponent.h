#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarryComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCarryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:	
	// 상자가 들려질 수 있는지 검사
	bool CanCarry(AActor* Carrier) const;

	// 상자가 집어졌을 때 상태 변경
	void OnPickedUp(AActor* Carrier);

	// 상자가 놓여졌을 때 상태 변경
	void OnDropped();

	// Getter
	FORCEINLINE bool IsCarried() const { return bIsCarried; }
	FORCEINLINE AActor* GetCarrier() const { return CarrierActor; }

private:
	UPROPERTY(Replicated)
	bool bIsCarried;

	UPROPERTY(Replicated)
	AActor* CarrierActor;
};