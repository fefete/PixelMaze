#include "PMECharacterCustomizationSubsystem.h"

#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PMEPlayerProfileSaveGame.h"

namespace PixelMazeCosmetics
{
	static const TCHAR* SaveSlotName = TEXT("PixelMazePlayerProfile_v1");
	static constexpr int32 SaveUserIndex = 0;

	static FPMECharacterCosmeticDefinition MakeDefinition(
		const TCHAR* Id,
		const TCHAR* TextKey,
		const TCHAR* PreviewTexturePath,
		const TCHAR* MaterialPath,
		const bool bUnlockedByDefault,
		const EPMEProfileAchievement RequiredAchievement)
	{
		FPMECharacterCosmeticDefinition Definition;
		Definition.CharacterId = FName(Id);
		Definition.DisplayNameTextKey = FName(TextKey);
		Definition.bUnlockedByDefault = bUnlockedByDefault;
		Definition.RequiredAchievement = RequiredAchievement;
		Definition.PreviewTexture = TSoftObjectPtr<UTexture2D>(
			FSoftObjectPath(PreviewTexturePath));
		Definition.Material = TSoftObjectPtr<UMaterialInterface>(
			FSoftObjectPath(MaterialPath));
		return Definition;
	}

	static const TArray<FPMECharacterCosmeticDefinition>& Catalog()
	{
		static const TArray<FPMECharacterCosmeticDefinition> Definitions =
		{
			MakeDefinition(
				TEXT("Character.01"),
				TEXT("Character.01.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_01.T_Character_01"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_01.M_Character_01"),
				true,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.02"),
				TEXT("Character.02.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_02.T_Character_02"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_02.M_Character_02"),
				true,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.03"),
				TEXT("Character.03.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_03.T_Character_03"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_03.M_Character_03"),
				true,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.04"),
				TEXT("Character.04.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_04.T_Character_04"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_04.M_Character_04"),
				true,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.05"),
				TEXT("Character.05.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_05.T_Character_05"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_05.M_Character_05"),
				true,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.06"),
				TEXT("Character.06.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_06.T_Character_06"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_06.M_Character_06"),
				false,
				EPMEProfileAchievement::CompleteAnyRun),
			MakeDefinition(
				TEXT("Character.07"),
				TEXT("Character.07.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_07.T_Character_07"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_07.M_Character_07"),
				false,
				EPMEProfileAchievement::UseLightBomb),
			MakeDefinition(
				TEXT("Character.08"),
				TEXT("Character.08.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_08.T_Character_08"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_08.M_Character_08"),
				false,
				EPMEProfileAchievement::UseMapPulse),
			MakeDefinition(
				TEXT("Character.09"),
				TEXT("Character.09.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_09.T_Character_09"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_09.M_Character_09"),
				false,
				EPMEProfileAchievement::CompleteMapWithoutTorch),
			MakeDefinition(
				TEXT("Character.10"),
				TEXT("Character.10.Name"),
				TEXT("/Game/PixelMaze/Textures/T_Character_10.T_Character_10"),
				TEXT("/Game/PixelMaze/Materials/Characters/M_Character_10.M_Character_10"),
				false,
				EPMEProfileAchievement::UseDecoy)
		};

		return Definitions;
	}
}

void UPMECharacterCustomizationSubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreateProfile();
}

void UPMECharacterCustomizationSubsystem::Deinitialize()
{
	SaveProfile();
	Profile = nullptr;
	Super::Deinitialize();
}

TArray<FPMECharacterCosmeticDefinition>
UPMECharacterCustomizationSubsystem::GetCharacterDefinitions() const
{
	return GetNativeCharacterCatalog();
}

FName UPMECharacterCustomizationSubsystem::GetSelectedCharacterId() const
{
	const UPMEPlayerProfileSaveGame* CurrentProfile = Profile;
	if (CurrentProfile &&
		IsKnownCharacterId(CurrentProfile->SelectedCharacterId))
	{
		return CurrentProfile->SelectedCharacterId;
	}

	return TEXT("Character.01");
}

bool UPMECharacterCustomizationSubsystem::SelectCharacter(
	const FName CharacterId)
{
	if (!IsKnownCharacterId(CharacterId) ||
		!IsCharacterUnlocked(CharacterId))
	{
		return false;
	}

	UPMEPlayerProfileSaveGame* CurrentProfile = GetMutableProfile();
	if (!CurrentProfile)
	{
		return false;
	}

	if (CurrentProfile->SelectedCharacterId == CharacterId)
	{
		return true;
	}

	CurrentProfile->SelectedCharacterId = CharacterId;
	SaveProfile();
	OnCharacterSelectionChanged.Broadcast(CharacterId);
	OnProfileChanged.Broadcast();
	return true;
}

bool UPMECharacterCustomizationSubsystem::IsCharacterUnlocked(
	const FName CharacterId) const
{
	for (const FPMECharacterCosmeticDefinition& Definition
	     : GetNativeCharacterCatalog())
	{
		if (Definition.CharacterId != CharacterId)
		{
			continue;
		}

		return Definition.bUnlockedByDefault ||
			IsAchievementUnlocked(Definition.RequiredAchievement);
	}

	return false;
}

bool UPMECharacterCustomizationSubsystem::IsAchievementUnlocked(
	const EPMEProfileAchievement Achievement) const
{
	return Profile &&
		Profile->UnlockedAchievements.Contains(Achievement);
}

void UPMECharacterCustomizationSubsystem::NotifyMapStarted()
{
	bTorchUsedInCurrentMap = false;
}

void UPMECharacterCustomizationSubsystem::NotifyItemUsed(
	const EPMEPickupType ItemType)
{
	bool bProfileChanged = false;

	switch (ItemType)
	{
	case EPMEPickupType::Torch:
		bTorchUsedInCurrentMap = true;
		break;

	case EPMEPickupType::LightBomb:
		bProfileChanged |= UnlockAchievement(
			EPMEProfileAchievement::UseLightBomb);
		break;

	case EPMEPickupType::MapPulse:
		bProfileChanged |= UnlockAchievement(
			EPMEProfileAchievement::UseMapPulse);
		break;

	case EPMEPickupType::Decoy:
		bProfileChanged |= UnlockAchievement(
			EPMEProfileAchievement::UseDecoy);
		break;

	default:
		break;
	}

	if (bProfileChanged)
	{
		SaveProfile();
		OnProfileChanged.Broadcast();
	}
}

void UPMECharacterCustomizationSubsystem::NotifyMapCompleted()
{
	if (bTorchUsedInCurrentMap)
	{
		return;
	}

	if (UnlockAchievement(
		EPMEProfileAchievement::CompleteMapWithoutTorch))
	{
		SaveProfile();
		OnProfileChanged.Broadcast();
	}
}

void UPMECharacterCustomizationSubsystem::NotifyRunCompleted(
	const EPMEPlayMode PlayMode)
{
	UPMEPlayerProfileSaveGame* CurrentProfile = GetMutableProfile();
	if (!CurrentProfile)
	{
		return;
	}

	switch (PlayMode)
	{
	case EPMEPlayMode::Coop:
		++CurrentProfile->CoopRunsCompleted;
		break;

	case EPMEPlayMode::Versus:
		++CurrentProfile->VersusRunsCompleted;
		break;

	default:
		++CurrentProfile->SinglePlayerRunsCompleted;
		break;
	}

	UnlockAchievement(EPMEProfileAchievement::CompleteAnyRun);
	SaveProfile();
	OnProfileChanged.Broadcast();
}

int32 UPMECharacterCustomizationSubsystem::GetCompletedRunCount(
	const EPMEPlayMode PlayMode) const
{
	if (!Profile)
	{
		return 0;
	}

	switch (PlayMode)
	{
	case EPMEPlayMode::Coop:
		return Profile->CoopRunsCompleted;

	case EPMEPlayMode::Versus:
		return Profile->VersusRunsCompleted;

	default:
		return Profile->SinglePlayerRunsCompleted;
	}
}

bool UPMECharacterCustomizationSubsystem::SaveProfile()
{
	return Profile &&
		UGameplayStatics::SaveGameToSlot(
			Profile,
			PixelMazeCosmetics::SaveSlotName,
			PixelMazeCosmetics::SaveUserIndex);
}

void UPMECharacterCustomizationSubsystem::ResetProfile()
{
	Profile = Cast<UPMEPlayerProfileSaveGame>(
		UGameplayStatics::CreateSaveGameObject(
			UPMEPlayerProfileSaveGame::StaticClass()));

	bTorchUsedInCurrentMap = false;
	SaveProfile();
	OnCharacterSelectionChanged.Broadcast(GetSelectedCharacterId());
	OnProfileChanged.Broadcast();
}

const TArray<FPMECharacterCosmeticDefinition>&
UPMECharacterCustomizationSubsystem::GetNativeCharacterCatalog()
{
	return PixelMazeCosmetics::Catalog();
}

bool UPMECharacterCustomizationSubsystem::IsKnownCharacterId(
	const FName CharacterId)
{
	return GetNativeCharacterCatalog().ContainsByPredicate(
		[CharacterId](const FPMECharacterCosmeticDefinition& Definition)
		{
			return Definition.CharacterId == CharacterId;
		});
}

FSoftObjectPath
UPMECharacterCustomizationSubsystem::GetCharacterMaterialPath(
	const FName CharacterId)
{
	const FPMECharacterCosmeticDefinition* Definition =
		GetNativeCharacterCatalog().FindByPredicate(
			[CharacterId](const FPMECharacterCosmeticDefinition& Candidate)
			{
				return Candidate.CharacterId == CharacterId;
			});

	if (!Definition)
	{
		Definition = &GetNativeCharacterCatalog()[0];
	}

	return Definition->Material.ToSoftObjectPath();
}

void UPMECharacterCustomizationSubsystem::LoadOrCreateProfile()
{
	if (UGameplayStatics::DoesSaveGameExist(
		PixelMazeCosmetics::SaveSlotName,
		PixelMazeCosmetics::SaveUserIndex))
	{
		Profile = Cast<UPMEPlayerProfileSaveGame>(
			UGameplayStatics::LoadGameFromSlot(
				PixelMazeCosmetics::SaveSlotName,
				PixelMazeCosmetics::SaveUserIndex));
	}

	if (!Profile)
	{
		Profile = Cast<UPMEPlayerProfileSaveGame>(
			UGameplayStatics::CreateSaveGameObject(
				UPMEPlayerProfileSaveGame::StaticClass()));
	}

	if (!IsKnownCharacterId(Profile->SelectedCharacterId) ||
		!IsCharacterUnlocked(Profile->SelectedCharacterId))
	{
		Profile->SelectedCharacterId = TEXT("Character.01");
	}

	SaveProfile();
}

bool UPMECharacterCustomizationSubsystem::UnlockAchievement(
	const EPMEProfileAchievement Achievement)
{
	UPMEPlayerProfileSaveGame* CurrentProfile = GetMutableProfile();
	if (!CurrentProfile ||
		CurrentProfile->UnlockedAchievements.Contains(Achievement))
	{
		return false;
	}

	CurrentProfile->UnlockedAchievements.Add(Achievement);
	OnAchievementUnlocked.Broadcast(Achievement);
	return true;
}

UPMEPlayerProfileSaveGame*
UPMECharacterCustomizationSubsystem::GetMutableProfile() const
{
	return Profile;
}
