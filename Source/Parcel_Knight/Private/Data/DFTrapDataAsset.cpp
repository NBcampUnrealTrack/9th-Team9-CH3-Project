#include "Data/DFTrapDataAsset.h"

UDFTrapDataAsset::UDFTrapDataAsset()
{
	TrapTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Type.Generic"), false);
	TriggerTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Trigger.Overlap"), false);
	EffectTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Trap.Effect.Slow"), false);
	TrapDamage = 0.0f;
}
