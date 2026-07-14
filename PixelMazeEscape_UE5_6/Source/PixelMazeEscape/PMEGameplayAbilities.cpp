#include "PMEGameplayAbilities.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PMEGameModeBase.h"
#include "PMEGameplayEffects.h"
#include "PMEInventoryComponent.h"
#include "PMEPlayerCharacter.h"
#include "PMEPlayerController.h"
#include "PMEPlayerState.h"

UPMEGameplayAbility::UPMEGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

APMEPlayerState* UPMEGameplayAbility::GetMazePlayerState(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return ActorInfo ? Cast<APMEPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
}

APMEPlayerCharacter* UPMEGameplayAbility::GetMazeCharacter(const FGameplayAbilityActorInfo* ActorInfo) const
{
	return ActorInfo ? Cast<APMEPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
}

bool UPMEGameplayAbility::ConsumeItem(const FGameplayAbilityActorInfo* ActorInfo, const EPMEPickupType ItemType) const
{
	APMEPlayerState* PlayerState = GetMazePlayerState(ActorInfo);
	return PlayerState && PlayerState->GetInventoryComponent() && PlayerState->GetInventoryComponent()->
	                                                                           ConsumeItem(ItemType, 1);
}

UPMEGA_Sprint::UPMEGA_Sprint()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	bReplicateInputDirectly = true;
}

void UPMEGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		SprintEffectHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo,
		                                                UPMEGE_Sprint::StaticClass()->GetDefaultObject<
			                                                UGameplayEffect>(), 1.0f);
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Movement.Sprinting")));
	}
}

void UPMEGA_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UPMEGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility,
                               const bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (SprintEffectHandle.IsValid()) ASC->RemoveActiveGameplayEffect(SprintEffectHandle);
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Movement.Sprinting")));
	}
	SprintEffectHandle.Invalidate();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UPMEGA_Torch::UPMEGA_Torch()
{
}

void UPMEGA_Torch::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	APMEPlayerState* PlayerState = GetMazePlayerState(ActorInfo);
	if (PlayerState && ConsumeItem(ActorInfo, EPMEPickupType::Torch))
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UPMEGE_Torch::StaticClass(), 1.0f);
		if (SpecHandle.IsValid())
		{
			const float Duration = BaseDuration + DurationPerUpgradeStack * PlayerState->GetUpgradeStack(
				EPMEUpgradeType::LongerTorch);
			SpecHandle.Data->SetDuration(Duration, true);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPMEGA_MapPulse::UPMEGA_MapPulse()
{
}

void UPMEGA_MapPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	APMEPlayerState* PlayerState = GetMazePlayerState(ActorInfo);
	APMEPlayerCharacter* Character = GetMazeCharacter(ActorInfo);
	if (Character && ConsumeItem(ActorInfo, EPMEPickupType::MapPulse))
	{
		if (APMEPlayerController* PC = Cast<APMEPlayerController>(Character->GetController()))
		{
			PC->ClientActivateMapPulse(BaseDuration + (PlayerState
				                                           ? PlayerState->GetUpgradeStack(
					                                           EPMEUpgradeType::BiggerMapPulse) *
				                                           DurationPerUpgradeStack
				                                           : 0.0f));
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPMEGA_Decoy::UPMEGA_Decoy()
{
}

void UPMEGA_Decoy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	APMEPlayerCharacter* Character = GetMazeCharacter(ActorInfo);
	if (Character && ConsumeItem(ActorInfo, EPMEPickupType::Decoy))
	{
		if (APMEGameModeBase* GM = Character->GetWorld()->GetAuthGameMode<APMEGameModeBase>()) GM->
			SpawnDecoy(Character);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPMEGA_LightBomb::UPMEGA_LightBomb()
{
}

void UPMEGA_LightBomb::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	APMEPlayerCharacter* Character = GetMazeCharacter(ActorInfo);
	if (Character && ConsumeItem(ActorInfo, EPMEPickupType::LightBomb))
	{
		if (APMEGameModeBase* GM = Character->GetWorld()->GetAuthGameMode<APMEGameModeBase>()) GM->StunEnemiesAround(
			Character->GetActorLocation(), RadiusInTiles, StunDuration);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPMEGA_MasterKey::UPMEGA_MasterKey()
{
}

void UPMEGA_MasterKey::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	APMEPlayerState* PlayerState = GetMazePlayerState(ActorInfo);
	if (PlayerState && ConsumeItem(ActorInfo, EPMEPickupType::MasterKey))
	{
		if (APMEGameModeBase* GM = PlayerState->GetWorld()->GetAuthGameMode<APMEGameModeBase>()) GM->
			UseMasterKey(PlayerState);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UPMEGA_Revive::UPMEGA_Revive()
{
}

void UPMEGA_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	APMEPlayerCharacter* Character = GetMazeCharacter(ActorInfo);
	if (Character)
	{
		if (APMEGameModeBase* GM = Character->GetWorld()->GetAuthGameMode<APMEGameModeBase>()) GM->
			TryReviveNearestTeammate(Character);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
