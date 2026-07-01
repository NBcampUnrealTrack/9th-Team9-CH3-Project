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

protected:
	virtual void BeginPlay() override;

public:	
	void PickUpBox(AActor* InBox);
	void DropBox();

	void ForceDropByTrap(float TrapDamage);

private:
	UPROPERTY()
	AActor* CurrentCarryingBox;
};