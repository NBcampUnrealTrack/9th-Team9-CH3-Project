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

	if (APawn* OldPawn = Controller->GetPawn())
		OldPawn->Destroy();

	GM->RestartPlayer(Controller);
	GAMERULE_LOG(Log, TEXT("[부활] RestartPlayer 호출 완료"));
}
