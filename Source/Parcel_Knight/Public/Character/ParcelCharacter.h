#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ParcelCharacter.generated.h"

class UParcelPlayerStateComponent;
class UParcelMovementStatComponent;
class URagdollComponent;
class UParcelHeroComponent;
class UParcelInteractionComponent;
class UCharacterCarryComponent;

// 플레이어가 조종하는 기본 캐릭터 클래스
// 이동, 시점 회전, 점프, 래그돌 테스트 입력을 처리하고 멀티플레이 복제를 지원
//
// 담당자: 김로운
UCLASS()
class PARCEL_KNIGHT_API AParcelCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AParcelCharacter();
    virtual void BeginPlay() override;
    
    // 플레이어 입력 컴포넌트에 Enhanced Input 액션들을 바인딩
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // 서버에서 이 캐릭터가 컨트롤러에 빙의될 때 호출, Listen Server의 로컬 플레이어 입력 매핑 등록을 보강
    virtual void PossessedBy(AController* NewController) override;

    // 클라이언트에서 Controller 값이 복제되어 바뀔 때 호출, 원격 접속 클라이언트가 possession 이후 입력 매핑을 놓치지 않게 합니다.
    virtual void OnRep_Controller() override;
    
    // 점프 시작 시점과 지면 착지 타이밍 이식
    virtual void OnJumped_Implementation() override;
    virtual void Landed(const FHitResult& Hit) override;
    
    // 양손, 무브먼트, UI 등이 상태 창고에 접근할 수 있도록 열어주는 게터
    FORCEINLINE UParcelPlayerStateComponent* GetParcelPlayerStateComponent() const { return PlayerStateComp; }

    FORCEINLINE UParcelInteractionComponent* GetParcelInteractionComponent() const { return InteractionComp; }
    FORCEINLINE UParcelMovementStatComponent* GetParcelMovementStatComponent() const { return MovementStatComp; }
    FORCEINLINE UCharacterCarryComponent* GetCharacterCarryComponent() const { return CarryComp; }

protected:
    // Player State Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelPlayerStateComponent> PlayerStateComp;
    
    // Ragdoll Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<URagdollComponent> RagdollComp;
    
    // Hero Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelHeroComponent> HeroComp;
    
    // Interaction Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelInteractionComponent> InteractionComp;
    
    // MovementStat Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Component")
    TObjectPtr<UParcelMovementStatComponent> MovementStatComp;
    
    // CharacterCarry Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCharacterCarryComponent> CarryComp;
};