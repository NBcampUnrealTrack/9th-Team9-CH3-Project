#include "Character/ParcelPlayerStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "ParcelLog.h"

DEFINE_LOG_CATEGORY(LogPlayerStateComp);

UParcelPlayerStateComponent::UParcelPlayerStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UParcelPlayerStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UParcelPlayerStateComponent, CharacterStateTags);
}

void UParcelPlayerStateComponent::AddStateTag(FGameplayTag NewStateTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !NewStateTag.IsValid()) return;

	if (!CharacterStateTags.HasTagExact(NewStateTag))
	{
		CharacterStateTags.AddTag(NewStateTag);
		OnStateTagAdded.Broadcast(NewStateTag);
        
		// 호스트 플레이어를 위해 OnRep 수동 호출
		OnRep_CharacterStateTags();
	}
}

void UParcelPlayerStateComponent::RemoveStateTag(FGameplayTag StateTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !StateTag.IsValid()) return;

	if (CharacterStateTags.HasTagExact(StateTag))
	{
		CharacterStateTags.RemoveTag(StateTag);
		OnStateTagRemoved.Broadcast(StateTag);
        
		// 호스트 플레이어를 위해 OnRep 수동 호출
		OnRep_CharacterStateTags();
	}
}

bool UParcelPlayerStateComponent::HasStateTag(FGameplayTag StateTag) const
{
	return CharacterStateTags.HasTag(StateTag);
}

void UParcelPlayerStateComponent::OnRep_CharacterStateTags()
{
	// Todo : 원격 클라이언트 기기에서 서버의 상태 복제를 받았을 때 로그 및 애니메이션 연동 처리 지원
	PLAYERSTATECOMP_LOG(Log, TEXT("[%s] 플레이어 상태 태그 컨테이너 동기화 완료: %s"), 
	   GetOwner() ? *GetOwner()->GetName() : TEXT("None"), *CharacterStateTags.ToString());
	
	if (OnCharacterStateTagsChanged.IsBound())
	{
		OnCharacterStateTagsChanged.Broadcast(CharacterStateTags);
	}
}