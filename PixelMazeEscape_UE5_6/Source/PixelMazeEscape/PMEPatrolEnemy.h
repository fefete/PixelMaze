#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMETypes.h"
#include "PMEPatrolEnemy.generated.h"

class APMEMazeGenerator;
class APMEPlayerCharacter;
class UCapsuleComponent;
class UPrimitiveComponent;
class USoundBase;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPMEEnemyBehaviourState : uint8
{
	Patrol UMETA(DisplayName="Patrol"),
	Chase UMETA(DisplayName="Chase"),
	ReturnToPatrol UMETA(DisplayName="Return To Patrol"),
	Investigate UMETA(DisplayName="Investigate"),
	Stunned UMETA(DisplayName="Stunned")
};

/**
 * Server-authoritative maze enemy.
 *
 * It patrols a deterministic route, detects players using a real visibility
 * trace, pursues the selected player for a configurable travel budget measured
 * in maze tiles, returns to the exact point where the chase started and then
 * resumes the interrupted patrol.
 *
 * Only the server executes perception, pathfinding and movement. Replicated
 * movement keeps both multiplayer clients in the same state. Audio is evaluated
 * locally from the replicated transform so the listen server does not multicast
 * every individual footstep.
 */
UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEPatrolEnemy : public AActor
{
	GENERATED_BODY()

public:
	APMEPatrolEnemy();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called by the authoritative GameMode immediately after spawning. */
	void InitializePatrol(
		APMEMazeGenerator* InMazeGenerator,
		const TArray<FVector>& InPatrolPoints,
		const TArray<uint8>& InTransitOnlyPointFlags,
		const TSet<FIntPoint>& InTransitOnlyCells,
		float InPatrolMovementSpeed,
		float InWaypointPauseDuration,
		float InSightRadiusInTiles,
		float InChaseDistanceInTiles,
		float InChaseMovementSpeed,
		float InReturnMovementSpeed,
		float InSightCheckInterval,
		float InPathRefreshInterval,
		float InHearingRadiusInTiles,
		float InInvestigationDuration,
		EPMEEnemyArchetype InArchetype);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Enemy")
	void SetPatrolEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Enemy|Perception")
	void HearNoise(const FVector& NoiseLocation, float RadiusInTiles, AActor* NoiseSource, FName NoiseTag);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Enemy")
	void ApplyStun(float Duration);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Enemy")
	void SetAlertMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Enemy")
	bool IsPatrolEnabled() const { return bPatrolEnabled; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Enemy")
	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Enemy|Perception")
	EPMEEnemyBehaviourState GetBehaviourState() const { return BehaviourState; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Enemy|Perception")
	float GetRemainingChaseDistanceInTiles() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> PatrolCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PixelBody;

	/**
	 * World-up orientation used by the top-down pixel body. It never inherits
	 * movement direction from the collision actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Visual")
	FRotator PixelBodyUprightRotation = FRotator::ZeroRotator;

	/**
	 * World Y is screen-horizontal with the project's top-down camera.
	 * Negative Y scale mirrors the sprite when moving left.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Visual")
	bool bMirrorPixelBodyWhenMovingLeft = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol", meta=(ClampMin="10.0"))
	float PatrolMovementSpeed = 170.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol", meta=(ClampMin="1.0"))
	float WaypointAcceptanceRadius = 5.0f;

	/** Pause applied at patrol route ends. Intermediate grid cells are crossed continuously. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol",
		meta=(ClampMin="0.0", ClampMax="5.0"))
	float WaypointPauseDuration = 0.15f;

	/**
	 * When enabled, WaypointPauseDuration is used only where a ping-pong patrol
	 * reverses direction. Disabling this restores the legacy pause at every
	 * non-transit waypoint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol")
	bool bPauseOnlyAtPatrolEndpoints = true;

	/**
	 * Safety limit for the number of intermediate patrol cells that may be
	 * consumed during one frame. This permits continuous movement through
	 * tile centres without risking an infinite loop on malformed routes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol", meta=(ClampMin="1", ClampMax="64"))
	int32 MaximumPatrolWaypointsPerTick = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Patrol")
	bool bPingPongPatrol = true;

	/** Detection radius measured in tiles. A target must share the same grid row or column. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="1.0", UIMin="1.0"))
	float SightRadiusInTiles = 7.0f;

	/** Maximum distance the enemy may physically travel while chasing, measured in tiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="1.0", UIMin="1.0"))
	float ChaseDistanceInTiles = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="10.0", UIMin="10.0"))
	float ChaseMovementSpeed = 245.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="10.0", UIMin="10.0"))
	float ReturnMovementSpeed = 195.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="0.02", ClampMax="2.0"))
	float SightCheckInterval = 0.10f;

	/** How frequently the grid path to a moving player is recalculated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="0.02", ClampMax="2.0"))
	float ChasePathRefreshInterval = 0.15f;

	/** Time spent blocked before the chase path is rebuilt from the current grid cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="0.02", ClampMax="1.0"))
	float ChaseBlockedRepathDelay = 0.10f;

	/** Consecutive failed path rebuilds before abandoning the chase safely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="1", ClampMax="10"))
	int32 MaximumChasePathFailures = 3;

	/**
	 * Short tolerance for a single missed trace. While this expires, the enemy
	 * continues towards the last visible position but never reads the hidden
	 * player's current position.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Perception",
		meta=(ClampMin="0.0", ClampMax="2.0"))
	float LostSightGraceDuration = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pixel Maze|Enemy|Debug")
	bool bDrawSightDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio")
	TObjectPtr<USoundBase> EnemyStepSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio")
	TObjectPtr<USoundBase> EnemyVocalSound;

	/** Distance between enemy footstep sounds, measured in maze tiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio",
		meta=(ClampMin="0.1", ClampMax="3.0"))
	float EnemyStepDistanceInTiles = 0.72f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio",
		meta=(ClampMin="0.0", ClampMax="1.0"))
	float EnemyStepVolume = 0.44f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio",
		meta=(ClampMin="0.0", ClampMax="1.0"))
	float EnemyVocalVolume = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio",
		meta=(ClampMin="1.0", ClampMax="60.0"))
	float EnemyVocalMinimumInterval = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Enemy|Audio",
		meta=(ClampMin="1.0", ClampMax="60.0"))
	float EnemyVocalMaximumInterval = 9.0f;

private:
	UFUNCTION()
	void OnPatrolCollisionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_FacingRight();

	/** Keeps the pixel body upright and mirrors it only on the screen-horizontal axis. */
	void ApplyPixelBodyFacing();

	/** Updates left/right facing from a cardinal movement direction. Vertical movement preserves the last side. */
	void UpdatePixelBodyFacing(const FIntPoint& CardinalDirection);

	void TickPatrol(float DeltaSeconds);
	void TickChase(float DeltaSeconds);
	void TickReturn(float DeltaSeconds);
	void TickInvestigate(float DeltaSeconds);
	void BeginInvestigate(const FVector& NoiseLocation);

	void CheckForVisiblePlayer();
	APMEPlayerCharacter* FindClosestVisiblePlayer() const;
	bool HasLineOfSightTo(const APMEPlayerCharacter* PlayerCharacter) const;

	void BeginChase(APMEPlayerCharacter* PlayerCharacter);
	void CapturePatrolReturnState();
	int32 FindSafeNonTransitReturnWaypointIndex() const;
	bool HasValidPatrolReturnState() const;
	bool RecoverPatrolReturnState();
	void BeginReturnToPatrol();
	void FinishReturnToPatrol();
	bool BuildReturnPath();

	bool RefreshPathToWorldLocation(const FVector& TargetWorldLocation);
	bool BuildGridPath(FIntPoint StartCell, FIntPoint GoalCell, TArray<FVector>& OutWorldPath) const;
	bool MoveAlongActivePath(float DeltaSeconds, float Speed, bool bConsumeChaseBudget,
	                         const FVector* FinalExactLocation = nullptr);
	float MoveTowardsLocation(const FVector& TargetLocation, float DeltaSeconds, float Speed);
	FIntPoint FindBestWalkableCellForLocation(const FVector& WorldLocation) const;
	FVector GetGridCellCenter(FIntPoint Cell) const;
	FVector GetSafePointInsideCell(const FVector& DesiredLocation, FIntPoint Cell) const;
	bool IsCardinalWalkableSegment(FIntPoint StartCell, FIntPoint TargetCell) const;

	void AdvanceWaypoint();
	bool IsPatrolEndpoint(int32 PointIndex) const;
	bool IsPatrolPointTransitOnly(int32 PointIndex) const;
	bool IsCrossingTransitOnlySection() const;
	bool IsRoundActive() const;
	bool IsValidChaseTarget(const APMEPlayerCharacter* PlayerCharacter) const;
	float GetTileSize() const;

	void ResolveAudioAssets();
	void TickLocalAudio(float DeltaSeconds);
	void ResetLocalAudioTracking();

	UPROPERTY()
	TObjectPtr<APMEMazeGenerator> MazeGenerator;

	UPROPERTY()
	TWeakObjectPtr<APMEPlayerCharacter> ChaseTarget;

	TArray<FVector> PatrolPoints;
	TArray<uint8> PatrolTransitOnlyPointFlags;
	TSet<FIntPoint> TransitOnlyCells;
	TArray<FVector> ActivePathPoints;

	/** Exact point on the patrol segment where patrol was interrupted. */
	FVector ChaseStartLocation = FVector::ZeroVector;

	/** Patrol waypoint used as the safe grid-path destination before returning along the original segment. */
	FVector ReturnPathAnchorLocation = FVector::ZeroVector;

	/** First safe grid point reached before calculating the return path. */
	FVector ReturnDepartureAnchorLocation = FVector::ZeroVector;

	FIntPoint LastPathGoalCell = FIntPoint(-1, -1);
	int32 SavedPatrolWaypointIndex = 0;
	int32 SavedPatrolDirection = 1;
	int32 CurrentWaypointIndex = 0;
	int32 PatrolDirection = 1;
	int32 ActivePathPointIndex = 0;

	float RemainingPauseTime = 0.0f;
	float RemainingSightCheckTime = 0.0f;
	float RemainingPathRefreshTime = 0.0f;
	float RemainingLostSightGraceTime = 0.0f;
	float BlockedChaseTime = 0.0f;
	int32 ConsecutiveChasePathFailures = 0;

	/** Centre of the last grid cell confirmed by both grid and physics visibility. */
	FVector LastKnownTargetLocation = FVector::ZeroVector;
	FIntPoint LastKnownTargetCell = FIntPoint(-1, -1);
	bool bChaseTargetVisible = false;

	UPROPERTY(Replicated)
	float RemainingChaseDistanceWorld = 0.0f;

	UPROPERTY(Replicated)
	float CachedTileSize = 100.0f;

	UPROPERTY(Replicated)
	EPMEEnemyBehaviourState BehaviourState = EPMEEnemyBehaviourState::Patrol;

	UPROPERTY(Replicated)
	EPMEEnemyArchetype EnemyArchetype = EPMEEnemyArchetype::Guardian;

	/** True means the pixel body uses its normal horizontal UV orientation. */
	UPROPERTY(ReplicatedUsing=OnRep_FacingRight)
	bool bFacingRight = true;

	float HearingRadiusInTiles = 8.0f;
	float InvestigationDuration = 2.5f;
	float RemainingInvestigationTime = 0.0f;
	float RemainingStunTime = 0.0f;
	float AlertMultiplier = 1.0f;
	FVector InvestigationLocation = FVector::ZeroVector;

	bool bPatrolEnabled = true;

	/** True only after a valid patrol segment and return location have been captured. */
	bool bHasPatrolReturnState = false;
	bool bHasReturnPathAnchor = false;
	bool bReturnAnchorReached = false;
	bool bAudioTrackingInitialized = false;
	FVector LastAudioLocation = FVector::ZeroVector;
	float AccumulatedAudioDistance = 0.0f;
	float RemainingVocalDelay = 0.0f;

	/** Last authoritative movement direction. Each component is always -1, 0 or 1 and only one axis may be non-zero. */
	FIntPoint LastCardinalMoveDirection = FIntPoint::ZeroValue;
	bool bMovementBlockedThisTick = false;
	bool bPathSegmentInvalidThisTick = false;
	bool bMadeMovementProgressThisTick = false;
};
