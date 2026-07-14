#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PMECharacterCosmeticTypes.h"
#include "PMECharacterCustomizationSubsystem.generated.h"

class UMaterialInterface;
class UPMEPlayerProfileSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPMECharacterSelectionChangedSignature,
	FName,
	CharacterId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FPMEAchievementUnlockedSignature,
	EPMEProfileAchievement,
	Achievement);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPMEProfileChangedSignature);

UCLASS()
class PIXELMAZEESCAPE_API UPMECharacterCustomizationSubsystem final
	: public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Customization")
	FPMECharacterSelectionChangedSignature OnCharacterSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Achievements")
	FPMEAchievementUnlockedSignature OnAchievementUnlocked;

	UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Profile")
	FPMEProfileChangedSignature OnProfileChanged;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Customization")
	TArray<FPMECharacterCosmeticDefinition> GetCharacterDefinitions() const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Customization")
	FName GetSelectedCharacterId() const;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Customization")
	bool SelectCharacter(FName CharacterId);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Customization")
	bool IsCharacterUnlocked(FName CharacterId) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Achievements")
	bool IsAchievementUnlocked(EPMEProfileAchievement Achievement) const;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Achievements")
	void NotifyMapStarted();

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Achievements")
	void NotifyItemUsed(EPMEPickupType ItemType);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Achievements")
	void NotifyMapCompleted();

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Achievements")
	void NotifyRunCompleted(EPMEPlayMode PlayMode);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Profile|Statistics")
	int32 GetCompletedRunCount(EPMEPlayMode PlayMode) const;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Profile")
	bool SaveProfile();

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Profile")
	void ResetProfile();

	static const TArray<FPMECharacterCosmeticDefinition>& GetNativeCharacterCatalog();
	static bool IsKnownCharacterId(FName CharacterId);
	static FSoftObjectPath GetCharacterMaterialPath(FName CharacterId);

private:
	void LoadOrCreateProfile();
	bool UnlockAchievement(EPMEProfileAchievement Achievement);
	UPMEPlayerProfileSaveGame* GetMutableProfile() const;

	UPROPERTY(Transient)
	TObjectPtr<UPMEPlayerProfileSaveGame> Profile;

	// Per-map state intentionally is not persisted.
	bool bTorchUsedInCurrentMap = false;
};
