#include "Core/ParcelGameplayTags.h"

namespace ParcelGameplayTags
{
	// ========================= Grade =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grade_A, "Grade.A", "높은 등급")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grade_B, "Grade.B", "평균 등급")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grade_C, "Grade.C", "망한 등급")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grade_F, "Grade.F", "낙제")

	// ========================= Effect =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Stat_HP, "Effect.Stat.HP", "MaxHP 증가 — Value만큼 최대 체력 상승")

	// ========================= Item =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable,          "Item.Consumables",          "소모품 카테고리")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Gun,      "Item.Consumables.Gun",      "즉사 총알 한 발 — 쿨타임 5초")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_HPBoost,  "Item.Consumables.HPBoost",  "최대 HP 증가 패시브 아이템")

	// ========================= Character =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_GettingUp, "Character.State.GettingUp", "래그돌 해제 후 기상 모션 중인 상태")
}
