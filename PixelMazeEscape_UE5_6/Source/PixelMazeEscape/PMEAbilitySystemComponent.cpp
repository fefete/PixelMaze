#include "PMEAbilitySystemComponent.h"

UPMEAbilitySystemComponent::UPMEAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}
