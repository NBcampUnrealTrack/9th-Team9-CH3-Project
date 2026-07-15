// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/RespawnComponent.h"
#include "GameFramework/GameMode.h"

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
	if (Controller)
	{
		if (AGameMode* GM = GetOwner<AGameMode>())
			GM->RestartPlayer(Controller);
	}
}
