// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ParcelGameState.h"
#include "Core/TeamScoreComponent.h"
#include "Core/ShopComponent.h"
#include "Net/UnrealNetwork.h"

AParcelGameState::AParcelGameState()
{
	TeamScoreComp = CreateDefaultSubobject<UTeamScoreComponent>("TeamScoreComponent");
	ShopComp      = CreateDefaultSubobject<UShopComponent>("ShopComponent");
	
	// [UI] 클라이언트에 의해 복제 허용
	TeamScoreComp->SetIsReplicated(true);
	
	SelectedMapIndex = 0;
}

void AParcelGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AParcelGameState, SelectedMapIndex);
}

UTeamScoreComponent* AParcelGameState::GetTeamScoreComponent() const
{
	return TeamScoreComp;
}

UShopComponent* AParcelGameState::GetShopComponent() const
{
	return ShopComp;
}

// [UI]
void AParcelGameState::Multicast_NotifyDeliveryLog_Implementation(const FString& PlayerName, const FString& BoxName, bool bSuccess)
{
	if (OnDeliveryLogReceived.IsBound())
	{
		OnDeliveryLogReceived.Broadcast(PlayerName, BoxName, bSuccess);
	}
}

void AParcelGameState::SetSelectedMapIndex(int32 NewIndex)
{
	if (!HasAuthority()) return;

	if (SelectedMapIndex != NewIndex)
	{
		SelectedMapIndex = NewIndex;
 
		if (OnLobbyMapChanged.IsBound())
		{
			OnLobbyMapChanged.Broadcast(SelectedMapIndex);
		}
	}
}

void AParcelGameState::OnRep_SelectedMapIndex()
{
	if (OnLobbyMapChanged.IsBound())
	{
		OnLobbyMapChanged.Broadcast(SelectedMapIndex);
	}
    
	UE_LOG(LogTemp, Log, TEXT("[GameState Synced] 서버로부터 신규 맵 인덱스(%d) 패킷 동동기화 완료 -> HUD 브로드캐스트 전송"), SelectedMapIndex);
}