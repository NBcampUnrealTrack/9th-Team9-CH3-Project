// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterCarryComponent.generated.h"

class ADeliveryBox;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARCEL_KNIGHT_API UCharacterCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterCarryComponent();
	
	// 멀티플레이 동기화를 위한 Rep 처리
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Pickup(ADeliveryBox* InBox);

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Drop();

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void Throw(FVector Force);

	UFUNCTION(BlueprintCallable, Category = "Carry")
	void ForceDropByTrap(float TrapDamage);

	UFUNCTION(BlueprintPure, Category = "Carry")
	FORCEINLINE bool IsCarrying() const { return bIsCarrying; }

	UFUNCTION(BlueprintPure, Category = "Carry")
	FORCEINLINE ADeliveryBox* GetCarriedBox() const { return CarriedBox; }

	FORCEINLINE float GetMoveSpeedMultiplier() const { return MoveSpeedMultiplier; }
	
protected:
	UFUNCTION(Server, Reliable, WithValidation) void Server_Drop();
	UFUNCTION(Server, Reliable, WithValidation) void Server_Throw(FVector Force);

private:
	// 네트워크 동기화 RepNotify
	UFUNCTION() void OnRep_CarriedBox();

	void SyncWeightToMovement();
	
	UPROPERTY(ReplicatedUsing = OnRep_CarriedBox, VisibleAnywhere, Category = "Carry")
	TObjectPtr<ADeliveryBox> CarriedBox;
	
	// [Local] 들고 있던 상자를 기억하는 로컬 변수
	UPROPERTY()
	TObjectPtr<ADeliveryBox> PreviousCarriedBox;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Carry")
	bool bIsCarrying;

	UPROPERTY(VisibleAnywhere, Category = "Carry")
	float MoveSpeedMultiplier;

	UPROPERTY(EditDefaultsOnly, Category = "Carry")
	FName HandSocketName;
};
