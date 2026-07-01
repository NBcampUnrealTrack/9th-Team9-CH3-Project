#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DeliverySubsystem.generated.h"

UCLASS()
class PARCEL_KNIGHT_API UDeliverySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UDeliverySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	AActor* SpawnBox(FName BoxTypeID, FVector SpawnLocation, FRotator SpawnRotation);

	void DespawnBox(AActor* Box);

	int32 GenerateBoxID();

private:

	UPROPERTY()
	TArray<AActor*> ActiveBoxes;

	int32 NextBoxID;
};