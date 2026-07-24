#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PhysicsJudgeManager.generated.h"

class ADeliveryBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoxDamaged, ADeliveryBox*, DamagedBox);

UCLASS()
class PARCEL_KNIGHT_API UPhysicsJudgeManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Physics")
	FOnBoxDamaged OnBoxDamaged;

	void EvaluateImpact(ADeliveryBox* Box, float ImpactForce);

	void EvaluateTrapImpact(ADeliveryBox* Box, float TrapImpactForce);

private:
	void ProcessBoxDamage(ADeliveryBox* Box, float Force, const FString& SourceName);
};