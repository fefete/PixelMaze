#include "PMEAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UPMEAttributeSet::UPMEAttributeSet()
{
	InitMaxHealth(3.0f);
	InitHealth(3.0f);
	InitMoveSpeed(500.0f);
	InitFogRevealRadius(5.0f);
	InitNoiseMultiplier(1.0f);
	InitDetectionMultiplier(1.0f);
	InitSprintMultiplier(1.65f);
}

void UPMEAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, FogRevealRadius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, NoiseMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, DetectionMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPMEAttributeSet, SprintMultiplier, COND_None, REPNOTIFY_Always);
}

void UPMEAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute()) NewValue = FMath::Max(1.0f, NewValue);
	else if (Attribute == GetHealthAttribute()) NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	else if (Attribute == GetMoveSpeedAttribute()) NewValue = FMath::Clamp(NewValue, 100.0f, 1200.0f);
	else if (Attribute == GetFogRevealRadiusAttribute()) NewValue = FMath::Clamp(NewValue, 1.0f, 30.0f);
	else if (Attribute == GetNoiseMultiplierAttribute()) NewValue = FMath::Clamp(NewValue, 0.2f, 4.0f);
	else if (Attribute == GetDetectionMultiplierAttribute()) NewValue = FMath::Clamp(NewValue, 0.35f, 2.0f);
	else if (Attribute == GetSprintMultiplierAttribute()) NewValue = FMath::Clamp(NewValue, 1.1f, 3.0f);
}

void UPMEAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Min(GetHealth(), GetMaxHealth()));
	}
}

void UPMEAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, Health, OldValue);
}

void UPMEAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, MaxHealth, OldValue);
}

void UPMEAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, MoveSpeed, OldValue);
}

void UPMEAttributeSet::OnRep_FogRevealRadius(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, FogRevealRadius, OldValue);
}

void UPMEAttributeSet::OnRep_NoiseMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, NoiseMultiplier, OldValue);
}

void UPMEAttributeSet::OnRep_DetectionMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, DetectionMultiplier, OldValue);
}

void UPMEAttributeSet::OnRep_SprintMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPMEAttributeSet, SprintMultiplier, OldValue);
}
