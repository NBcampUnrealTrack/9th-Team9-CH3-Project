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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Carryable")
	bool CanCarry(AActor* Carrier);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Carryable")
	void OnPickedUp(AActor* Carrier);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Carryable")
	void OnDropped();
};