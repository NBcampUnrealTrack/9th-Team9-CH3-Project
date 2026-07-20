#pragma once

#include "NativeGameplayTags.h"

namespace ParcelGameplayTags
{
	// ========================= Grade =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grade_A)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grade_B)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grade_C)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Grade_F)

	// ========================= Effect =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Stat_HP)

	// ========================= Item =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Gun)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_HPBoost)

	// ========================= Character =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_GettingUp)
}
