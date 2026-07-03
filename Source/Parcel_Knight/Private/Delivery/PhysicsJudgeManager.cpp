#include "Delivery/PhysicsJudgeManager.h"
#include "Delivery/DeliveryBox.h"

void UPhysicsJudgeManager::EvaluateImpact(ADeliveryBox* Box, float ImpactForce)
{
	ProcessBoxDamage(Box, ImpactForce, TEXT("impact force"));
}

void UPhysicsJudgeManager::EvaluateTrapImpact(ADeliveryBox* Box, float TrapImpactForce)
{
	ProcessBoxDamage(Box, TrapImpactForce, TEXT("Trap Impact Force"));
}

void UPhysicsJudgeManager::ProcessBoxDamage(ADeliveryBox* Box, float Force, const FString& SourceName)
{
	if (!Box) return;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

	float DamageThreshold = Box->GetDamageThreshold(); 
	if (Force >= DamageThreshold)
	{
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged")));
		
		DELIVERY_LOG(LogParcelDelivery, Warning, TEXT("[Server] Box ID %d was damaged by %s of %f (Threshold: %f)!"), 
			Box->GetBoxID(), *SourceName, Force, DamageThreshold);

		// 델리게이트 알림
		OnBoxDamaged.Broadcast(Box);
	}
	else if (SourceName.Contains(TEXT("Trap")))
	{
		DELIVERY_LOG(LogParcelDelivery, Log, TEXT("[Server] Box ID %d hit by Trap but force %f was below threshold %f."), 
			Box->GetBoxID(), Force, DamageThreshold);
	}
}