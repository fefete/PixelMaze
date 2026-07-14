#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PMETypes.h"
#include "PMEGameState.generated.h"

UCLASS()
class PIXELMAZEESCAPE_API APMEGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	APMEGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	EPMEPlayMode GetPlayMode() const { return PlayMode; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	int32 GetActiveSeed() const { return ActiveSeed; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	bool IsWaitingForPlayers() const { return RoundPhase == EPMERoundPhase::WaitingForPlayers; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	bool IsRoundFinished() const
	{
		return RoundPhase == EPMERoundPhase::RoundResult || RoundPhase == EPMERoundPhase::UpgradeSelection || RoundPhase
			== EPMERoundPhase::RunComplete;
	}

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	EPMERoundPhase GetRoundPhase() const { return RoundPhase; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	EPMELevelModifier GetLevelModifier() const { return LevelModifier; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	float GetElapsedRoundTime() const;
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	float GetBestTime() const { return BestTime; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	int32 GetWinnerPlayerIndex() const { return WinnerPlayerIndex; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Match")
	FString GetWinnerName() const { return WinnerName; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetSharedObjectiveCount() const { return SharedObjectiveCount; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetSharedObjectiveRequired() const { return SharedObjectiveRequired; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetPlayerObjectiveCount(int32 PlayerIndex) const
	{
		return PlayerIndex == 2 ? Player2ObjectiveCount : Player1ObjectiveCount;
	}

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetPlayerObjectiveRequired(int32 PlayerIndex) const
	{
		return PlayerIndex == 2 ? Player2ObjectiveRequired : Player1ObjectiveRequired;
	}

	void ConfigureMatch(EPMEPlayMode NewMode);
	void SetWaitingForPlayers(bool bWaiting);
	void BeginRound(int32 NewLevel, int32 NewSeed, EPMELevelModifier NewModifier);
	void SetRoundPhase(EPMERoundPhase NewPhase);
	void FinishRound(float CompletionTime, int32 NewWinnerPlayerIndex, const FString& NewWinnerName);
	void SetBestTime(float NewBestTime);
	void SetObjectiveProgress(int32 SharedDone, int32 SharedRequired, int32 P1Done, int32 P1Required, int32 P2Done,
	                          int32 P2Required);

private:
	UPROPERTY(Replicated)
	EPMEPlayMode PlayMode = EPMEPlayMode::SinglePlayer;
	UPROPERTY(Replicated)
	int32 CurrentLevel = 1;
	UPROPERTY(Replicated)
	int32 ActiveSeed = 0;
	UPROPERTY(Replicated)
	EPMERoundPhase RoundPhase = EPMERoundPhase::WaitingForPlayers;
	UPROPERTY(Replicated)
	EPMELevelModifier LevelModifier = EPMELevelModifier::None;
	UPROPERTY(Replicated)
	float RoundStartServerTime = 0.0f;
	UPROPERTY(Replicated)
	float FrozenCompletionTime = 0.0f;
	UPROPERTY(Replicated)
	float BestTime = 0.0f;
	UPROPERTY(Replicated)
	int32 WinnerPlayerIndex = 0;
	UPROPERTY(Replicated)
	FString WinnerName;
	UPROPERTY(Replicated)
	int32 SharedObjectiveCount = 0;
	UPROPERTY(Replicated)
	int32 SharedObjectiveRequired = 0;
	UPROPERTY(Replicated)
	int32 Player1ObjectiveCount = 0;
	UPROPERTY(Replicated)
	int32 Player1ObjectiveRequired = 0;
	UPROPERTY(Replicated)
	int32 Player2ObjectiveCount = 0;
	UPROPERTY(Replicated)
	int32 Player2ObjectiveRequired = 0;
};
