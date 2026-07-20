#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interface/IDFTrap.h"
#include "Interface/IDFActivatableTrap.h"
#include "DFTrapBase.generated.h"

class UBoxComponent;
class UDFTrapDataAsset;
class ACharacter;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PARCEL_KNIGHT_API ADFTrapBase : public AActor, public IIDFTrap, public IIDFActivatableTrap
{
	GENERATED_BODY()

public:
	ADFTrapBase();

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
	void ActivateTrap_ServerOnly(AActor* Activator);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ResetTrap_ServerOnly();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayActivateFX();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayResetFX();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTrapBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnTrapEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UFUNCTION()
	void OnRep_CurrentState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Trap|Visual")
	void OnTrapActivatedVisual();

	UFUNCTION(BlueprintImplementableEvent, Category = "Trap|Visual")
	void OnTrapResetVisual();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap|Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap")
	TObjectPtr<UDFTrapDataAsset> TrapDataAsset;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentState, BlueprintReadOnly, Category = "Trap|State")
	FGameplayTag CurrentStateTag;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Trap|State")
	bool bHasTriggeredOnce = false;

private:
	bool CanActivate_ServerOnly(AActor* Activator) const;
	void SetTrapState_ServerOnly(FGameplayTag NewStateTag);
	void EnterActiveState_ServerOnly();
	void EnterCooldownState_ServerOnly();
	void ApplyTrapEffectToOverlappingActors_ServerOnly();
	void ApplyTrapEffect_ServerOnly(AActor* TargetActor);
	void ApplyDamageOnce_ServerOnly(AActor* TargetActor);
	bool CanApplyDamageToActor(AActor* TargetActor) const;
	void ClearDamageCooldownForActor(AActor* TargetActor);
	void ApplyForcedDropEffect_ServerOnly(AActor* TargetActor);
	void ApplyPushEffect_ServerOnly(AActor* TargetActor);
	void ApplyReversePushEffect_ServerOnly(AActor* TargetActor, bool bRepeated);
	void ApplyReverseGroundPushEffect_ServerOnly(ACharacter* TargetCharacter, bool bRepeated);
	void ApplyInputInvertEffect_ServerOnly(AActor* TargetActor);
	void LaunchCharacterFromTrap_ServerOnly(
		ACharacter* TargetCharacter,
		const FVector& Direction,
		const TCHAR* EffectName,
		bool bXYOverride,
		bool bZOverride
	);
	void AddRepeatingReversePushTarget_ServerOnly(AActor* TargetActor);
	void RemoveRepeatingReversePushTarget_ServerOnly(AActor* TargetActor);
	void StartRepeatEffectTimer_ServerOnly();
	void StopRepeatEffectTimer_ServerOnly();
	void ApplyRepeatEffect_ServerOnly();
	bool IsOverlapTrigger() const;
	bool IsSlowEffect() const;
	bool IsForcedDropEffect() const;
	bool IsPushEffect() const;
	bool IsReversePushEffect() const;
	bool IsInputInvertEffect() const;
	bool IsKnownEffect() const;
	bool ShouldRepeatReversePush() const;

	UPROPERTY()
	TObjectPtr<AActor> PendingActivator;

	TArray<TWeakObjectPtr<ACharacter>> RepeatingReversePushTargets;
	TSet<TWeakObjectPtr<AActor>> ActorsInsideTrigger;
	TSet<TWeakObjectPtr<AActor>> AffectedActorsThisActivation;
	TSet<TWeakObjectPtr<AActor>> DamagedActorsThisActivation;
	TSet<TWeakObjectPtr<AActor>> ActorsOnDamageCooldown;

	FTimerHandle WarningTimerHandle;
	FTimerHandle ActiveTimerHandle;
	FTimerHandle CooldownTimerHandle;
	FTimerHandle RepeatEffectTimerHandle;
};
