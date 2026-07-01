#include "Delivery/PhysicsJudgeManager.h"
#include "Delivery/DeliveryBox.h"

void UPhysicsJudgeManager::EvaluateImpact(ADeliveryBox* Box, float ImpactForce)
{
	if (!Box) return;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

	float DamageThreshold = Box->GetDamageThreshold(); 

	if (ImpactForce >= DamageThreshold)
	{
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged")));
		
		UE_LOG(LogDelivery, Warning, TEXT("[Server] Box ID %d was damaged by impact force of %f (Threshold: %f)!"), 
			Box->GetBoxID(), ImpactForce, DamageThreshold);
	}
}

void UPhysicsJudgeManager::EvaluateTrapImpact(ADeliveryBox* Box, float TrapImpactForce)
{
	if (!Box) return;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

	float DamageThreshold = Box->GetDamageThreshold();
	if (TrapImpactForce >= DamageThreshold)
	{
		Box->AddStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Damaged")));
		UE_LOG(LogDelivery, Warning, TEXT("[Server] Box ID %d was damaged by Trap Impact Force of %f (Threshold: %f)!"), Box->GetBoxID(), TrapImpactForce, DamageThreshold);
	}
	else
	{
		UE_LOG(LogDelivery, Log, TEXT("[Server] Box ID %d hit by Trap but force %f was below threshold %f."), Box->GetBoxID(), TrapImpactForce, DamageThreshold);
	}
}