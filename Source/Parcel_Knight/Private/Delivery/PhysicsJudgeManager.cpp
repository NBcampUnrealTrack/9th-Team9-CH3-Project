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
			// 1) 상자 자체의 데이터 테이블 파손 임계치(DamageThreshold) 로드
			float DamageThreshold = Box->GetDamageThreshold();

			// 임계값 미만의 미세한 물리 충격은 완전히 무시
			if (Force < DamageThreshold)
			{
				return;
			}

			float DamageMultiplier = 0.15f;
			float MaxDamageLimitPercent = 0.20f;

			// 깨지기 쉬운 상자(Box.Type.Fragile)는 매우 약하므로 대미지 계수와 최대 한계를 별도로 강화 조율
			if (Box->GetBoxData().BoxTypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.Fragile"))))
			{
				DamageMultiplier = 0.70f;       // 대미지 가중치 70%로 큰 대미지 유도
				MaxDamageLimitPercent = 0.60f;  // 최대 60% 캡 설정 (확정 2회 충격에 파손 보장)
			}

			// 큰 임계치를 대미지 연산에서 직접 빼버리면 대미지가 너무 줄어들므로, 
			// 임계값을 충족(체크 완료)한 이후에는 기본 감쇄 오프셋(50.f)만 차감하여 스케일링합니다.
			float BaseDamage = (Force - 50.0f) * DamageMultiplier;
			float MaxDamageLimit = BoxHealth->GetMaxHP() * MaxDamageLimitPercent;
			
			FinalDamage = FMath::Clamp(BaseDamage, 0.0f, MaxDamageLimit);
		}
		else if (SourceName.Contains(TEXT("Trap")))
		{
			float MaxTrapDamageLimitPercent = 0.20f;
			if (Box->GetBoxData().BoxTypeTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("Box.Type.Fragile"))))
			{
				MaxTrapDamageLimitPercent = 0.60f;
			}

			// 함정 대미지의 경우도 깨지기 쉬운 상자는 60% 캡, 일반/무거운 상자는 20% 캡으로 조율
			float MaxTrapDamageLimit = BoxHealth->GetMaxHP() * MaxTrapDamageLimitPercent;
			FinalDamage = FMath::Clamp(Force, 0.0f, MaxTrapDamageLimit);
		}

		// 최종 대미지가 0 이하인 경우 연산하지 않고 스킵
		if (FinalDamage <= 0.0f) return;

		BoxHealth->TakeDamage(FinalDamage);

		// 다중 피격으로 인한 폭사 방지를 위해 상자에 최근 피격 시간 기록 (0.3초 쿨타임용)
		if (UWorld* World = GetWorld())
		{
			Box->SetLastDamageTime(World->GetTimeSeconds());
		}

		PHYSICSJUDGE_LOG(Warning, TEXT("[Server] Box ID %d received %f damage (Raw Force: %f) from %s. (Remaining HP: %f/%f)"), 
			Box->GetBoxID(), FinalDamage, Force, *SourceName, BoxHealth->GetHP(), BoxHealth->GetMaxHP());
	}
}