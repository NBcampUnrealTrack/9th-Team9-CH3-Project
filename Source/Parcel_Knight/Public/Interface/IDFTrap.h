#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "IDFTrap.generated.h"

UINTERFACE(BlueprintType)
class PARCEL_KNIGHT_API UIDFTrap : public UInterface
{
	GENERATED_BODY()
};

class PARCEL_KNIGHT_API IIDFTrap
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Trap")
	FGameplayTag GetTrapStateTag() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Trap")
	bool IsTrapReady() const;
};
