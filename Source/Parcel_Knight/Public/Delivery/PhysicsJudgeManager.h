#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PhysicsJudgeManager.generated.h"

class ADeliveryBox;

UCLASS()
class PARCEL_KNIGHT_API UPhysicsJudgeManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void EvaluateImpact(ADeliveryBox* Box, float ImpactForce);

	void EvaluateTrapImpact(ADeliveryBox* Box, float TrapImpactForce);
};