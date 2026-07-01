#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface { GENERATED_BODY() };

class PARCEL_KNIGHT_API IInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(AActor* Interactor) = 0;
	
	virtual void Interact(AActor* Interactor) = 0;
};