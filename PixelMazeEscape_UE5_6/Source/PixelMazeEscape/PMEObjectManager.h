#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMETypes.h"
#include "PMEObjectManager.generated.h"

class APMEMazeGenerator;
class APMEObjectActor;
class APMEPickup;
class APMEPlayerState;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEObjectManager : public AActor
{
	GENERATED_BODY()

public:
	APMEObjectManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeObjectives(APMEMazeGenerator* InMaze, EPMEPlayMode InMode, int32 Level, int32 Seed,
	                          int32 ObjectiveCount, int32 PickupCount);
	void HandleObjectiveActivated(APMEObjectActor* Objective, APMEPlayerState* ActivatingPlayer);
	bool CompleteOneObjectiveWithMasterKey(APMEPlayerState* PlayerState);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	bool CanPlayerUseExit(const APMEPlayerState* PlayerState) const;
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetRequiredObjectivesForPlayer(const APMEPlayerState* PlayerState) const;
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	int32 GetActivatedObjectivesForPlayer(const APMEPlayerState* PlayerState) const;
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	bool AreAllRequiredObjectivesComplete() const;

private:
	TArray<FIntPoint> BuildCandidateCells() const;
	FIntPoint PickCandidateCell(TArray<FIntPoint>& Candidates, FRandomStream& Stream, FIntPoint Reference,
	                            int32 MinimumDistance) const;
	bool SpawnObjectiveAt(FIntPoint Cell, int32 OwnerPlayerIndex, EPMEObjectType Type, int32 ObjectiveId);
	void SpawnPickupAt(FIntPoint Cell, EPMEPickupType Type, int32 OwnerPlayerIndex);
	void RefreshReplicatedProgress();

	UPROPERTY()
	TObjectPtr<APMEMazeGenerator> Maze;
	UPROPERTY()
	TArray<TObjectPtr<APMEObjectActor>> Objectives;
	UPROPERTY()
	TArray<TObjectPtr<APMEPickup>> Pickups;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<APMEObjectActor> ObjectiveClass;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<APMEPickup> PickupClass;
	UPROPERTY(Replicated)
	EPMEPlayMode PlayMode = EPMEPlayMode::SinglePlayer;
	UPROPERTY(Replicated)
	int32 SharedRequired = 0;
	UPROPERTY(Replicated)
	int32 SharedActivated = 0;
	UPROPERTY(Replicated)
	int32 Player1Required = 0;
	UPROPERTY(Replicated)
	int32 Player1Activated = 0;
	UPROPERTY(Replicated)
	int32 Player2Required = 0;
	UPROPERTY(Replicated)
	int32 Player2Activated = 0;
};
