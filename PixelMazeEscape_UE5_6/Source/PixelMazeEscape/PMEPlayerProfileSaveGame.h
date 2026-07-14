#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PMECharacterCosmeticTypes.h"
#include "PMEPlayerProfileSaveGame.generated.h"

UCLASS()
class PIXELMAZEESCAPE_API UPMEPlayerProfileSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile")
	int32 SaveVersion = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile")
	FName SelectedCharacterId = TEXT("Character.01");

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile")
	TArray<EPMEProfileAchievement> UnlockedAchievements;

	// Kept separately so future achievements can distinguish each mode.
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile|Statistics")
	int32 SinglePlayerRunsCompleted = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile|Statistics")
	int32 CoopRunsCompleted = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Pixel Maze|Profile|Statistics")
	int32 VersusRunsCompleted = 0;
};
