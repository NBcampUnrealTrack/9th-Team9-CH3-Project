#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Delivery/DeliveryTypes.h"
#include "DeliverySubsystem.generated.h"

class UDataTable;
class UStageData;

UCLASS()
class PARCEL_KNIGHT_API UDeliverySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UDeliverySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void InitializeStage(UStageData* InStageData);
	
	UFUNCTION(BlueprintCallable, Category = "Delivery")
	AActor* SpawnBox(FGameplayTag BoxTypeTag, FVector SpawnLocation, FRotator SpawnRotation);

	UFUNCTION(BlueprintCallable, Category = "Delivery")
	AActor* SpawnRandomBox(FVector SpawnLocation, FRotator SpawnRotation);

	void DespawnBox(AActor* Box);
	void UnregisterBox(AActor* Box);
	int32 GenerateBoxID();

	FORCEINLINE UStageData* GetCurrentStageData() const { return CurrentStageData; }

private:
	UPROPERTY()
	TArray<AActor*> ActiveBoxes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Delivery")
	TObjectPtr<UDataTable> BoxDataTable;

	UPROPERTY()
	TObjectPtr<UStageData> CurrentStageData;

	UPROPERTY()
	TMap<FGameplayTag, FBoxData> CachedBoxData;

	void EnsureCacheLoaded();

	int32 NextBoxID;
};
