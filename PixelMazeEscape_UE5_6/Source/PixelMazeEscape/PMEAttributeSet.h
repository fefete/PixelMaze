#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "PMEAttributeSet.generated.h"

#define PME_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PIXELMAZEESCAPE_API UPMEAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPMEAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Pixel Maze|Attributes")
	FGameplayAttributeData Health;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Pixel Maze|Attributes")
	FGameplayAttributeData MaxHealth;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MoveSpeed, Category="Pixel Maze|Attributes")
	FGameplayAttributeData MoveSpeed;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_FogRevealRadius, Category="Pixel Maze|Attributes")
	FGameplayAttributeData FogRevealRadius;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, FogRevealRadius)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_NoiseMultiplier, Category="Pixel Maze|Attributes")
	FGameplayAttributeData NoiseMultiplier;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, NoiseMultiplier)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_DetectionMultiplier, Category="Pixel Maze|Attributes")
	FGameplayAttributeData DetectionMultiplier;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, DetectionMultiplier)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SprintMultiplier, Category="Pixel Maze|Attributes")
	FGameplayAttributeData SprintMultiplier;
	PME_ATTRIBUTE_ACCESSORS(UPMEAttributeSet, SprintMultiplier)

private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_FogRevealRadius(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_NoiseMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_DetectionMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_SprintMultiplier(const FGameplayAttributeData& OldValue);
};
