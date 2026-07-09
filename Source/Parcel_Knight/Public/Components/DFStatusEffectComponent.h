#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DFStatusEffectComponent.generated.h"

class ACharacter;

USTRUCT(BlueprintType)
struct FDFMoveSpeedEffectState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EffectTag;

	UPROPERTY(BlueprintReadOnly)
	float Multiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float BaseMaxWalkSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct FDFInputInvertEffectState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag EffectTag;

	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PARCEL_KNIGHT_API UDFStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDFStatusEffectComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void ApplyMoveSpeedModifier(FGameplayTag EffectTag, float Multiplier, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void ApplyInputInvert(FGameplayTag EffectTag, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void ClearInputInvert();

	UFUNCTION(BlueprintPure, Category = "Status Effect")
	bool IsInputInverted() const { return InputInvertEffectState.bIsActive; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void Server_ApplyMoveSpeedModifier(FGameplayTag EffectTag, float Multiplier, float Duration);

	UFUNCTION(Client, Reliable)
	void Client_ApplyMoveSpeedEffectState(FDFMoveSpeedEffectState NewState);

	UFUNCTION(Server, Reliable)
	void Server_ApplyInputInvert(FGameplayTag EffectTag, float Duration);

	UFUNCTION(Server, Reliable)
	void Server_ClearInputInvert();

	UFUNCTION(Client, Reliable)
	void Client_ApplyInputInvertEffectState(FDFInputInvertEffectState NewState);

	UFUNCTION()
	void OnRep_MoveSpeedEffectState();

	UFUNCTION()
	void OnRep_InputInvertEffectState();

	void ApplyMoveSpeedState();
	void ClearMoveSpeedModifier_ServerOnly();
	void ClearInputInvert_ServerOnly();

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(ReplicatedUsing = OnRep_MoveSpeedEffectState)
	FDFMoveSpeedEffectState MoveSpeedEffectState;

	UPROPERTY(ReplicatedUsing = OnRep_InputInvertEffectState)
	FDFInputInvertEffectState InputInvertEffectState;

	FTimerHandle MoveSpeedEffectTimerHandle;
	FTimerHandle InputInvertEffectTimerHandle;
};
