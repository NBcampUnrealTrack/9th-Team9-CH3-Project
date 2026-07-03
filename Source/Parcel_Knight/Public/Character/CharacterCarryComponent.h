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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
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

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Throw(FVector Force);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	ADeliveryBox* CarriedBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	bool bIsCarrying;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	float MoveSpeedMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	float DefaultMaxWalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carry", meta = (AllowPrivateAccess = "true"))
	FName HandSocketName;
};
