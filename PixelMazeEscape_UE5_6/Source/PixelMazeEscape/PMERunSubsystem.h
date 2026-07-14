#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PMERunSubsystem.generated.h"

UCLASS(BlueprintType)
class PIXELMAZEESCAPE_API UPMERunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Run")
	void ResetRun(int32 InMaximumLevels = 5);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Run")
	void AdvanceLevel();

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Run")
	void AddScore(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Run")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Run")
	int32 GetMaximumLevels() const { return MaximumLevels; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Run")
	int32 GetTotalScore() const { return TotalScore; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Run")
	int32 GetRunSeed() const { return RunSeed; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Run")
	bool IsFinalLevel() const { return CurrentLevel >= MaximumLevels; }

private:
	UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Run", meta=(AllowPrivateAccess="true"))
	int32 CurrentLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Run", meta=(AllowPrivateAccess="true"))
	int32 MaximumLevels = 5;

	UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Run", meta=(AllowPrivateAccess="true"))
	int32 TotalScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Run", meta=(AllowPrivateAccess="true"))
	int32 RunSeed = 0;
};
