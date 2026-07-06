#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CarryableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCarryableInterface : public UInterface { GENERATED_BODY() };

class PARCEL_KNIGHT_API ICarryableInterface
{
	GENERATED_BODY()

public:
	virtual bool CanCarry(AActor* Carrier) = 0;

	virtual void OnPickedUp(AActor* Carrier) = 0;


	virtual void OnDropped() = 0;
};