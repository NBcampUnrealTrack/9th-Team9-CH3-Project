#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ParcelHeroComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ADeliveryBox;

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

	// 래그돌이 끝나자마자(기상 애니메이션 재생 중) 카메라를 캐릭터에 다시 붙여서 3인칭으로 따라가게 함.
	// 1인칭 전환(ExitRagdollCameraMode)은 이동 잠금이 풀릴 때까지 별도로 지연됨.
	void ReattachCameraAfterRagdoll();

	// [Server] 투척 충전 상태를 취소하고 Throwing 상태 태그를 정리
	void CancelServerThrowCharge();
	
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
	void SetMouseSensitivity(float NewSensitivity) { MouseSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 3.0f); }

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|ItemSlot")
	TObjectPtr<UInputAction> UseSlotAction1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|ItemSlot")
	TObjectPtr<UInputAction> UseSlotAction2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|ItemSlot")
	TObjectPtr<UInputAction> UseSlotAction3;

	// ESC 메뉴 위젯 관리는 AParcelPlayerController::ToggleInGameMenu()로 일원화되어 있음
	// (리스폰 시 이 컴포넌트가 재생성되면서 위젯 정리가 안 되던 중복 구현 버그 수정 — project_escmenu_consolidation 메모리 참고)
	void ToggleInGameMenu();
	void UseSlot1(const FInputActionValue& Value);
	void UseSlot2(const FInputActionValue& Value);
	void UseSlot3(const FInputActionValue& Value);
	void UseSlot(int32 SlotIndex);
	
private:
	// InitializePlayerInput 중복 호출 시 액션 바인딩이 중첩 등록되는 것을 막는 플래그
	// (SetupPlayerInputComponent가 재호출되면 BindAction이 중복 등록되어 R키 등이 한 번 입력에 두 번 실행되는 문제 방지)
	bool bInputBound = false;

	// 달리기 기능을 위한 Server RPC
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewIsSprinting);

	void ApplySprintSpeed(bool bNewIsSprinting);
	bool CanProcessLocalInput() const;
	
	UFUNCTION(Server, Reliable)
	void ServerSetJumping(bool bNewIsJumping);

	void ApplyJumpTag(bool bNewIsJumping);

	UFUNCTION(Server, Reliable)
	void Server_BeginThrowCharge();

	UFUNCTION(Server, Reliable)
	void Server_ReleaseThrow();

	void BeginThrowChargeServerOnly();
	void ReleaseThrowServerOnly();

	bool bServerThrowChargeActive = false;
	double ServerThrowChargeStartTimeSeconds = 0.0;
	TWeakObjectPtr<ADeliveryBox> ServerThrowChargeBox;
	
protected:
	void Input_OpenChat();
	void Input_ToggleLobbyMenu();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> OpenChatAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LobbyMenuAction;
};
