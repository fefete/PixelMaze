#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PMETypes.h"
#include "PMEGameModeBase.generated.h"

class APMEDecoyActor;
class APMEExitActor;
class APMEGameState;
class APMEMazeGenerator;
class APMEObjectManager;
class APMEPatrolEnemy;
class APMEPlayerCharacter;
class APMEPlayerState;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	APMEGameModeBase();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	                      FString& ErrorMessage) override;

	void HandlePlayerReachedExit(APMEPlayerCharacter* PlayerCharacter);
	void HandlePlayerCaughtByEnemy(APMEPlayerCharacter* PlayerCharacter);
	void HandleObjectiveProgressChanged(APMEPlayerState* ActivatingPlayer);
	void HandleAllObjectivesCompleted();
	void HandleUpgradeSelected(APMEPlayerState* PlayerState, EPMEUpgradeType UpgradeType);
	void EmitNoise(const FVector& Location, float RadiusInTiles, AActor* Source, FName NoiseTag);
	void SpawnDecoy(APMEPlayerCharacter* SourceCharacter);
	void StunEnemiesAround(const FVector& Location, float RadiusInTiles, float Duration);
	void UseMasterKey(APMEPlayerState* PlayerState);
	void TryReviveNearestTeammate(APMEPlayerCharacter* Reviver);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze")
	void RestartCurrentMaze();
	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	EPMEPlayMode GetPlayMode() const { return PlayMode; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objectives")
	APMEObjectManager* GetObjectiveManager() const { return ObjectiveManager; }

protected:
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze")
	TSubclassOf<APMEMazeGenerator> MazeGeneratorClass;
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze")
	TSubclassOf<APMEExitActor> ExitActorClass;
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze")
	TSubclassOf<APMEObjectManager> ObjectiveManagerClass;
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze")
	TSubclassOf<APMEDecoyActor> DecoyClass;
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze", meta=(ClampMin="0.1"))
	float TransitionDelay = 2.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Run", meta=(ClampMin="1", ClampMax="20"))
	int32 MaximumRunLevels = 5;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Run", meta=(ClampMin="1", ClampMax="4"))
	int32 BaseObjectiveCount = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Run", meta=(ClampMin="0", ClampMax="8"))
	int32 BasePickupCount = 3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Run", meta=(ClampMin="0.0", ClampMax="30.0"))
	float UpgradeSelectionTimeout = 15.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Coop", meta=(ClampMin="0.5", ClampMax="5.0"))
	float ReviveRadiusInTiles = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies")
	bool bSpawnPatrolEnemies = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies")
	TSubclassOf<APMEPatrolEnemy> PatrolEnemyClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="0"))
	int32 BaseEnemyCount = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="0"))
	int32 EnemiesAddedPerLevel = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="0"))
	int32 MaximumEnemyCount = 8;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="2"))
	int32 EnemyPatrolRouteLengthInTiles = 10;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="10.0"))
	float EnemyMovementSpeed = 170.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies",
		meta=(ClampMin="0.0", ClampMax="5.0"))
	float EnemyWaypointPauseDuration = 0.15f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase", meta=(ClampMin="1.0"))
	float EnemySightRadiusInTiles = 7.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase", meta=(ClampMin="1.0"))
	float EnemyChaseDistanceInTiles = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase", meta=(ClampMin="10.0"))
	float EnemyChaseMovementSpeed = 245.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase", meta=(ClampMin="10.0"))
	float EnemyReturnMovementSpeed = 195.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase",
		meta=(ClampMin="0.02", ClampMax="2.0"))
	float EnemySightCheckInterval = 0.10f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Chase",
		meta=(ClampMin="0.02", ClampMax="2.0"))
	float EnemyChasePathRefreshInterval = 0.15f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Hearing", meta=(ClampMin="1.0"))
	float EnemyHearingRadiusInTiles = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Hearing",
		meta=(ClampMin="0.1", ClampMax="10.0"))
	float EnemyInvestigationDuration = 2.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="0"))
	int32 EnemySafeZoneRadiusInTiles = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies", meta=(ClampMin="1"))
	int32 EnemyMinimumSeparationInTiles = 4;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Patrol Safety",
		meta=(ClampMin="0", ClampMax="3"))
	int32 EnemyProgressRouteClearanceInTiles = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Patrol Safety")
	bool bAllowPatrolTransitAcrossProgressRoutes = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Patrol Safety")
	bool bPreventPatrolRouteOverlap = true;

	/** Maximum number of consecutive critical-route/object cells in a patrol route. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Patrol Safety",
		meta=(ClampMin="1", ClampMax="6"))
	int32 EnemyMaximumConsecutiveTransitTiles = 2;

	/** Reserve at least one adjacent tile where a player can wait before collecting an object. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies|Patrol Safety")
	bool bReserveInteractionApproachCell = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemies",
		meta=(ClampMin="0.0", ClampMax="10.0"))
	float EnemyContactResetCooldown = 1.0f;

private:
	void StartMaze(bool bUseNewSeed);
	void FinishRound(APMEPlayerState* WinnerPlayerState, float CompletionTime, bool bDefeat = false);
	void BeginUpgradeSelection();
	void AdvanceToNextMaze();
	void FinishRun();
	void EnterWaitingForPlayers();
	void EnterEscapePhase();
	void EnsureWorldLighting();
	void PlaceAllPlayersAtStart();
	void EnsurePlayerPawn(APlayerController* PlayerController);
	void SetAllPlayersMoveIgnored(bool bIgnored);
	void AssignPlayerIndex(APMEPlayerState* NewPlayerState);
	void SpawnPatrolEnemies();
	void DestroyPatrolEnemies();
	void SetPatrolEnemiesEnabled(bool bEnabled);
	void ApplyEscapeEnemyEscalation();
	void ResolveUpgradeTimeout();
	TArray<EPMEUpgradeType> BuildUpgradeChoices(const APMEPlayerState* PlayerState) const;
	EPMELevelModifier SelectLevelModifier(int32 Level) const;
	bool AreAllPlayersDowned() const;
	bool HaveAllPlayersSelectedUpgrade() const;
	APMEPlayerState* FindOtherActivePlayer(const APMEPlayerState* PlayerState) const;

	TArray<FIntPoint> BuildPatrolRoute(FIntPoint StartCell, FRandomStream& RandomStream,
	                                   const TSet<FIntPoint>& HardForbiddenPatrolCells,
	                                   const TSet<FIntPoint>& TransitOnlyPatrolCells,
	                                   const TSet<FIntPoint>& OccupiedPatrolCells) const;
	TArray<FIntPoint> GetEnemyProtectedCells() const;
	TSet<FIntPoint> BuildTransitOnlyPatrolCells() const;
	TSet<FIntPoint> BuildHardForbiddenPatrolCells(const TSet<FIntPoint>& TransitOnlyPatrolCells) const;
	TSet<FIntPoint> BuildReservedObjectApproachCells(const TSet<FIntPoint>& TransitOnlyPatrolCells) const;
	bool IsValidPatrolRoute(const TArray<FIntPoint>& Route, const TSet<FIntPoint>& HardForbiddenPatrolCells,
	                        const TSet<FIntPoint>& TransitOnlyPatrolCells) const;
	bool BuildWalkableGridPath(FIntPoint StartCell, FIntPoint GoalCell, TArray<FIntPoint>& OutPath) const;
	void AddPathToCellSet(const TArray<FIntPoint>& Path, TSet<FIntPoint>& InOutCells) const;
	bool IsCellInsideEnemySafeZone(FIntPoint Cell, const TArray<FIntPoint>& ProtectedCells) const;
	int32 CountWalkableNeighbours(FIntPoint Cell) const;
	bool AreAllCoopPlayersAtExit() const;
	int32 GetConnectedPlayerCount();

	UPROPERTY()
	TObjectPtr<APMEMazeGenerator> MazeGenerator;
	UPROPERTY()
	TObjectPtr<APMEExitActor> ExitActor;
	UPROPERTY()
	TObjectPtr<APMEGameState> MazeGameState;
	UPROPERTY()
	TObjectPtr<APMEObjectManager> ObjectiveManager;
	UPROPERTY()
	TArray<TObjectPtr<APMEPatrolEnemy>> PatrolEnemies;
	UPROPERTY()
	TArray<TObjectPtr<APMEDecoyActor>> ActiveDecoys;
	TMap<TWeakObjectPtr<APMEPlayerCharacter>, float> LastEnemyContactTimes;
	TSet<TWeakObjectPtr<APMEPlayerState>> PlayersWithUpgradeSelection;
	FTimerHandle TransitionTimer;
	EPMEPlayMode PlayMode = EPMEPlayMode::SinglePlayer;
	EPMELevelModifier CurrentModifier = EPMELevelModifier::None;
	int32 CurrentLevel = 1;
	bool bRoundTransitionPending = false;
};
