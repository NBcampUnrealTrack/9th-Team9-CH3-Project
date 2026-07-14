#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interface/IDFActivatableTrap.h"
#include "Interface/IDFTrap.h"
#include "DFTeleportPad.generated.h"

class AParcelCharacter;
class UBoxComponent;
class UNiagaraSystem;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EDFTeleportCarryPolicy : uint8
{
	KeepHeldAfterTeleport UMETA(DisplayName = "Keep Held After Teleport"),
	DropBeforeTeleport UMETA(DisplayName = "Drop Before Teleport"),
	DropAfterTeleport UMETA(DisplayName = "Drop After Teleport")
};

UCLASS(Blueprintable)
class PARCEL_KNIGHT_API ADFTeleportPad : public AActor, public IIDFTrap, public IIDFActivatableTrap
{
	GENERATED_BODY()

public:
	ADFTeleportPad();

	virtual FGameplayTag GetTrapStateTag_Implementation() const override;
	virtual bool IsTrapReady_Implementation() const override;
	virtual bool RequestActivate_Implementation(AActor* Activator) override;
	virtual bool CanActivate_Implementation(AActor* Activator) const override;

	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool ActivateTeleportPad(AActor* Activator);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool TryTeleportActor(AActor* ActorToTeleport);

	UFUNCTION(Server, Reliable)
	void Server_RequestTeleport(AActor* ActorToTeleport);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayTeleportFX(FVector StartLocation, FVector EndLocation);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Trap|Teleport")
	TObjectPtr<AActor> DestinationActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TObjectPtr<UNiagaraSystem> TeleportEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TObjectPtr<USoundBase> TeleportSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Cooldown", meta = (ClampMin = "0.0"))
	float TeleportCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Teleport")
	bool bResetVelocityOnTeleport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Networking")
	bool bUseServerAuthority = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Carry")
	EDFTeleportCarryPolicy CarryPolicy = EDFTeleportCarryPolicy::KeepHeldAfterTeleport;

private:
	bool CanActivate_ServerOnly(AActor* Activator) const;
	bool TeleportPlayer_ServerOnly(AParcelCharacter* PlayerCharacter);
	void ClearTeleportCooldown(AActor* ActorToClear);

	TSet<TWeakObjectPtr<AActor>> RecentlyTeleportedActors;
};
