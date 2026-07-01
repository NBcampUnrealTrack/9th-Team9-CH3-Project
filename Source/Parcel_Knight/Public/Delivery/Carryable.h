#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Carryable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCarryable : public UInterface { GENERATED_BODY() };

class PARCEL_KNIGHT_API ICarryable
{
	GENERATED_BODY()

public:
	virtual bool CanCarry(AActor* Carrier) = 0;

	virtual void OnPickedUp(AActor* Carrier) = 0;


	virtual void OnDropped() = 0;
};