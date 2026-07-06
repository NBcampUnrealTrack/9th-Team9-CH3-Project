#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IDFActivatableTrap.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class PARCEL_KNIGHT_API UIDFActivatableTrap : public UInterface
{
	GENERATED_BODY()
};

class PARCEL_KNIGHT_API IIDFActivatableTrap
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Trap")
	bool RequestActivate(AActor* Activator);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Trap")
	bool CanActivate(AActor* Activator) const;
};
