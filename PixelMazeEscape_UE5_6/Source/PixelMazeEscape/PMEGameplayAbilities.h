#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "PMETypes.h"
#include "PMEGameplayAbilities.generated.h"

UCLASS(Abstract)
class PIXELMAZEESCAPE_API UPMEGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UPMEGameplayAbility();

protected:
    class APMEPlayerState* GetMazePlayerState(const FGameplayAbilityActorInfo* ActorInfo) const;
    class APMEPlayerCharacter* GetMazeCharacter(const FGameplayAbilityActorInfo* ActorInfo) const;
    bool ConsumeItem(const FGameplayAbilityActorInfo* ActorInfo, EPMEPickupType ItemType) const;
    void NotifySuccessfulItemUse(
        const FGameplayAbilityActorInfo* ActorInfo,
        EPMEPickupType ItemType) const;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_Sprint : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_Sprint();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
    FActiveGameplayEffectHandle SprintEffectHandle;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_Torch : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_Torch();
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="1.0")) float BaseDuration = 12.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="0.0")) float DurationPerUpgradeStack = 3.0f;
public:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_MapPulse : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_MapPulse();
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="0.2")) float BaseDuration = 4.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="0.0")) float DurationPerUpgradeStack = 1.5f;
public:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_Decoy : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_Decoy();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_LightBomb : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_LightBomb();
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="0.5")) float RadiusInTiles = 4.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|Ability", meta=(ClampMin="0.1")) float StunDuration = 3.0f;
public:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_MasterKey : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_MasterKey();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGA_Revive : public UPMEGameplayAbility
{
    GENERATED_BODY()
public:
    UPMEGA_Revive();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
