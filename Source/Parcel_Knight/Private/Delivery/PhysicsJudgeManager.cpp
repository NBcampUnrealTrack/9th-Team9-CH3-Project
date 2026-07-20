#include "Delivery/PhysicsJudgeManager.h"
#include "ParcelLog.h"
#include "Delivery/DeliveryBox.h"
#include "Core/HealthComponent.h"

DEFINE_LOG_CATEGORY(LogDeliveryPhysics);

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
	if (!Box->HasAuthority()) return;
	if (Box->IsInvulnerable()) return;

	if (UHealthComponent* BoxHealth = Box->FindComponentByClass<UHealthComponent>())
	{
		float FinalDamage = Force;

		// 물리 충격에 의한 대미지인 경우 데미지 스케일링 및 캡핑(상한선) 적용
		if (SourceName.Contains(TEXT("impact")))
		{
			// 1) 최소 임계치(50)를 초과한 분량에 대해 대미지 환산 (계수 0.1f)
			float BaseDamage = (Force - 50.0f) * 0.1f;
			
			// 2) 최대 파워로 던져도 한 번에 부서지지 않도록 단일 타격 최대 대미지를 최대 체력의 30%로 제한 (최소 3~4회 부딪혀야 깨짐)
			float MaxDamageLimit = BoxHealth->GetMaxHP() * 0.3f;
			
			FinalDamage = FMath::Clamp(BaseDamage, 1.0f, MaxDamageLimit);
		}
		else if (SourceName.Contains(TEXT("Trap")))
		{
			// 함정 대미지의 경우도 단일 타격 최대 대미지를 최대 체력의 25%로 제한 (최소 4회 밟아야 깨짐)
			float MaxTrapDamageLimit = BoxHealth->GetMaxHP() * 0.25f;
			FinalDamage = FMath::Clamp(Force, 1.0f, MaxTrapDamageLimit);
		}

		BoxHealth->TakeDamage(FinalDamage);

		PHYSICSJUDGE_LOG(Warning, TEXT("[Server] Box ID %d received %f damage (Raw Force: %f) from %s. (Remaining HP: %f/%f)"), 
			Box->GetBoxID(), FinalDamage, Force, *SourceName, BoxHealth->GetHP(), BoxHealth->GetMaxHP());
	}
}