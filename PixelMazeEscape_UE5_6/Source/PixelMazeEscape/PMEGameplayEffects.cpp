#include "PMEGameplayEffects.h"

#include "PMEAttributeSet.h"

namespace
{
	FGameplayModifierInfo MakeModifier(const FGameplayAttribute& Attribute, const EGameplayModOp::Type Operation,
	                                   const float Magnitude)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Operation;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
		return Modifier;
	}
}

UPMEGE_DamageOne::UPMEGE_DamageOne()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeModifier(UPMEAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -1.0f));
}

UPMEGE_Sprint::UPMEGE_Sprint()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Modifiers.Add(MakeModifier(UPMEAttributeSet::GetNoiseMultiplierAttribute(), EGameplayModOp::Multiplicitive, 1.75f));
}

UPMEGE_Torch::UPMEGE_Torch()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(12.0f);
	Modifiers.Add(MakeModifier(UPMEAttributeSet::GetFogRevealRadiusAttribute(), EGameplayModOp::Additive, 3.0f));
}
