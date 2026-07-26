#include "Core/ParcelSmokeCloud.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"

AParcelSmokeCloud::AParcelSmokeCloud()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SmokeVolume = CreateDefaultSubobject<USphereComponent>(TEXT("SmokeVolume"));
	SmokeVolume->InitSphereRadius(300.f);
	SmokeVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SmokeVolume;

	SmokeFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SmokeFX"));
	SmokeFX->SetupAttachment(RootComponent);
}

void AParcelSmokeCloud::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &AParcelSmokeCloud::OnLifetimeExpired, Lifetime, false);
	}
}

void AParcelSmokeCloud::OnLifetimeExpired()
{
	Destroy();
}
