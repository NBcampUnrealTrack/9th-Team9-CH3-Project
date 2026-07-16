#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ParcelHeroComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UParcelInGameESCMenuWidget;

// [UI] 던지기 충전 상태 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThrowChargeChangedSignature, bool, bIsCharging, float, ChargeRatio);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PARCEL_KNIGHT_API UParcelHeroComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UParcelHeroComponent();
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// 캐릭터의 입력을 바인딩 (SetupPlayerInputComponent 시점에 호출)
	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void AddInputMappingContext();
	
	void ResetCameraAttachment();
	
	void EnterRagdollCameraMode();
	void ExitRagdollCameraMode();
	
	// [UI] HUD 위젯 바인딩
	UPROPERTY(BlueprintAssignable, Category = "Carry|Throw")
	FOnThrowChargeChangedSignature OnThrowChargeChanged;

	// [UI] 현재 로컬 차징 상태 게터 함수들
	UFUNCTION(BlueprintPure, Category = "Carry|Throw")
	FORCEINLINE bool IsChargingThrow() const { return bIsChargingThrow; }
	
	UFUNCTION(BlueprintPure, Category = "Carry|Throw")
	FORCEINLINE float GetThrowChargeRatio() const { return MaxThrowChargeTime > 0.f ? FMath::Clamp(CurrentThrowChargeTime / MaxThrowChargeTime, 0.f, 1.f) : 0.f; }
	
	// [UI] 마우스 감도
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetMouseSensitivity(float NewSensitivity) { MouseSensitivity = NewSensitivity; }

	UFUNCTION(BlueprintPure, Category = "Input")
	float GetMouseSensitivity() const { return MouseSensitivity; }
	
	
protected:
	// 카메라 컴포넌트 (인게임에 시점 변경이 필요하다면 이것도 분리 가능)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
	// EnhanceInput (인게임에서 조작 변경이 필요하다면 이것도 분리 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RagdollAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ThrowAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MinThrowForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MaxThrowForce = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Carry|Throw")
	float MaxThrowChargeTime = 1.5f;

	bool bIsChargingThrow = false;
	float CurrentThrowChargeTime = 0.f;
	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void TestRagdoll(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void StartThrow(const FInputActionValue& Value);
	void ReleaseThrow(const FInputActionValue& Value);
	
	// Ragdoll 카메라 세팅 에디터 노출
	UPROPERTY(EditAnywhere, Category = "Camera|Ragdoll")
	float RagdollCameraHeightOffset = 20.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Ragdoll")
	float RagdollCameraBackOffset = 90.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Collision")
	float CameraCollisionProbeSize = 18.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float MouseSensitivity = 1.0f;
	
	// [UI] InGame ESC 버튼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InGameMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UParcelInGameESCMenuWidget> ESCMenuClass;

	UPROPERTY(Transient)
	TObjectPtr<UParcelInGameESCMenuWidget> ESCMenuRef;
	
	void ToggleInGameMenu();
	
private:
	// 달리기 기능을 위한 Server RPC
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewIsSprinting);

	void ApplySprintSpeed(bool bNewIsSprinting);
	bool CanProcessLocalInput() const;
	
	UFUNCTION(Server, Reliable)
	void ServerSetJumping(bool bNewIsJumping);

	void ApplyJumpTag(bool bNewIsJumping);
};
