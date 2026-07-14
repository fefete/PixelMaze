#include "PMEPatrolEnemy.h"

#include "AbilitySystemComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Queue.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PMEAttributeSet.h"
#include "PMEGameModeBase.h"
#include "PMEGameState.h"
#include "PMEMazeGenerator.h"
#include "PMEPlayerCharacter.h"
#include "PMEPlayerState.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace PMEEnemyPath
{
    const FIntPoint Directions[] =
    {
        FIntPoint(1, 0),
        FIntPoint(-1, 0),
        FIntPoint(0, 1),
        FIntPoint(0, -1)
    };

    constexpr float CardinalAxisTolerance = 0.25f;

    bool HasReachedCardinalPoint(const FVector& Current, const FVector& Target)
    {
        return
            FMath::Abs(Current.X - Target.X) <= CardinalAxisTolerance &&
            FMath::Abs(Current.Y - Target.Y) <= CardinalAxisTolerance;
    }
}

namespace PixelMazeEnemyAudio
{
    static const TCHAR* StepSoundPath =
        TEXT("/Game/PixelMaze/Audio/SW_SFX_Enemy_Step.SW_SFX_Enemy_Step");
    static const TCHAR* VocalSoundPath =
        TEXT("/Game/PixelMaze/Audio/SW_SFX_Enemy_Vocal.SW_SFX_Enemy_Vocal");
}

APMEPatrolEnemy::APMEPatrolEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;

    bReplicates = true;
    SetReplicateMovement(true);
    NetUpdateFrequency = 60.0f;
    MinNetUpdateFrequency = 30.0f;
    NetPriority = 3.0f;

    PatrolCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("PatrolCollision"));
    SetRootComponent(PatrolCollision);
    PatrolCollision->InitCapsuleSize(28.0f, 42.0f);
    PatrolCollision->SetCollisionObjectType(ECC_WorldDynamic);
    PatrolCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PatrolCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    PatrolCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    PatrolCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    PatrolCollision->SetGenerateOverlapEvents(true);

    PixelBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelBody"));
    PixelBody->SetupAttachment(PatrolCollision);
    PixelBody->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
    PixelBody->SetRelativeScale3D(FVector(0.46f, 0.46f, 0.68f));
    PixelBody->SetAbsolute(false, true, true);
    PixelBody->SetWorldRotation(FRotator::ZeroRotator);
    PixelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PixelBody->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        PixelBody->SetStaticMesh(CubeMesh.Object);
    }
}

void APMEPatrolEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APMEPatrolEnemy, RemainingChaseDistanceWorld);
    DOREPLIFETIME(APMEPatrolEnemy, CachedTileSize);
    DOREPLIFETIME(APMEPatrolEnemy, BehaviourState);
    DOREPLIFETIME(APMEPatrolEnemy, EnemyArchetype);
    DOREPLIFETIME(APMEPatrolEnemy, bFacingRight);
}

void APMEPatrolEnemy::BeginPlay()
{
    Super::BeginPlay();

    PatrolCollision->OnComponentBeginOverlap.AddDynamic(
        this,
        &APMEPatrolEnemy::OnPatrolCollisionBeginOverlap);

    if (UMaterialInterface* EnemyMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/PixelMaze/Materials/M_Enemy.M_Enemy")))
    {
        PixelBody->SetMaterial(0, EnemyMaterial);
    }

    // Collision and pathfinding do not need actor facing. Keep the replicated
    // actor and the pixel body upright; only horizontal mirroring changes.
    SetActorRotation(FRotator::ZeroRotator);
    ApplyPixelBodyFacing();

    ResolveAudioAssets();
    ResetLocalAudioTracking();
}

void APMEPatrolEnemy::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    TickLocalAudio(DeltaSeconds);

    if (!HasAuthority() || !bPatrolEnabled || PatrolPoints.Num() < 2 || !IsRoundActive())
    {
        return;
    }

    bMovementBlockedThisTick = false;
    bPathSegmentInvalidThisTick = false;
    bMadeMovementProgressThisTick = false;

    if (BehaviourState == EPMEEnemyBehaviourState::Stunned)
    {
        RemainingStunTime = FMath::Max(0.0f, RemainingStunTime - DeltaSeconds);
        if (RemainingStunTime <= 0.0f) BeginReturnToPatrol();
        return;
    }

    switch (BehaviourState)
    {
        case EPMEEnemyBehaviourState::Chase: TickChase(DeltaSeconds); break;
        case EPMEEnemyBehaviourState::ReturnToPatrol: TickReturn(DeltaSeconds); break;
        case EPMEEnemyBehaviourState::Investigate: TickInvestigate(DeltaSeconds); break;
        case EPMEEnemyBehaviourState::Patrol:
        default: TickPatrol(DeltaSeconds); break;
    }
}

void APMEPatrolEnemy::InitializePatrol(
    APMEMazeGenerator* InMazeGenerator,
    const TArray<FVector>& InPatrolPoints,
    const TArray<uint8>& InTransitOnlyPointFlags,
    const TSet<FIntPoint>& InTransitOnlyCells,
    const float InPatrolMovementSpeed,
    const float InWaypointPauseDuration,
    const float InSightRadiusInTiles,
    const float InChaseDistanceInTiles,
    const float InChaseMovementSpeed,
    const float InReturnMovementSpeed,
    const float InSightCheckInterval,
    const float InPathRefreshInterval,
    const float InHearingRadiusInTiles,
    const float InInvestigationDuration,
    const EPMEEnemyArchetype InArchetype)
{
    check(HasAuthority());

    MazeGenerator = InMazeGenerator;
    CachedTileSize = MazeGenerator ? FMath::Max(1.0f, MazeGenerator->GetTileSize()) : 100.0f;
    PatrolPoints = InPatrolPoints;
    PatrolTransitOnlyPointFlags = InTransitOnlyPointFlags;
    if (PatrolTransitOnlyPointFlags.Num() != PatrolPoints.Num())
    {
        PatrolTransitOnlyPointFlags.Init(0, PatrolPoints.Num());
    }
    TransitOnlyCells = InTransitOnlyCells;
    PatrolMovementSpeed = FMath::Max(10.0f, InPatrolMovementSpeed);
    WaypointPauseDuration = FMath::Max(0.0f, InWaypointPauseDuration);
    SightRadiusInTiles = FMath::Max(1.0f, InSightRadiusInTiles);
    ChaseDistanceInTiles = FMath::Max(1.0f, InChaseDistanceInTiles);
    ChaseMovementSpeed = FMath::Max(10.0f, InChaseMovementSpeed);
    ReturnMovementSpeed = FMath::Max(10.0f, InReturnMovementSpeed);
    SightCheckInterval = FMath::Max(0.02f, InSightCheckInterval);
    ChasePathRefreshInterval = FMath::Max(0.02f, InPathRefreshInterval);
    HearingRadiusInTiles = FMath::Max(1.0f, InHearingRadiusInTiles);
    InvestigationDuration = FMath::Max(0.1f, InInvestigationDuration);
    EnemyArchetype = InArchetype;
    if (EnemyArchetype == EPMEEnemyArchetype::Listener)
    {
        SightRadiusInTiles *= 0.5f; HearingRadiusInTiles *= 1.8f; PatrolMovementSpeed *= 0.9f;
    }
    else if (EnemyArchetype == EPMEEnemyArchetype::Stalker)
    {
        SightRadiusInTiles *= 1.15f; ChaseDistanceInTiles *= 1.35f; ChaseMovementSpeed *= 1.12f;
    }

    PatrolDirection = 1;
    RemainingPauseTime = 0.0f;
    RemainingSightCheckTime = 0.0f;
    RemainingPathRefreshTime = 0.0f;
    RemainingChaseDistanceWorld = 0.0f;
    BlockedChaseTime = 0.0f;
    ConsecutiveChasePathFailures = 0;
    BehaviourState = EPMEEnemyBehaviourState::Patrol;
    ChaseTarget.Reset();
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);
    LastKnownTargetCell = FIntPoint(-1, -1);
    ChaseStartLocation = FVector::ZeroVector;
    ReturnPathAnchorLocation = FVector::ZeroVector;
    ReturnDepartureAnchorLocation = FVector::ZeroVector;
    bHasPatrolReturnState = false;
    bHasReturnPathAnchor = false;
    bReturnAnchorReached = false;
    bFacingRight = true;
    SetActorRotation(FRotator::ZeroRotator);
    ApplyPixelBodyFacing();

    if (!PatrolPoints.IsEmpty())
    {
        SetActorLocation(PatrolPoints[0], false, nullptr, ETeleportType::TeleportPhysics);
        CurrentWaypointIndex = PatrolPoints.Num() > 1 ? 1 : 0;

        // Capture a valid initial return state immediately. This protects
        // against a stun or another state transition occurring before the
        // enemy has completed its first patrol tick.
        CapturePatrolReturnState();
    }

    ForceNetUpdate();
}

void APMEPatrolEnemy::SetPatrolEnabled(const bool bEnabled)
{
    bPatrolEnabled = bEnabled;
    RemainingPauseTime = 0.0f;

    if (!bPatrolEnabled)
    {
        BehaviourState = EPMEEnemyBehaviourState::Patrol;
        ChaseTarget.Reset();
        ActivePathPoints.Reset();
        ActivePathPointIndex = 0;
        RemainingChaseDistanceWorld = 0.0f;
    }
}

float APMEPatrolEnemy::GetRemainingChaseDistanceInTiles() const
{
    const float TileSize = GetTileSize();
    return TileSize > KINDA_SMALL_NUMBER
        ? RemainingChaseDistanceWorld / TileSize
        : 0.0f;
}

void APMEPatrolEnemy::TickPatrol(const float DeltaSeconds)
{
    RemainingSightCheckTime -= DeltaSeconds;

    // Perception remains active while crossing a critical route. A chase may
    // use every walkable cell; only the normal patrol is forbidden from
    // pausing or reversing on those cells.
    if (RemainingSightCheckTime <= 0.0f)
    {
        RemainingSightCheckTime = SightCheckInterval;
        CheckForVisiblePlayer();

        if (BehaviourState != EPMEEnemyBehaviourState::Patrol)
        {
            return;
        }
    }

    // Consume only the part of this frame spent waiting. If the endpoint pause
    // finishes mid-frame, use the remaining time for movement immediately.
    float MovementDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
    if (RemainingPauseTime > 0.0f)
    {
        const float ConsumedPauseTime = FMath::Min(RemainingPauseTime, MovementDeltaSeconds);
        RemainingPauseTime -= ConsumedPauseTime;
        MovementDeltaSeconds -= ConsumedPauseTime;

        if (MovementDeltaSeconds <= KINDA_SMALL_NUMBER)
        {
            return;
        }
    }

    const float EffectiveSpeed = FMath::Max(0.0f, PatrolMovementSpeed * AlertMultiplier);
    float RemainingMovementDistance = EffectiveSpeed * MovementDeltaSeconds;
    const int32 IterationLimit = FMath::Clamp(MaximumPatrolWaypointsPerTick, 1, 64);

    // A patrol path contains one point per grid cell. Consume intermediate
    // points in the same frame so reaching a tile centre never creates a
    // one-frame stop. Only a real route endpoint may introduce a pause.
    for (int32 Iteration = 0;
         Iteration < IterationLimit && RemainingMovementDistance > PMEEnemyPath::CardinalAxisTolerance;
         ++Iteration)
    {
        if (!PatrolPoints.IsValidIndex(CurrentWaypointIndex))
        {
            CurrentWaypointIndex = PatrolPoints.Num() > 1 ? 1 : 0;
        }

        if (!PatrolPoints.IsValidIndex(CurrentWaypointIndex))
        {
            return;
        }

        const int32 ReachedWaypointIndex = CurrentWaypointIndex;
        const FVector TargetLocation = PatrolPoints[ReachedWaypointIndex];

        // The previous movement step may already have landed exactly on this
        // point. Advance without sacrificing the current frame's movement.
        if (PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), TargetLocation))
        {
            const bool bReachedEndpoint = IsPatrolEndpoint(ReachedWaypointIndex);
            const bool bTransitOnly = IsPatrolPointTransitOnly(ReachedWaypointIndex);
            AdvanceWaypoint();

            const bool bShouldPause =
                !bTransitOnly &&
                WaypointPauseDuration > KINDA_SMALL_NUMBER &&
                (!bPauseOnlyAtPatrolEndpoints || bReachedEndpoint);

            if (bShouldPause)
            {
                RemainingPauseTime = WaypointPauseDuration;
                ForceNetUpdate();
                return;
            }

            continue;
        }

        const float StepDeltaSeconds = RemainingMovementDistance / FMath::Max(EffectiveSpeed, 1.0f);
        const float DistanceMoved = MoveTowardsLocation(TargetLocation, StepDeltaSeconds, EffectiveSpeed);
        RemainingMovementDistance = FMath::Max(0.0f, RemainingMovementDistance - DistanceMoved);

        if (PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), TargetLocation))
        {
            const bool bReachedEndpoint = IsPatrolEndpoint(ReachedWaypointIndex);
            const bool bTransitOnly = IsPatrolPointTransitOnly(ReachedWaypointIndex);
            AdvanceWaypoint();

            const bool bShouldPause =
                !bTransitOnly &&
                WaypointPauseDuration > KINDA_SMALL_NUMBER &&
                (!bPauseOnlyAtPatrolEndpoints || bReachedEndpoint);

            if (bShouldPause)
            {
                RemainingPauseTime = WaypointPauseDuration;
                ForceNetUpdate();
                return;
            }

            // Continue with the unused movement budget and the next grid cell.
            continue;
        }

        if (DistanceMoved <= KINDA_SMALL_NUMBER)
        {
            // Never skip a blocked waypoint because that could cut a corner.
            // Stop this frame and retry the same cardinal segment next tick.
            ForceNetUpdate();
        }

        break;
    }
}

void APMEPatrolEnemy::TickChase(const float DeltaSeconds)
{
    APMEPlayerCharacter* TargetCharacter = ChaseTarget.Get();
    if (RemainingChaseDistanceWorld <= KINDA_SMALL_NUMBER || !IsValidChaseTarget(TargetCharacter))
    {
        BeginReturnToPatrol();
        return;
    }

    RemainingSightCheckTime -= DeltaSeconds;
    if (RemainingSightCheckTime <= 0.0f)
    {
        RemainingSightCheckTime = SightCheckInterval;
        bChaseTargetVisible = HasLineOfSightTo(TargetCharacter);

        if (bChaseTargetVisible && MazeGenerator)
        {
            const FIntPoint VisibleCell = MazeGenerator->WorldToGrid(TargetCharacter->GetActorLocation());
            if (MazeGenerator->IsWalkable(VisibleCell.X, VisibleCell.Y))
            {
                LastKnownTargetCell = VisibleCell;
                LastKnownTargetLocation = GetGridCellCenter(VisibleCell);
                RemainingLostSightGraceTime = LostSightGraceDuration;
            }
            else
            {
                bChaseTargetVisible = false;
            }
        }
    }

    if (!bChaseTargetVisible)
    {
        RemainingLostSightGraceTime = FMath::Max(
            0.0f,
            RemainingLostSightGraceTime - DeltaSeconds);
    }

    if (!MazeGenerator ||
        !MazeGenerator->IsWalkable(LastKnownTargetCell.X, LastKnownTargetCell.Y))
    {
        BeginReturnToPatrol();
        return;
    }

    RemainingPathRefreshTime -= DeltaSeconds;

    const FIntPoint CurrentCell = FindBestWalkableCellForLocation(GetActorLocation());
    const bool bCurrentCellValid = MazeGenerator->IsWalkable(CurrentCell.X, CurrentCell.Y);
    const bool bNeedsInitialPath = LastPathGoalCell.X < 0 || LastPathGoalCell.Y < 0;
    const bool bTargetChangedCell = LastKnownTargetCell != LastPathGoalCell;
    const bool bPathExhaustedBeforeGoal =
        !ActivePathPoints.IsValidIndex(ActivePathPointIndex) &&
        bCurrentCellValid &&
        CurrentCell != LastKnownTargetCell;

    if (bNeedsInitialPath || bTargetChangedCell ||
        (RemainingPathRefreshTime <= 0.0f && bPathExhaustedBeforeGoal))
    {
        RemainingPathRefreshTime = ChasePathRefreshInterval;

        if (!RefreshPathToWorldLocation(LastKnownTargetLocation))
        {
            ++ConsecutiveChasePathFailures;
            if (ConsecutiveChasePathFailures >= FMath::Max(1, MaximumChasePathFailures))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("PMEPatrolEnemy '%s': chase path could not be built; returning to patrol."),
                    *GetName());
                BeginReturnToPatrol();
            }
            return;
        }

    }

    bool bReachedLastKnownCell = MoveAlongActivePath(
        DeltaSeconds,
        ChaseMovementSpeed * AlertMultiplier,
        true,
        nullptr);

    // Once the grid path reaches the target cell, the enemy may close the small
    // remaining distance to a currently visible player. The destination is
    // clamped inside that same walkable tile, so this never creates a shortcut
    // through an adjacent wall and movement remains cardinal.
    const FIntPoint CellAfterPath = FindBestWalkableCellForLocation(GetActorLocation());
    if (bReachedLastKnownCell &&
        bChaseTargetVisible &&
        CellAfterPath == LastKnownTargetCell)
    {
        const FVector SafeTargetPoint = GetSafePointInsideCell(
            TargetCharacter->GetActorLocation(),
            LastKnownTargetCell);

        const float DistanceMoved = MoveTowardsLocation(
            SafeTargetPoint,
            DeltaSeconds,
            ChaseMovementSpeed * AlertMultiplier);

        RemainingChaseDistanceWorld = FMath::Max(
            0.0f,
            RemainingChaseDistanceWorld - DistanceMoved);

        bReachedLastKnownCell = PMEEnemyPath::HasReachedCardinalPoint(
            GetActorLocation(),
            SafeTargetPoint);
    }

    if (bMovementBlockedThisTick || bPathSegmentInvalidThisTick)
    {
        BlockedChaseTime += DeltaSeconds;

        // Discard the stale segment. Keep the current goal marked as valid so
        // the normal refresh gate waits for ChaseBlockedRepathDelay instead of
        // rebuilding the same invalid path every frame.
        ActivePathPoints.Reset();
        ActivePathPointIndex = 0;
        LastPathGoalCell = LastKnownTargetCell;
        RemainingPathRefreshTime = FMath::Max(0.02f, ChaseBlockedRepathDelay);

        if (BlockedChaseTime >= FMath::Max(0.02f, ChaseBlockedRepathDelay))
        {
            BlockedChaseTime = 0.0f;
            ++ConsecutiveChasePathFailures;

            if (ConsecutiveChasePathFailures >= FMath::Max(1, MaximumChasePathFailures))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("PMEPatrolEnemy '%s': chase repeatedly blocked; returning to patrol."),
                    *GetName());
                BeginReturnToPatrol();
                return;
            }
        }
    }
    else if (bMadeMovementProgressThisTick)
    {
        BlockedChaseTime = 0.0f;
        ConsecutiveChasePathFailures = 0;
    }

    if (RemainingChaseDistanceWorld <= KINDA_SMALL_NUMBER ||
        (!bChaseTargetVisible &&
         RemainingLostSightGraceTime <= KINDA_SMALL_NUMBER &&
         bReachedLastKnownCell))
    {
        BeginReturnToPatrol();
    }
}

void APMEPatrolEnemy::TickReturn(const float DeltaSeconds)
{
    if (!HasValidPatrolReturnState())
    {
        if (!RecoverPatrolReturnState())
        {
            BehaviourState = EPMEEnemyBehaviourState::Patrol;
            ActivePathPoints.Reset();
            ActivePathPointIndex = 0;
            ForceNetUpdate();
            return;
        }

        ActivePathPoints.Reset();
        ActivePathPointIndex = 0;
        bReturnAnchorReached = false;
        BuildReturnPath();
    }

    if (!bReturnAnchorReached)
    {
        if (ActivePathPoints.IsEmpty() && !BuildReturnPath())
        {
            // Never fall back to direct movement. Retry on a later tick so a
            // temporary invalid state cannot make the enemy cross a wall.
            return;
        }

        if (MoveAlongActivePath(
            DeltaSeconds,
            ReturnMovementSpeed * AlertMultiplier,
            false,
            nullptr))
        {
            bReturnAnchorReached = true;
            ActivePathPoints.Reset();
            ActivePathPointIndex = 0;
        }

        return;
    }

    // The anchor is one endpoint of the exact patrol segment where the chase
    // started. This final movement therefore follows that original segment,
    // rather than drawing a line from an arbitrary maze cell.
    const float DistanceMoved = MoveTowardsLocation(
        ChaseStartLocation,
        DeltaSeconds,
        ReturnMovementSpeed * AlertMultiplier);

    if (PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), ChaseStartLocation))
    {
        FinishReturnToPatrol();
    }
    else if (DistanceMoved <= KINDA_SMALL_NUMBER)
    {
        // A custom blocking actor may obstruct the original patrol segment. Do
        // not teleport through it; remain in return state until the route clears.
        ForceNetUpdate();
    }
}


void APMEPatrolEnemy::HearNoise(const FVector& NoiseLocation, const float RadiusInTiles, AActor* NoiseSource, const FName NoiseTag)
{
    //if (!HasAuthority() || !bPatrolEnabled || !MazeGenerator || IsCrossingTransitOnlySection()) return;
    //if (BehaviourState == EPMEEnemyBehaviourState::Chase || BehaviourState == EPMEEnemyBehaviourState::ReturnToPatrol || BehaviourState == EPMEEnemyBehaviourState::Stunned) return;
    //const float EffectiveRadius = FMath::Min(RadiusInTiles, HearingRadiusInTiles) * GetTileSize();
    //if (FVector::DistSquared2D(GetActorLocation(), NoiseLocation) > FMath::Square(EffectiveRadius)) return;
    //BeginInvestigate(NoiseLocation);
}

void APMEPatrolEnemy::BeginInvestigate(const FVector& NoiseLocation)
{
    if (BehaviourState == EPMEEnemyBehaviourState::Patrol)
    {
        CapturePatrolReturnState();
    }

    BehaviourState = EPMEEnemyBehaviourState::Investigate;

    const FIntPoint NoiseCell = FindBestWalkableCellForLocation(NoiseLocation);
    InvestigationLocation = MazeGenerator && MazeGenerator->IsWalkable(NoiseCell.X, NoiseCell.Y)
        ? GetGridCellCenter(NoiseCell)
        : GetActorLocation();

    RemainingInvestigationTime = InvestigationDuration;
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);
    RefreshPathToWorldLocation(InvestigationLocation);
    ForceNetUpdate();
}

void APMEPatrolEnemy::TickInvestigate(const float DeltaSeconds)
{
    RemainingSightCheckTime -= DeltaSeconds;
    if (RemainingSightCheckTime <= 0.0f)
    {
        RemainingSightCheckTime = SightCheckInterval;
        CheckForVisiblePlayer();
        if (BehaviourState == EPMEEnemyBehaviourState::Chase)
        {
            return;
        }
    }

    if (ActivePathPoints.IsEmpty())
    {
        RefreshPathToWorldLocation(InvestigationLocation);
    }

    const bool bArrived = MoveAlongActivePath(
        DeltaSeconds,
        PatrolMovementSpeed * 1.15f * AlertMultiplier,
        false,
        nullptr);

    if (bMovementBlockedThisTick || bPathSegmentInvalidThisTick)
    {
        ActivePathPoints.Reset();
        ActivePathPointIndex = 0;
        LastPathGoalCell = FIntPoint(-1, -1);
        return;
    }

    if (bArrived)
    {
        RemainingInvestigationTime = FMath::Max(
            0.0f,
            RemainingInvestigationTime - DeltaSeconds);

        if (RemainingInvestigationTime <= 0.0f)
        {
            BeginReturnToPatrol();
        }
    }
}

void APMEPatrolEnemy::ApplyStun(const float Duration)
{
    if (!HasAuthority()) return;
    if (BehaviourState == EPMEEnemyBehaviourState::Patrol)
    {
        CapturePatrolReturnState();
    }
    BehaviourState = EPMEEnemyBehaviourState::Stunned; RemainingStunTime = FMath::Max(RemainingStunTime, Duration);
    ChaseTarget.Reset(); ActivePathPoints.Reset(); ForceNetUpdate();
}

void APMEPatrolEnemy::SetAlertMultiplier(const float NewMultiplier)
{
    AlertMultiplier = FMath::Clamp(NewMultiplier, 1.0f, 3.0f);
}

void APMEPatrolEnemy::CheckForVisiblePlayer()
{
    if (APMEPlayerCharacter* VisiblePlayer = FindClosestVisiblePlayer())
    {
        BeginChase(VisiblePlayer);
    }
}

APMEPlayerCharacter* APMEPatrolEnemy::FindClosestVisiblePlayer() const
{
    if (!GetWorld() || !MazeGenerator)
    {
        return nullptr;
    }

    const FIntPoint EnemyCell = FindBestWalkableCellForLocation(GetActorLocation());
    if (!MazeGenerator->IsWalkable(EnemyCell.X, EnemyCell.Y))
    {
        return nullptr;
    }

    APMEPlayerCharacter* ClosestPlayer = nullptr;
    float ClosestTileDistance = MAX_flt;

    for (TActorIterator<APMEPlayerCharacter> It(GetWorld()); It; ++It)
    {
        APMEPlayerCharacter* PlayerCharacter = *It;
        if (!IsValidChaseTarget(PlayerCharacter))
        {
            continue;
        }

        const FIntPoint PlayerCell = MazeGenerator->WorldToGrid(PlayerCharacter->GetActorLocation());
        if (!MazeGenerator->IsWalkable(PlayerCell.X, PlayerCell.Y))
        {
            continue;
        }

        // Enemy vision is intentionally grid-cardinal. Being geometrically
        // close is not enough when the target is on a different row/column.
        if (EnemyCell.X != PlayerCell.X && EnemyCell.Y != PlayerCell.Y)
        {
            continue;
        }

        const float TileDistance = static_cast<float>(
            FMath::Abs(PlayerCell.X - EnemyCell.X) +
            FMath::Abs(PlayerCell.Y - EnemyCell.Y));

        const APMEPlayerState* TargetPS = PlayerCharacter->GetPlayerState<APMEPlayerState>();
        const UAbilitySystemComponent* TargetASC = TargetPS ? TargetPS->GetAbilitySystemComponent() : nullptr;
        const float DetectionMultiplier = TargetASC
            ? TargetASC->GetNumericAttribute(UPMEAttributeSet::GetDetectionMultiplierAttribute())
            : 1.0f;
        const float EffectiveSightRadius = SightRadiusInTiles * FMath::Max(0.1f, DetectionMultiplier);

        if (TileDistance > EffectiveSightRadius || TileDistance >= ClosestTileDistance)
        {
            continue;
        }

        if (HasLineOfSightTo(PlayerCharacter))
        {
            ClosestPlayer = PlayerCharacter;
            ClosestTileDistance = TileDistance;
        }
    }

    return ClosestPlayer;
}

bool APMEPatrolEnemy::HasLineOfSightTo(const APMEPlayerCharacter* PlayerCharacter) const
{
    if (!GetWorld() || !MazeGenerator || !PlayerCharacter)
    {
        return false;
    }

    const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 15.0f);
    const FVector TraceEnd = PlayerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 15.0f);

    // The logical grid is the complete perception rule: same row/column and
    // every intermediate cell walkable. Dynamic actors and interactive objects
    // do not incorrectly behave like vision-blocking walls.
    const bool bHasLineOfSight = MazeGenerator->HasClearGridLineOfSight(
        TraceStart,
        TraceEnd);

#if ENABLE_DRAW_DEBUG
    if (bDrawSightDebug)
    {
        DrawDebugLine(
            GetWorld(),
            TraceStart,
            TraceEnd,
            bHasLineOfSight ? FColor::Green : FColor::Red,
            false,
            SightCheckInterval,
            0,
            2.0f);

        if (!bHasLineOfSight)
        {
            DrawDebugPoint(
                GetWorld(),
                (TraceStart + TraceEnd) * 0.5f,
                10.0f,
                FColor::Red,
                false,
                SightCheckInterval);
        }
    }
#endif

    return bHasLineOfSight;
}

void APMEPatrolEnemy::BeginChase(APMEPlayerCharacter* PlayerCharacter)
{
    if (!PlayerCharacter ||
        (BehaviourState != EPMEEnemyBehaviourState::Patrol &&
         BehaviourState != EPMEEnemyBehaviourState::Investigate) ||
        !MazeGenerator)
    {
        return;
    }

    const FIntPoint VisibleCell = MazeGenerator->WorldToGrid(PlayerCharacter->GetActorLocation());
    if (!MazeGenerator->IsWalkable(VisibleCell.X, VisibleCell.Y))
    {
        return;
    }

    const bool bStartedFromPatrol = BehaviourState == EPMEEnemyBehaviourState::Patrol;
    BehaviourState = EPMEEnemyBehaviourState::Chase;
    ChaseTarget = PlayerCharacter;
    LastKnownTargetCell = VisibleCell;
    LastKnownTargetLocation = GetGridCellCenter(VisibleCell);
    bChaseTargetVisible = true;
    RemainingLostSightGraceTime = LostSightGraceDuration;
    RemainingSightCheckTime = SightCheckInterval;
    BlockedChaseTime = 0.0f;
    ConsecutiveChasePathFailures = 0;

    if (APMEPlayerState* DetectedPS = PlayerCharacter->GetPlayerState<APMEPlayerState>())
    {
        DetectedPS->RecordDetection();
    }

    if (bStartedFromPatrol)
    {
        CapturePatrolReturnState();
    }

    RemainingChaseDistanceWorld = ChaseDistanceInTiles * GetTileSize();
    RemainingPathRefreshTime = 0.0f;
    RemainingPauseTime = 0.0f;
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);
    ForceNetUpdate();
}

void APMEPatrolEnemy::CapturePatrolReturnState()
{
    bHasPatrolReturnState = false;
    bHasReturnPathAnchor = false;
    bReturnAnchorReached = false;

    if (PatrolPoints.Num() < 2)
    {
        return;
    }

    // If perception interrupts a short critical-route crossing, return to a
    // real non-transit patrol waypoint instead of storing a point on the
    // player's mandatory corridor or on top of an interactive object.
    if (IsCrossingTransitOnlySection())
    {
        const int32 SafeWaypointIndex = FindSafeNonTransitReturnWaypointIndex();
        if (PatrolPoints.IsValidIndex(SafeWaypointIndex))
        {
            ChaseStartLocation = PatrolPoints[SafeWaypointIndex];
            ReturnPathAnchorLocation = ChaseStartLocation;
            SavedPatrolWaypointIndex = SafeWaypointIndex;
            SavedPatrolDirection = PatrolDirection == 0 ? 1 : PatrolDirection;
            bHasPatrolReturnState = true;
            bHasReturnPathAnchor = true;
            return;
        }
    }

    const int32 TargetIndex = FMath::Clamp(CurrentWaypointIndex, 0, PatrolPoints.Num() - 1);
    int32 PreviousIndex = TargetIndex;

    if (bPingPongPatrol)
    {
        const int32 CandidateIndex = TargetIndex - (PatrolDirection == 0 ? 1 : PatrolDirection);
        if (PatrolPoints.IsValidIndex(CandidateIndex))
        {
            PreviousIndex = CandidateIndex;
        }
    }
    else
    {
        PreviousIndex = (TargetIndex - 1 + PatrolPoints.Num()) % PatrolPoints.Num();
    }

    if (!PatrolPoints.IsValidIndex(PreviousIndex) || PreviousIndex == TargetIndex)
    {
        return;
    }

    const FVector& SegmentStart = PatrolPoints[PreviousIndex];
    const FVector& SegmentEnd = PatrolPoints[TargetIndex];
    const FVector ActorLocation = GetActorLocation();

    // The actor can be a few centimetres outside the mathematical segment due
    // to swept movement. Store the closest point on the actual patrol segment,
    // never an arbitrary world location.
    FVector ProjectedReturnLocation = FMath::ClosestPointOnSegment(
        ActorLocation,
        SegmentStart,
        SegmentEnd);
    ProjectedReturnLocation.Z = ActorLocation.Z;

    const bool bFiniteReturnLocation =
        FMath::IsFinite(ProjectedReturnLocation.X) &&
        FMath::IsFinite(ProjectedReturnLocation.Y) &&
        FMath::IsFinite(ProjectedReturnLocation.Z);

    if (!bFiniteReturnLocation)
    {
        return;
    }

    ChaseStartLocation = ProjectedReturnLocation;
    SavedPatrolWaypointIndex = TargetIndex;
    SavedPatrolDirection = PatrolDirection == 0 ? 1 : PatrolDirection;

    // Return through the closest endpoint, then follow the original segment to
    // the exact interruption point. Both parts are guaranteed to be walkable.
    ReturnPathAnchorLocation =
        FVector::DistSquared2D(ChaseStartLocation, SegmentStart) <=
        FVector::DistSquared2D(ChaseStartLocation, SegmentEnd)
            ? SegmentStart
            : SegmentEnd;

    bHasPatrolReturnState = true;
    bHasReturnPathAnchor = true;
}

int32 APMEPatrolEnemy::FindSafeNonTransitReturnWaypointIndex() const
{
    if (PatrolPoints.IsEmpty())
    {
        return INDEX_NONE;
    }

    const int32 Direction = PatrolDirection == 0 ? 1 : PatrolDirection;
    const int32 TargetIndex = FMath::Clamp(CurrentWaypointIndex, 0, PatrolPoints.Num() - 1);

    // Prefer completing the current crossing in the existing travel direction.
    for (int32 Index = TargetIndex; PatrolPoints.IsValidIndex(Index); Index += Direction)
    {
        if (!IsPatrolPointTransitOnly(Index))
        {
            return Index;
        }
    }

    // At a route boundary, use the closest ordinary waypoint behind us.
    for (int32 Index = TargetIndex - Direction; PatrolPoints.IsValidIndex(Index); Index -= Direction)
    {
        if (!IsPatrolPointTransitOnly(Index))
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

bool APMEPatrolEnemy::HasValidPatrolReturnState() const
{
    if (!bHasPatrolReturnState ||
        !bHasReturnPathAnchor ||
        PatrolPoints.Num() < 2 ||
        !PatrolPoints.IsValidIndex(SavedPatrolWaypointIndex))
    {
        return false;
    }

    const bool bFiniteChaseStart =
        FMath::IsFinite(ChaseStartLocation.X) &&
        FMath::IsFinite(ChaseStartLocation.Y) &&
        FMath::IsFinite(ChaseStartLocation.Z);

    const bool bFiniteAnchor =
        FMath::IsFinite(ReturnPathAnchorLocation.X) &&
        FMath::IsFinite(ReturnPathAnchorLocation.Y) &&
        FMath::IsFinite(ReturnPathAnchorLocation.Z);

    return bFiniteChaseStart && bFiniteAnchor;
}

bool APMEPatrolEnemy::RecoverPatrolReturnState()
{
    bHasPatrolReturnState = false;
    bHasReturnPathAnchor = false;
    bReturnAnchorReached = false;

    if (PatrolPoints.IsEmpty())
    {
        return false;
    }

    const FVector ActorLocation = GetActorLocation();
    int32 ClosestPointIndex = INDEX_NONE;
    float ClosestDistanceSquared = MAX_flt;

    for (int32 PointIndex = 0; PointIndex < PatrolPoints.Num(); ++PointIndex)
    {
        const float DistanceSquared = FVector::DistSquared2D(
            ActorLocation,
            PatrolPoints[PointIndex]);

        if (DistanceSquared < ClosestDistanceSquared)
        {
            ClosestDistanceSquared = DistanceSquared;
            ClosestPointIndex = PointIndex;
        }
    }

    if (!PatrolPoints.IsValidIndex(ClosestPointIndex))
    {
        return false;
    }

    // Recovery deliberately targets a real patrol waypoint. It must never use
    // FVector::ZeroVector as a fallback because world origin can be a valid but
    // unrelated location in a centred maze.
    ChaseStartLocation = PatrolPoints[ClosestPointIndex];
    ReturnPathAnchorLocation = ChaseStartLocation;
    SavedPatrolWaypointIndex = ClosestPointIndex;
    SavedPatrolDirection = PatrolDirection == 0 ? 1 : PatrolDirection;
    bHasPatrolReturnState = true;
    bHasReturnPathAnchor = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("PMEPatrolEnemy '%s': rebuilt missing patrol return state using waypoint %d at %s."),
        *GetName(),
        ClosestPointIndex,
        *ChaseStartLocation.ToCompactString());

    return true;
}

void APMEPatrolEnemy::BeginReturnToPatrol()
{
    if (BehaviourState == EPMEEnemyBehaviourState::ReturnToPatrol)
    {
        return;
    }

    if (!HasValidPatrolReturnState() && !RecoverPatrolReturnState())
    {
        // There is no safe destination. Resume patrol state without moving to
        // an uninitialised world-space location.
        BehaviourState = EPMEEnemyBehaviourState::Patrol;
        ChaseTarget.Reset();
        ActivePathPoints.Reset();
        ActivePathPointIndex = 0;
        RemainingChaseDistanceWorld = 0.0f;
        ForceNetUpdate();
        return;
    }

    // Return pathfinding always starts from the current walkable grid cell.
    // Reusing the next chase waypoint here can preserve a stale point located
    // beyond a wall after a target-path refresh.
    const FIntPoint DepartureCell = FindBestWalkableCellForLocation(GetActorLocation());
    ReturnDepartureAnchorLocation = MazeGenerator && MazeGenerator->IsWalkable(DepartureCell.X, DepartureCell.Y)
        ? GetGridCellCenter(DepartureCell)
        : GetActorLocation();

    BehaviourState = EPMEEnemyBehaviourState::ReturnToPatrol;
    ChaseTarget.Reset();
    bChaseTargetVisible = false;
    LastKnownTargetCell = FIntPoint(-1, -1);
    RemainingLostSightGraceTime = 0.0f;
    BlockedChaseTime = 0.0f;
    ConsecutiveChasePathFailures = 0;
    RemainingChaseDistanceWorld = 0.0f;
    RemainingPathRefreshTime = 0.0f;
    RemainingPauseTime = 0.0f;
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);
    bReturnAnchorReached = false;

    BuildReturnPath();
    ForceNetUpdate();
}

bool APMEPatrolEnemy::BuildReturnPath()
{
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);

    if (!MazeGenerator || !HasValidPatrolReturnState())
    {
        return false;
    }

    const FIntPoint StartCell = MazeGenerator->WorldToGrid(ReturnDepartureAnchorLocation);
    const FIntPoint GoalCell = MazeGenerator->WorldToGrid(ReturnPathAnchorLocation);

    if (!MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
        !MazeGenerator->IsWalkable(GoalCell.X, GoalCell.Y))
    {
        return false;
    }

    TArray<FVector> NewPathPoints;
    if (!BuildGridPath(StartCell, GoalCell, NewPathPoints))
    {
        return false;
    }

    // Reach the next point from the interrupted chase/investigation path first.
    // That segment was already known to be valid when the enemy was moving on it.
    if (FVector::Dist2D(GetActorLocation(), ReturnDepartureAnchorLocation) > WaypointAcceptanceRadius)
    {
        NewPathPoints.Insert(ReturnDepartureAnchorLocation, 0);
    }

    ActivePathPoints = MoveTemp(NewPathPoints);
    ActivePathPointIndex = 0;
    LastPathGoalCell = GoalCell;
    return true;
}

void APMEPatrolEnemy::FinishReturnToPatrol()
{
    // MoveTowardsLocation reaches the interruption point one cardinal axis at
    // a time. No final diagonal snap or teleport is required here.

    CurrentWaypointIndex = FMath::Clamp(SavedPatrolWaypointIndex, 0, FMath::Max(0, PatrolPoints.Num() - 1));
    PatrolDirection = SavedPatrolDirection == 0 ? 1 : SavedPatrolDirection;
    BehaviourState = EPMEEnemyBehaviourState::Patrol;
    RemainingPauseTime = 0.0f;
    RemainingSightCheckTime = SightCheckInterval;
    ActivePathPoints.Reset();
    ActivePathPointIndex = 0;
    LastPathGoalCell = FIntPoint(-1, -1);
    bReturnAnchorReached = false;
    bHasPatrolReturnState = false;
    bHasReturnPathAnchor = false;
    ForceNetUpdate();
}

bool APMEPatrolEnemy::RefreshPathToWorldLocation(const FVector& TargetWorldLocation)
{
    if (!MazeGenerator)
    {
        return false;
    }

    const FIntPoint StartCell = FindBestWalkableCellForLocation(GetActorLocation());
    const FIntPoint GoalCell = FindBestWalkableCellForLocation(TargetWorldLocation);

    if (!MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
        !MazeGenerator->IsWalkable(GoalCell.X, GoalCell.Y))
    {
        return false;
    }

    TArray<FVector> NewPathPoints;
    if (!BuildGridPath(StartCell, GoalCell, NewPathPoints))
    {
        return false;
    }

    // First align to the centre of the cell that physically contains the enemy.
    // This is always inside a walkable tile and prevents a stale waypoint from
    // pulling the actor through an inside corner.
    const FVector StartCellWorld = GetGridCellCenter(StartCell);
    if (FVector::Dist2D(GetActorLocation(), StartCellWorld) > WaypointAcceptanceRadius)
    {
        NewPathPoints.Insert(StartCellWorld, 0);
    }

    ActivePathPoints = MoveTemp(NewPathPoints);
    ActivePathPointIndex = 0;
    LastPathGoalCell = GoalCell;
    return true;
}

bool APMEPatrolEnemy::BuildGridPath(
    const FIntPoint StartCell,
    const FIntPoint GoalCell,
    TArray<FVector>& OutWorldPath) const
{
    OutWorldPath.Reset();

    if (!MazeGenerator ||
        !MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
        !MazeGenerator->IsWalkable(GoalCell.X, GoalCell.Y))
    {
        return false;
    }

    if (StartCell == GoalCell)
    {
        return true;
    }

    const int32 Width = MazeGenerator->GetGridWidth();
    const int32 Height = MazeGenerator->GetGridHeight();
    const int32 CellCount = Width * Height;

    auto ToIndex = [Width](const FIntPoint Cell)
    {
        return Cell.Y * Width + Cell.X;
    };

    auto ToCell = [Width](const int32 Index)
    {
        return FIntPoint(Index % Width, Index / Width);
    };

    TArray<int32> CameFrom;
    CameFrom.Init(INDEX_NONE, CellCount);

    TQueue<FIntPoint> OpenCells;
    OpenCells.Enqueue(StartCell);
    CameFrom[ToIndex(StartCell)] = ToIndex(StartCell);

    bool bFoundGoal = false;
    FIntPoint Current;
    while (OpenCells.Dequeue(Current))
    {
        if (Current == GoalCell)
        {
            bFoundGoal = true;
            break;
        }

        for (const FIntPoint& Direction : PMEEnemyPath::Directions)
        {
            const FIntPoint Next = Current + Direction;
            if (!MazeGenerator->IsWalkable(Next.X, Next.Y))
            {
                continue;
            }

            const int32 NextIndex = ToIndex(Next);
            if (!CameFrom.IsValidIndex(NextIndex) || CameFrom[NextIndex] != INDEX_NONE)
            {
                continue;
            }

            CameFrom[NextIndex] = ToIndex(Current);
            OpenCells.Enqueue(Next);
        }
    }

    if (!bFoundGoal)
    {
        return false;
    }

    TArray<FIntPoint> ReverseCells;
    int32 CurrentIndex = ToIndex(GoalCell);
    const int32 StartIndex = ToIndex(StartCell);

    while (CurrentIndex != StartIndex)
    {
        if (!CameFrom.IsValidIndex(CurrentIndex) || CameFrom[CurrentIndex] == INDEX_NONE)
        {
            OutWorldPath.Reset();
            return false;
        }

        ReverseCells.Add(ToCell(CurrentIndex));
        CurrentIndex = CameFrom[CurrentIndex];
    }

    OutWorldPath.Reserve(ReverseCells.Num());
    for (int32 Index = ReverseCells.Num() - 1; Index >= 0; --Index)
    {
        const FIntPoint Cell = ReverseCells[Index];
        OutWorldPath.Add(
            MazeGenerator->GridToWorld(Cell.X, Cell.Y) + FVector(0.0f, 0.0f, 50.0f));
    }

    return true;
}

bool APMEPatrolEnemy::MoveAlongActivePath(
    const float DeltaSeconds,
    const float Speed,
    const bool bConsumeChaseBudget,
    const FVector* FinalExactLocation)
{
    FVector TargetLocation = GetActorLocation();
    bool bHasTarget = false;

    while (ActivePathPoints.IsValidIndex(ActivePathPointIndex))
    {
        TargetLocation = ActivePathPoints[ActivePathPointIndex];
        if (PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), TargetLocation))
        {
            ++ActivePathPointIndex;
            continue;
        }

        bHasTarget = true;
        break;
    }

    if (!bHasTarget && FinalExactLocation)
    {
        TargetLocation = *FinalExactLocation;
        bHasTarget = !PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), TargetLocation);
    }

    if (!bHasTarget)
    {
        return true;
    }

    const float DistanceMoved = MoveTowardsLocation(TargetLocation, DeltaSeconds, Speed);
    if (bConsumeChaseBudget)
    {
        RemainingChaseDistanceWorld = FMath::Max(0.0f, RemainingChaseDistanceWorld - DistanceMoved);
    }

    if (PMEEnemyPath::HasReachedCardinalPoint(GetActorLocation(), TargetLocation))
    {
        if (ActivePathPoints.IsValidIndex(ActivePathPointIndex))
        {
            ++ActivePathPointIndex;
        }
        else if (FinalExactLocation)
        {
            return true;
        }
    }

    return false;
}

float APMEPatrolEnemy::MoveTowardsLocation(
    const FVector& TargetLocation,
    const float DeltaSeconds,
    const float Speed)
{
    const FVector StartLocation = GetActorLocation();
    const float DeltaX = TargetLocation.X - StartLocation.X;
    const float DeltaY = TargetLocation.Y - StartLocation.Y;
    const float AbsDeltaX = FMath::Abs(DeltaX);
    const float AbsDeltaY = FMath::Abs(DeltaY);

    if (AbsDeltaX <= PMEEnemyPath::CardinalAxisTolerance &&
        AbsDeltaY <= PMEEnemyPath::CardinalAxisTolerance)
    {
        LastCardinalMoveDirection = FIntPoint::ZeroValue;
        return 0.0f;
    }

    bool bMoveOnX = false;

    if (MazeGenerator)
    {
        const FIntPoint StartCell = FindBestWalkableCellForLocation(StartLocation);
        const FIntPoint TargetCell = FindBestWalkableCellForLocation(TargetLocation);

        if (!MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
            !MazeGenerator->IsWalkable(TargetCell.X, TargetCell.Y) ||
            !IsCardinalWalkableSegment(StartCell, TargetCell))
        {
            // Do not choose a dominant axis as a fallback. That was the source
            // of enemies repeatedly walking into a wall when a stale diagonal
            // waypoint survived a chase refresh.
            bPathSegmentInvalidThisTick = true;
            LastCardinalMoveDirection = FIntPoint::ZeroValue;
            return 0.0f;
        }

        const int32 CellDeltaX = TargetCell.X - StartCell.X;
        const int32 CellDeltaY = TargetCell.Y - StartCell.Y;

        if (CellDeltaX != 0)
        {
            // Align to the row centre before moving horizontally.
            bMoveOnX = AbsDeltaY <= PMEEnemyPath::CardinalAxisTolerance;
        }
        else if (CellDeltaY != 0)
        {
            // Align to the column centre before moving vertically.
            bMoveOnX = AbsDeltaX > PMEEnemyPath::CardinalAxisTolerance;
        }
        else
        {
            // Inside one tile, close the distance in an L shape.
            bMoveOnX = AbsDeltaX > PMEEnemyPath::CardinalAxisTolerance;
        }
    }
    else
    {
        // Without the maze grid there is no safe chase movement.
        bPathSegmentInvalidThisTick = true;
        return 0.0f;
    }

    FVector CardinalDirection = FVector::ZeroVector;
    float RemainingAxisDistance = 0.0f;
    FIntPoint NewCardinalDirection = FIntPoint::ZeroValue;

    if (bMoveOnX && AbsDeltaX > PMEEnemyPath::CardinalAxisTolerance)
    {
        const float Sign = FMath::Sign(DeltaX);
        CardinalDirection.X = Sign;
        RemainingAxisDistance = AbsDeltaX;
        NewCardinalDirection.X = Sign > 0.0f ? 1 : -1;
    }
    else if (AbsDeltaY > PMEEnemyPath::CardinalAxisTolerance)
    {
        const float Sign = FMath::Sign(DeltaY);
        CardinalDirection.Y = Sign;
        RemainingAxisDistance = AbsDeltaY;
        NewCardinalDirection.Y = Sign > 0.0f ? 1 : -1;
    }
    else if (AbsDeltaX > PMEEnemyPath::CardinalAxisTolerance)
    {
        const float Sign = FMath::Sign(DeltaX);
        CardinalDirection.X = Sign;
        RemainingAxisDistance = AbsDeltaX;
        NewCardinalDirection.X = Sign > 0.0f ? 1 : -1;
    }
    else
    {
        LastCardinalMoveDirection = FIntPoint::ZeroValue;
        return 0.0f;
    }

    if (NewCardinalDirection != LastCardinalMoveDirection)
    {
        ForceNetUpdate();
        LastCardinalMoveDirection = NewCardinalDirection;
    }

    const float RequestedMoveDistance = FMath::Min(
        FMath::Max(0.0f, Speed) * DeltaSeconds,
        RemainingAxisDistance);

    FHitResult HitResult;
    AddActorWorldOffset(
        CardinalDirection * RequestedMoveDistance,
        true,
        &HitResult,
        ETeleportType::None);

    // Never rotate the actor or the pixel body with movement. The top-down
    // artwork remains upright; only left/right movement changes its mirror.
    SetActorRotation(FRotator::ZeroRotator);
    UpdatePixelBodyFacing(NewCardinalDirection);

    const FVector EndLocation = GetActorLocation();
    const FVector ActualDelta = EndLocation - StartLocation;
    const bool bActuallyMovedOnX = !FMath::IsNearlyZero(CardinalDirection.X);

    if (bActuallyMovedOnX && FMath::Abs(ActualDelta.Y) > PMEEnemyPath::CardinalAxisTolerance)
    {
        FVector CorrectedLocation = EndLocation;
        CorrectedLocation.Y = StartLocation.Y;
        SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::None);
    }
    else if (!bActuallyMovedOnX && FMath::Abs(ActualDelta.X) > PMEEnemyPath::CardinalAxisTolerance)
    {
        FVector CorrectedLocation = EndLocation;
        CorrectedLocation.X = StartLocation.X;
        SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::None);
    }

    const float DistanceMoved = FVector::Dist2D(StartLocation, GetActorLocation());
    if (DistanceMoved > PMEEnemyPath::CardinalAxisTolerance)
    {
        bMadeMovementProgressThisTick = true;
    }

    const bool bBlockedBeforeTarget =
        HitResult.bBlockingHit &&
        DistanceMoved + PMEEnemyPath::CardinalAxisTolerance < RequestedMoveDistance;

    if (bBlockedBeforeTarget)
    {
        bMovementBlockedThisTick = true;
    }

    if (RemainingAxisDistance - DistanceMoved <= PMEEnemyPath::CardinalAxisTolerance)
    {
        ForceNetUpdate();
    }

    return DistanceMoved;
}

void APMEPatrolEnemy::OnRep_FacingRight()
{
    ApplyPixelBodyFacing();
}

void APMEPatrolEnemy::ApplyPixelBodyFacing()
{
    if (!PixelBody)
    {
        return;
    }

    // The project camera is top-down with world +Y pointing to screen-right.
    // Absolute rotation/scale prevents the root actor from rotating the artwork.
    PixelBody->SetAbsolute(false, true, true);
    PixelBody->SetWorldRotation(PixelBodyUprightRotation);

    FVector VisualScale = PixelBody->GetComponentScale().GetAbs();
    if (VisualScale.IsNearlyZero())
    {
        VisualScale = FVector(0.46f, 0.46f, 0.68f);
    }

    if (bMirrorPixelBodyWhenMovingLeft && !bFacingRight)
    {
        VisualScale.Y *= -1.0f;
    }

    PixelBody->SetWorldScale3D(VisualScale);
}

void APMEPatrolEnemy::UpdatePixelBodyFacing(const FIntPoint& CardinalDirection)
{
    // Vertical movement must not rotate or flip the artwork. Preserve the
    // most recent horizontal facing until the enemy next moves left/right.
    if (CardinalDirection.Y == 0)
    {
        ApplyPixelBodyFacing();
        return;
    }

    const bool bNewFacingRight = CardinalDirection.Y > 0;
    if (bFacingRight != bNewFacingRight)
    {
        bFacingRight = bNewFacingRight;
        ForceNetUpdate();
    }

    ApplyPixelBodyFacing();
}

void APMEPatrolEnemy::OnPatrolCollisionBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    const int32 OtherBodyIndex,
    const bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || !bPatrolEnabled)
    {
        return;
    }

    APMEPlayerCharacter* PlayerCharacter = Cast<APMEPlayerCharacter>(OtherActor);
    if (!PlayerCharacter)
    {
        return;
    }

    if (APMEGameModeBase* MazeGameMode = GetWorld()->GetAuthGameMode<APMEGameModeBase>())
    {
        MazeGameMode->HandlePlayerCaughtByEnemy(PlayerCharacter);

        if (BehaviourState == EPMEEnemyBehaviourState::Chase)
        {
            BeginReturnToPatrol();
        }
    }
}

void APMEPatrolEnemy::ResolveAudioAssets()
{
    if (!EnemyStepSound)
    {
        EnemyStepSound = LoadObject<USoundBase>(nullptr, PixelMazeEnemyAudio::StepSoundPath);
    }

    if (!EnemyVocalSound)
    {
        EnemyVocalSound = LoadObject<USoundBase>(nullptr, PixelMazeEnemyAudio::VocalSoundPath);
    }
}

void APMEPatrolEnemy::TickLocalAudio(const float DeltaSeconds)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!bAudioTrackingInitialized)
    {
        ResetLocalAudioTracking();
    }

    if (!IsRoundActive())
    {
        LastAudioLocation = GetActorLocation();
        AccumulatedAudioDistance = 0.0f;
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    const float DistanceMoved = FVector::Dist2D(CurrentLocation, LastAudioLocation);
    LastAudioLocation = CurrentLocation;

    const float StepDistance = FMath::Max(10.0f, EnemyStepDistanceInTiles * GetTileSize());
    if (DistanceMoved > StepDistance * 3.0f)
    {
        // Initial placement and correction teleports must not generate footsteps.
        AccumulatedAudioDistance = 0.0f;
    }
    else if (DistanceMoved > KINDA_SMALL_NUMBER)
    {
        AccumulatedAudioDistance += DistanceMoved;
        if (EnemyStepSound && AccumulatedAudioDistance >= StepDistance)
        {
            AccumulatedAudioDistance = FMath::Fmod(AccumulatedAudioDistance, StepDistance);
            UGameplayStatics::PlaySoundAtLocation(
                this,
                EnemyStepSound,
                CurrentLocation,
                EnemyStepVolume,
                FMath::FRandRange(0.94f, 1.06f));
        }
    }

    RemainingVocalDelay -= DeltaSeconds;
    if (RemainingVocalDelay <= 0.0f)
    {
        if (EnemyVocalSound)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                EnemyVocalSound,
                CurrentLocation,
                EnemyVocalVolume,
                FMath::FRandRange(0.96f, 1.04f));
        }

        const float MinimumInterval = FMath::Min(EnemyVocalMinimumInterval, EnemyVocalMaximumInterval);
        const float MaximumInterval = FMath::Max(EnemyVocalMinimumInterval, EnemyVocalMaximumInterval);
        RemainingVocalDelay = FMath::FRandRange(
            FMath::Max(1.0f, MinimumInterval),
            FMath::Max(1.0f, MaximumInterval));
    }
}

void APMEPatrolEnemy::ResetLocalAudioTracking()
{
    bAudioTrackingInitialized = true;
    LastAudioLocation = GetActorLocation();
    AccumulatedAudioDistance = 0.0f;

    const float MinimumInterval = FMath::Min(EnemyVocalMinimumInterval, EnemyVocalMaximumInterval);
    const float MaximumInterval = FMath::Max(EnemyVocalMinimumInterval, EnemyVocalMaximumInterval);
    RemainingVocalDelay = FMath::FRandRange(
        FMath::Max(1.0f, MinimumInterval),
        FMath::Max(1.0f, MaximumInterval));
}


bool APMEPatrolEnemy::IsPatrolPointTransitOnly(const int32 PointIndex) const
{
    return PatrolTransitOnlyPointFlags.IsValidIndex(PointIndex) &&
        PatrolTransitOnlyPointFlags[PointIndex] != 0;
}

bool APMEPatrolEnemy::IsCrossingTransitOnlySection() const
{
    if (!MazeGenerator || PatrolPoints.IsEmpty())
    {
        return false;
    }

    if (TransitOnlyCells.Contains(MazeGenerator->WorldToGrid(GetActorLocation())))
    {
        return true;
    }

    // CurrentWaypointIndex is the target. The previous point is derived from
    // the current ping-pong direction. Treat both sides of a transit segment as
    // crossing so perception cannot interrupt the enemy halfway through it.
    if (IsPatrolPointTransitOnly(CurrentWaypointIndex))
    {
        return true;
    }

    const int32 PreviousWaypointIndex = CurrentWaypointIndex - PatrolDirection;
    return IsPatrolPointTransitOnly(PreviousWaypointIndex);
}

FIntPoint APMEPatrolEnemy::FindBestWalkableCellForLocation(const FVector& WorldLocation) const
{
    if (!MazeGenerator)
    {
        return FIntPoint(-1, -1);
    }

    const FIntPoint RoundedCell = MazeGenerator->WorldToGrid(WorldLocation);
    if (MazeGenerator->IsWalkable(RoundedCell.X, RoundedCell.Y))
    {
        return RoundedCell;
    }

    FIntPoint BestCell(-1, -1);
    float BestDistanceSquared = MAX_flt;

    for (int32 Radius = 1; Radius <= 2; ++Radius)
    {
        for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
        {
            for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
            {
                if (FMath::Abs(OffsetX) + FMath::Abs(OffsetY) != Radius)
                {
                    continue;
                }

                const FIntPoint Candidate = RoundedCell + FIntPoint(OffsetX, OffsetY);
                if (!MazeGenerator->IsWalkable(Candidate.X, Candidate.Y))
                {
                    continue;
                }

                const float DistanceSquared = FVector::DistSquared2D(
                    WorldLocation,
                    GetGridCellCenter(Candidate));

                if (DistanceSquared < BestDistanceSquared)
                {
                    BestDistanceSquared = DistanceSquared;
                    BestCell = Candidate;
                }
            }
        }

        if (BestCell.X >= 0)
        {
            break;
        }
    }

    return BestCell;
}

FVector APMEPatrolEnemy::GetGridCellCenter(const FIntPoint Cell) const
{
    if (!MazeGenerator)
    {
        return GetActorLocation();
    }

    FVector Result = MazeGenerator->GridToWorld(Cell.X, Cell.Y);
    Result.Z = GetActorLocation().Z;
    return Result;
}

FVector APMEPatrolEnemy::GetSafePointInsideCell(
    const FVector& DesiredLocation,
    const FIntPoint Cell) const
{
    const FVector CellCenter = GetGridCellCenter(Cell);
    const float CapsuleRadius = PatrolCollision
        ? PatrolCollision->GetScaledCapsuleRadius()
        : 28.0f;
    const float SafeHalfExtent = FMath::Max(
        1.0f,
        GetTileSize() * 0.5f - CapsuleRadius - 4.0f);

    return FVector(
        FMath::Clamp(DesiredLocation.X, CellCenter.X - SafeHalfExtent, CellCenter.X + SafeHalfExtent),
        FMath::Clamp(DesiredLocation.Y, CellCenter.Y - SafeHalfExtent, CellCenter.Y + SafeHalfExtent),
        GetActorLocation().Z);
}

bool APMEPatrolEnemy::IsCardinalWalkableSegment(
    const FIntPoint StartCell,
    const FIntPoint TargetCell) const
{
    if (!MazeGenerator ||
        !MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
        !MazeGenerator->IsWalkable(TargetCell.X, TargetCell.Y))
    {
        return false;
    }

    const int32 DeltaX = TargetCell.X - StartCell.X;
    const int32 DeltaY = TargetCell.Y - StartCell.Y;

    if (DeltaX != 0 && DeltaY != 0)
    {
        return false;
    }

    const FIntPoint Step(
        DeltaX == 0 ? 0 : (DeltaX > 0 ? 1 : -1),
        DeltaY == 0 ? 0 : (DeltaY > 0 ? 1 : -1));

    FIntPoint Cell = StartCell;
    while (Cell != TargetCell)
    {
        Cell += Step;
        if (!MazeGenerator->IsWalkable(Cell.X, Cell.Y))
        {
            return false;
        }
    }

    return true;
}

bool APMEPatrolEnemy::IsPatrolEndpoint(const int32 PointIndex) const
{
    if (!PatrolPoints.IsValidIndex(PointIndex) || PatrolPoints.Num() < 2)
    {
        return false;
    }

    if (!bPingPongPatrol)
    {
        // A looping patrol has no reversal endpoint and therefore remains
        // continuous unless legacy per-waypoint pauses are explicitly enabled.
        return false;
    }

    return PointIndex == 0 || PointIndex == PatrolPoints.Num() - 1;
}

void APMEPatrolEnemy::AdvanceWaypoint()
{
    if (PatrolPoints.Num() < 2)
    {
        CurrentWaypointIndex = 0;
        return;
    }

    if (!bPingPongPatrol)
    {
        CurrentWaypointIndex = (CurrentWaypointIndex + 1) % PatrolPoints.Num();
        return;
    }

    if (CurrentWaypointIndex >= PatrolPoints.Num() - 1)
    {
        PatrolDirection = -1;
    }
    else if (CurrentWaypointIndex <= 0)
    {
        PatrolDirection = 1;
    }

    CurrentWaypointIndex = FMath::Clamp(
        CurrentWaypointIndex + PatrolDirection,
        0,
        PatrolPoints.Num() - 1);
}

bool APMEPatrolEnemy::IsRoundActive() const
{
    const APMEGameState* MazeGameState = GetWorld() ? GetWorld()->GetGameState<APMEGameState>() : nullptr;
    return MazeGameState && !MazeGameState->IsWaitingForPlayers() && !MazeGameState->IsRoundFinished();
}

bool APMEPatrolEnemy::IsValidChaseTarget(const APMEPlayerCharacter* PlayerCharacter) const
{
    if (!IsValid(PlayerCharacter))
    {
        return false;
    }

    const APMEPlayerState* MazePlayerState = PlayerCharacter->GetPlayerState<APMEPlayerState>();
    return !MazePlayerState || (!MazePlayerState->HasReachedExit() && !MazePlayerState->IsDowned());
}

float APMEPatrolEnemy::GetTileSize() const
{
    return MazeGenerator
        ? FMath::Max(1.0f, MazeGenerator->GetTileSize())
        : FMath::Max(1.0f, CachedTileSize);
}
