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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Stat_RespawnTimeReduction, "Effect.Stat.RespawnTimeReduction", "부활 대기시간 감소(초) — Value만큼 ReviveDelay에서 차감")

	// ========================= Item =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable,            "Item.Consumables",            "소모품 카테고리")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Gun,        "Item.Consumables.Gun",        "즉사 총알 한 발 — 쿨타임 5초")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_HPBoost,    "Item.Consumables.HPBoost",    "최대 HP 증가 패시브 아이템")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Magnet,     "Item.Consumables.Magnet",     "미보유 시 앞에 있는 택배를 바로 집는 아이템")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_Smoke,      "Item.Consumables.Smoke",      "일정 시간 시야를 가리는 연막탄")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_PushTrap,   "Item.Consumables.PushTrap",   "앞에 밀치기 함정(BP_Trap_PushPad)을 설치하는 아이템")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Consumable_FastRespawn,"Item.Consumables.FastRespawn","부활 대기시간을 2초 단축하는 패시브 아이템")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic,            "Item.Cosmetic",             "코스메틱 카테고리")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Title,      "Item.Cosmetic.Title",       "칭호 카테고리")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Title_Ace,  "Item.Cosmetic.Title.Ace",   "에이스 칭호")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin,        "Item.Cosmetic.Skin",        "스킨 카테고리")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Red,    "Item.Cosmetic.Skin.Red",    "빨간색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Orange, "Item.Cosmetic.Skin.Orange", "주황색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Yellow, "Item.Cosmetic.Skin.Yellow", "노란색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Green,  "Item.Cosmetic.Skin.Green",  "초록색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Blue,   "Item.Cosmetic.Skin.Blue",   "파란색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Indigo, "Item.Cosmetic.Skin.Indigo", "남색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Purple, "Item.Cosmetic.Skin.Purple", "보라색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_White,  "Item.Cosmetic.Skin.White",  "흰색 스킨")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Cosmetic_Skin_Gold,   "Item.Cosmetic.Skin.Gold",   "황금색 스킨")

	// ========================= Character =========================
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_GettingUp, "Character.State.GettingUp", "래그돌 해제 후 기상 모션 중인 상태")
}
