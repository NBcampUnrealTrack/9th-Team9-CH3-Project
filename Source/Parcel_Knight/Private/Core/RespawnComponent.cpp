// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/RespawnComponent.h"
#include "GameFramework/GameMode.h"
#include "ParcelLog.h"

URespawnComponent::URespawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URespawnComponent::RespawnPlayerAfterDelay(AController* Controller, float DelaySeconds)
{
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &URespawnComponent::DoRespawnPlayer, Controller);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DelaySeconds, false);
}

void URespawnComponent::DoRespawnPlayer(AController* Controller)
{
	if (!Controller) return;

	AGameMode* GM = GetOwner<AGameMode>();
	if (!GM) return;

	// 구 폰 명시적 제거 — 래그돌 상태로 월드에 남아있는 캐릭터 파괴
	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	GM->RestartPlayer(Controller);
	GAMERULE_LOG(Log, TEXT("[부활] RestartPlayer 호출 완료"));
}
