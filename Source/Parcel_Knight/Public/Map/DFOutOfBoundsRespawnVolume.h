#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFOutOfBoundsRespawnVolume.generated.h"

class ACharacter;
class AController;
class APlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogDFOutOfBoundsRespawn, Log, All);

UCLASS(Blueprintable)
class PARCEL_KNIGHT_API ADFOutOfBoundsRespawnVolume : public AActor
{
	GENERATED_BODY()

public:
	ADFOutOfBoundsRespawnVolume();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Respawn|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Respawn|Components")
	TObjectPtr<UBoxComponent> RespawnTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn", meta = (DisplayName = "Is Lobby"))
	bool bIsLobby = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Respawn")
	TObjectPtr<AActor> RespawnTargetActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Respawn|Stage")
	TObjectPtr<AActor> OverviewCameraActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn|Stage", meta = (ClampMin = "0.0"))
	float StageRespawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn|Camera", meta = (ClampMin = "0.0"))
	float CameraBlendTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn|Stage")
	bool bDisableMovementWhileWaiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn|Stage")
	bool bHidePlayerWhileWaiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	bool bResetVelocityOnRespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn", meta = (ClampMin = "0.0"))
	float RespawnZOffset = 5.0f;

private:
	void HandleOutOfBounds(ACharacter* Character);
	void RespawnImmediately(ACharacter* Character);
	void BeginDelayedRespawn(ACharacter* Character);
	void FinishDelayedRespawn(
		TWeakObjectPtr<ACharacter> CharacterPtr,
		TWeakObjectPtr<AController> ControllerPtr
	);
	bool GetRespawnTransform(AController* Controller, FTransform& OutTransform) const;
	void ApplyOverviewCamera(APlayerController* PlayerController);
	void RestorePlayerCamera(APlayerController* PlayerController, ACharacter* Character);
	void SetCharacterWaitingState(ACharacter* Character, bool bWaiting);
	void ClearPendingRespawn(AActor* Actor);
	bool TeleportCharacter(ACharacter* Character, const FTransform& RespawnTransform);

	TSet<TWeakObjectPtr<AActor>> PendingRespawnActors;
	TMap<TWeakObjectPtr<AActor>, bool> PreviousHiddenStates;
};
