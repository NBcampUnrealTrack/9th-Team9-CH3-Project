#include "Delivery/CarryComponent.h"
#include "Delivery/DeliveryBox.h"
#include "Net/UnrealNetwork.h"

UCarryComponent::UCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 복제 활성화
	bIsCarried = false;
	CarrierActor = nullptr;
}

void UCarryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCarryComponent, bIsCarried);
	DOREPLIFETIME(UCarryComponent, CarrierActor);
}

void UCarryComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UCarryComponent::CanCarry(AActor* Carrier) const
{
	ADeliveryBox* OwnerBox = Cast<ADeliveryBox>(GetOwner());
	if (!OwnerBox) return false;

	// 상자가 들려있지 않고 스폰된 상태일 때만 집기 가능
	return !bIsCarried && OwnerBox->HasStateTag(FGameplayTag::RequestGameplayTag(TEXT("Box.State.Spawned")));
}

void UCarryComponent::OnPickedUp(AActor* Carrier)
{
	if (!GetOwner()->HasAuthority()) return;

	bIsCarried = true;
	CarrierActor = Carrier;
}

void UCarryComponent::OnDropped()
{
	if (!GetOwner()->HasAuthority()) return;

	bIsCarried = false;
	CarrierActor = nullptr;
}