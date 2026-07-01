#include "Delivery/DeliverySubsystem.h"
#include "Delivery/DeliveryBox.h"

UDeliverySubsystem::UDeliverySubsystem()
{
	NextBoxID = 1000;
}

void UDeliverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ActiveBoxes.Empty();
}

void UDeliverySubsystem::Deinitialize()
{
	ActiveBoxes.Empty();
	Super::Deinitialize();
}

int32 UDeliverySubsystem::GenerateBoxID()
{
	return ++NextBoxID;
}


AActor* UDeliverySubsystem::SpawnBox(FName BoxTypeID, FVector SpawnLocation, FRotator SpawnRotation)
{
	// 서버(HasAuthority)에서만 스폰하도록 검증 공간 확보
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
	{
		// FActorSpawnParameters SpawnParams;
		// ADFDeliveryBox* NewBox = GetWorld()->SpawnActor<ADFDeliveryBox>(..., SpawnLocation, SpawnRotation, SpawnParams);
		// if(NewBox) { 
		//     NewBox->BoxID = GenerateBoxID();
		//     ActiveBoxes.Add(NewBox);
		//     return NewBox;
		// }
	}
	return nullptr;
}

void UDeliverySubsystem::DespawnBox(AActor* Box)
{
	if (!Box) return;

	ActiveBoxes.Remove(Box);
	Box->Destroy();
}