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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Stat_RespawnTimeReduction)

	// ========================= Item =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Gun)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_HPBoost)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Magnet)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_Smoke)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_PushTrap)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_FastRespawn)

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Title)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Title_Ace)

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Red)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Orange)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Yellow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Green)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Blue)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Indigo)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Purple)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_White)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Cosmetic_Skin_Gold)

	// ========================= Character =========================
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_GettingUp)
}
