// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RespawnComponent.generated.h"

/**
 * 플레이어 사망 후 리스폰을 처리하는 컴포넌트
 * GameMode에 부착되며 ReviveDelay 후 RestartPlayer를 호출한다.
 *
 * 담당자: 한수현
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API URespawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URespawnComponent();

	// [Server Only] DelaySeconds 후 RestartPlayer 호출 — PlayerState::HandleDeath에서 사용
	void RespawnPlayerAfterDelay(AController* Controller, float DelaySeconds);

	// 사망 후 부활 대기 시간 — 에디터에서 조정 가능
	UPROPERTY(EditAnywhere, Category = "Respawn")
	float ReviveDelay = 5.0f;

private:
	void DoRespawnPlayer(AController* Controller);
};
