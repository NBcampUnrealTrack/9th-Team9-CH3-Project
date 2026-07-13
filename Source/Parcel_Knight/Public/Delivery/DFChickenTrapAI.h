#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Pawn.h"
#include "Interface/IDFActivatableTrap.h"
#include "Interface/IDFTrap.h"
#include "DFChickenTrapAI.generated.h"

class AParcelCharacter;
class UAnimInstance;
class UFloatingPawnMovement;
class UNiagaraSystem;
class USkeletalMeshComponent;
class USkeletalMesh;
class USoundBase;
class USphereComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable)
class PARCEL_KNIGHT_API ADFChickenTrapAI : public APawn, public IIDFTrap, public IIDFActivatableTrap
{
	GENERATED_BODY()

public:
	ADFChickenTrapAI();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FGameplayTag GetTrapStateTag_Implementation() const override;
	virtual bool IsTrapReady_Implementation() const override;
	virtual bool RequestActivate_Implementation(AActor* Activator) override;
	virtual bool CanActivate_Implementation(AActor* Activator) const override;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Trap")
	void Server_RequestActivate(AActor* Activator);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool TryActivate(AActor* Activator);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool ActivateTrap(AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ActivateTrap_ServerOnly(AActor* Activator);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayExplosionFX(FVector_NetQuantize ExplosionLocation);

protected:
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
	TObjectPtr<USkeletalMeshComponent> ChickenMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Movement")
	TObjectPtr<UFloatingPawnMovement> FloatingMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TObjectPtr<USkeletalMesh> ChickenSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TSubclassOf<UAnimInstance> ChickenAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Visual")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float KnockbackStrength = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Effect", meta = (ClampMin = "0.0"))
	float UpwardStrength = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Lifetime", meta = (ClampMin = "0.0"))
	float DestroyDelay = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Effect")
	bool bDropCarriedBoxOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement")
	bool bEnableServerRoaming = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement", meta = (ClampMin = "0.0"))
	float PatrolRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement", meta = (ClampMin = "0.0"))
	float DetectionRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement", meta = (ClampMin = "0.1"))
	float MoveUpdateInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Movement", meta = (ClampMin = "0.0"))
	float MoveAcceptanceRadius = 75.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly, Category = "Trap|State")
	FGameplayTag CurrentStateTag;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trap|State")
	bool bHasTriggeredOnce = false;

private:
	UFUNCTION()
	void OnRep_CurrentState();

	void StartAIMovement_ServerOnly();
	void UpdateAIMovement_ServerOnly();
	bool MoveToClosestPlayer_ServerOnly();
	void MoveToRandomReachablePoint_ServerOnly();
	AParcelCharacter* FindClosestPlayerInDetectionRadius_ServerOnly() const;
	bool CanActivate_ServerOnly(AActor* Activator) const;
	void SetTrapState_ServerOnly(FGameplayTag NewStateTag);
	void ApplyKnockback_ServerOnly(AParcelCharacter* HitCharacter) const;
	void ApplyLaunchCharacterFallback_ServerOnly(AParcelCharacter* HitCharacter) const;
	void DropCarriedBox_ServerOnly(AParcelCharacter* HitCharacter) const;
	void StopAIMovement_ServerOnly();

	FTimerHandle AIMoveTimerHandle;
	FVector PatrolOriginLocation = FVector::ZeroVector;
};
