#include "Data/DFTrapDataAsset.h"

UDFTrapDataAsset::UDFTrapDataAsset()
{
	TrapTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Type.Generic"), false);
	TriggerTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Trigger.Overlap"), false);
	EffectTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.Slow"), false);
	ActivationDelay = 0.0f;
	TrapDamage_DEPRECATED = 0.0f;
	bApplyDamageOnOverlap = false;
	DamageAmount = 0.0f;
	bDamageOnlyOncePerActivation = true;
	DamageCooldownPerActor = 0.5f;
}
