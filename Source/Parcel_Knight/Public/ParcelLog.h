#pragma once

#include "CoreMinimal.h"

/** <사용법>
1) 사용할 cpp 파일에 #include "ParcelLog.h" 하고, 
2) 사용할 cpp 파일 #include 묶음 아래에 DEFINE_LOG_CATEGORY(사용할 로그 이름); 입력하고,
3) ParcelLog.h에 (1)외부 선언, (2) 파트별 전용 로그 매크로 정의를 적어 넣으면 됩니다.

<언리얼 콘솔창(~)에서>
1) Log LogParcelCharacter none, Log LogParcelMovement none, .... 이런식으로 입력하면 관련 로그를 끌 수 있습니다.
2) 반대로 Log LogParcelCharacter all, ... 이런식으로 입력하면 관련 로그만 켤 수 있습니다.
*/

// ==========================================================================
// [DECLARE] 로그 카테고리 외부 선언
// ==========================================================================

// Character
DECLARE_LOG_CATEGORY_EXTERN(LogCharacter, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogParcelMovementStat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogParcelInteraction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogHeroComp, Log, All);

// Delivery

// ==========================================================================
// [define] 파트별 전용 로그 매크로 정의
// ==========================================================================

// Character
#define PLAYER_LOG(Verbosity, Format, ...)      UE_LOG(LogCharacter, Verbosity, Format, ##__VA_ARGS__)
#define MOVEMENT_LOG(Verbosity, Format, ...)    UE_LOG(LogParcelMovementStat, Verbosity, Format, ##__VA_ARGS__)
#define INTERACT_LOG(Verbosity, Format, ...)    UE_LOG(LogParcelInteraction, Verbosity, Format, ##__VA_ARGS__)
#define HEROCOMP_LOG(Verbosity, Format, ...)    UE_LOG(LogParcelInteraction, Verbosity, Format, ##__VA_ARGS__)

// Delivery