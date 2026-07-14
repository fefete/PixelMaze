#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "PMETypes.h"
#include "PMEPlayerState.generated.h"

class UPMEAbilitySystemComponent;
class UPMEAttributeSet;
class UPMEInventoryComponent;
class UGameplayAbility;

UCLASS()
class PIXELMAZEESCAPE_API APMEPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    APMEPlayerState();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintPure, Category="Pixel Maze|GAS")
    UPMEAbilitySystemComponent* GetPMEAbilitySystemComponent() const { return AbilitySystemComponent; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|GAS")
    const UPMEAttributeSet* GetAttributeSet() const { return AttributeSet; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|Inventory")
    UPMEInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|Player") int32 GetPlayerIndex() const { return PlayerIndex; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|Customization")
    FName GetSelectedCharacterId() const { return SelectedCharacterId; }

    void SetSelectedCharacterId(FName NewCharacterId);
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Player") int32 GetWins() const { return Wins; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Player") bool HasReachedExit() const { return bReachedExit; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Player") float GetFinishTime() const { return FinishTime; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Player") bool IsDowned() const { return bDowned; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Score") int32 GetRunScore() const { return RunScore; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Score") int32 GetDetectionCount() const { return DetectionCount; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Score") int32 GetHitCount() const { return HitCount; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Score") int32 GetObjectivesActivated() const { return ObjectivesActivated; }
    UFUNCTION(BlueprintPure, Category="Pixel Maze|Score") float GetDistanceTravelled() const { return DistanceTravelled; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|Upgrades")
    int32 GetUpgradeStack(EPMEUpgradeType UpgradeType) const;

    UFUNCTION(BlueprintPure, Category="Pixel Maze|GAS")
    TSubclassOf<UGameplayAbility> GetReviveAbilityClass() const { return ReviveAbilityClass; }

    UFUNCTION(BlueprintPure, Category="Pixel Maze|GAS")
    TSubclassOf<UGameplayAbility> GetAbilityClassForItem(EPMEPickupType ItemType) const;

    void AssignPlayerIndex(int32 NewIndex);
    void ResetForRound();
    void MarkReachedExit(float NewFinishTime);
    void AddWin();
    void SetDowned(bool bNewDowned);
    void AddScore(int32 Amount);
    void RecordDetection();
    void RecordHit();
    void RecordObjective();
    void AddDistanceTravelled(float Distance);
    void AddUpgrade(EPMEUpgradeType UpgradeType);
    void ApplyPersistentUpgradeAttributes();
    void GrantDefaultAbilities();

private:
    UFUNCTION() void OnRep_PlayerIndex();
    UFUNCTION() void OnRep_SelectedCharacterId();
    UFUNCTION() void OnRep_Downed();
    UFUNCTION() void OnRep_Upgrades();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pixel Maze|GAS", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UPMEAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<UPMEAttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pixel Maze|Inventory", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UPMEInventoryComponent> InventoryComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> SprintAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> ReviveAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> TorchAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> MapPulseAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> DecoyAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> LightBombAbilityClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pixel Maze|GAS|Abilities", meta=(AllowPrivateAccess="true")) TSubclassOf<UGameplayAbility> MasterKeyAbilityClass;

    UPROPERTY(ReplicatedUsing=OnRep_PlayerIndex) int32 PlayerIndex = 0;
    UPROPERTY(ReplicatedUsing=OnRep_SelectedCharacterId) FName SelectedCharacterId = TEXT("Character.01");
    UPROPERTY(Replicated) int32 Wins = 0;
    UPROPERTY(Replicated) bool bReachedExit = false;
    UPROPERTY(Replicated) float FinishTime = 0.0f;
    UPROPERTY(ReplicatedUsing=OnRep_Downed) bool bDowned = false;
    UPROPERTY(Replicated) int32 RunScore = 0;
    UPROPERTY(Replicated) int32 DetectionCount = 0;
    UPROPERTY(Replicated) int32 HitCount = 0;
    UPROPERTY(Replicated) int32 ObjectivesActivated = 0;
    UPROPERTY(Replicated) float DistanceTravelled = 0.0f;
    UPROPERTY(ReplicatedUsing=OnRep_Upgrades) TArray<int32> UpgradeStacks;

    bool bAbilitiesGranted = false;
};
