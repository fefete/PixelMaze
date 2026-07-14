#include "PMEFogOfWarActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PMEMazeGenerator.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	float SmoothStep01(const float Value)
	{
		const float T = FMath::Clamp(Value, 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}
}

APMEFogOfWarActor::APMEFogOfWarActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	bReplicates = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FogInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FogInstances"));
	FogInstances->SetupAttachment(SceneRoot);
	FogInstances->SetMobility(EComponentMobility::Movable);
	FogInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogInstances->SetCanEverAffectNavigation(false);
	FogInstances->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FogInstances->SetStaticMesh(CubeMesh.Object);
	}
}

void APMEFogOfWarActor::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* FogMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/PixelMaze/Materials/M_Fog.M_Fog")))
	{
		FogInstances->SetMaterial(0, FogMaterial);
	}

	ResolveMazeGenerator();
	RefreshFog(0.0f, true);
}

void APMEFogOfWarActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFogEnabled || !TrackedPawn)
	{
		return;
	}

	AccumulatedTime += DeltaSeconds;
	if (UpdateInterval > 0.0f && AccumulatedTime < UpdateInterval)
	{
		return;
	}

	const float EvaluationDelta = AccumulatedTime;
	AccumulatedTime = 0.0f;
	RefreshFog(EvaluationDelta, false);
}

void APMEFogOfWarActor::InitializeFog(APawn* InTrackedPawn, const float InRevealRadiusInTiles)
{
	TrackedPawn = InTrackedPawn;
	RevealRadiusInTiles = FMath::Max(0.5f, InRevealRadiusInTiles);
	bHasRevealCenter = false;
	RefreshFog(0.0f, true);
}

void APMEFogOfWarActor::SetTrackedPawn(APawn* InTrackedPawn)
{
	TrackedPawn = InTrackedPawn;
	bHasRevealCenter = false;
	RefreshFog(0.0f, true);
}

void APMEFogOfWarActor::SetRevealRadiusInTiles(const float NewRadius)
{
	RevealRadiusInTiles = FMath::Clamp(NewRadius, 0.5f, 30.0f);
	RefreshFog(0.0f, false);
}

void APMEFogOfWarActor::SetFogEnabled(const bool bEnabled)
{
	bFogEnabled = bEnabled;
	FogInstances->SetVisibility(bFogEnabled, true);

	if (bFogEnabled)
	{
		bHasRevealCenter = false;
		RefreshFog(0.0f, true);
	}
}

void APMEFogOfWarActor::ResolveMazeGenerator()
{
	if (!MazeGenerator)
	{
		MazeGenerator = Cast<APMEMazeGenerator>(
			UGameplayStatics::GetActorOfClass(this, APMEMazeGenerator::StaticClass()));
	}
}

void APMEFogOfWarActor::RefreshFog(const float DeltaSeconds, const bool bForce)
{
	if (!bFogEnabled || !TrackedPawn)
	{
		return;
	}

	ResolveMazeGenerator();
	if (!MazeGenerator || MazeGenerator->GetGridWidth() <= 0 || MazeGenerator->GetGridHeight() <= 0)
	{
		return;
	}

	const int32 MazeSeed = MazeGenerator->GetActiveSeed();
	const int32 GridWidth = MazeGenerator->GetGridWidth();
	const int32 GridHeight = MazeGenerator->GetGridHeight();

	const bool bMazeChanged =
		!bPersistentGridBuilt ||
		MazeSeed != LastMazeSeed ||
		GridWidth != LastGridWidth ||
		GridHeight != LastGridHeight;

	const bool bRadiusChanged = !FMath::IsNearlyEqual(RevealRadiusInTiles, LastAppliedRadius);

	LastMazeSeed = MazeSeed;
	LastGridWidth = GridWidth;
	LastGridHeight = GridHeight;

	if (bMazeChanged)
	{
		BuildPersistentFogGrid();
		bHasRevealCenter = false;
	}

	if (!bPersistentGridBuilt)
	{
		return;
	}

	const FVector2D TargetRevealCenter = WorldToContinuousGrid(TrackedPawn->GetActorLocation());

	if (!bHasRevealCenter || bForce || FogFollowInterpolationSpeed <= KINDA_SMALL_NUMBER)
	{
		SmoothedRevealCenter = TargetRevealCenter;
		bHasRevealCenter = true;
	}
	else
	{
		SmoothedRevealCenter.X = FMath::FInterpTo(
			SmoothedRevealCenter.X,
			TargetRevealCenter.X,
			DeltaSeconds,
			FogFollowInterpolationSpeed);

		SmoothedRevealCenter.Y = FMath::FInterpTo(
			SmoothedRevealCenter.Y,
			TargetRevealCenter.Y,
			DeltaSeconds,
			FogFollowInterpolationSpeed);
	}

	// Snap only when a new maze is built or when smoothing is explicitly
	// disabled. Radius changes and normal movement transition progressively.
	const bool bSnapAll = bMazeChanged || bForce || FogTileTransitionSpeed <= KINDA_SMALL_NUMBER;
	UpdateFogVisibility(SmoothedRevealCenter, DeltaSeconds, bSnapAll);

	LastAppliedRadius = RevealRadiusInTiles;

	// Keep evaluating after a radius change even if the pawn is stationary;
	// UpdateFogVisibility performs the interpolation until every tile settles.
	if (bRadiusChanged && DeltaSeconds <= KINDA_SMALL_NUMBER)
	{
		UpdateFogVisibility(SmoothedRevealCenter, 1.0f / 60.0f, false);
	}
}

void APMEFogOfWarActor::BuildPersistentFogGrid()
{
	bPersistentGridBuilt = false;
	FogCoverageValues.Reset();

	if (!MazeGenerator || LastGridWidth <= 0 || LastGridHeight <= 0)
	{
		return;
	}

	const int32 TotalTiles = LastGridWidth * LastGridHeight;

	FogInstances->ClearInstances();
	FogInstances->PreAllocateInstancesMemory(TotalTiles);
	FogCoverageValues.Init(1.0f, TotalTiles);

	for (int32 Y = 0; Y < LastGridHeight; ++Y)
	{
		for (int32 X = 0; X < LastGridWidth; ++X)
		{
			FogInstances->AddInstanceWorldSpace(MakeFogTransform(X, Y, 1.0f));
		}
	}

	bPersistentGridBuilt = FogInstances->GetInstanceCount() == TotalTiles;
}

void APMEFogOfWarActor::UpdateFogVisibility(
	const FVector2D& RevealCenterInGrid,
	const float DeltaSeconds,
	const bool bSnapAll)
{
	if (!bPersistentGridBuilt || !MazeGenerator)
	{
		return;
	}

	const float Feather = FMath::Max(0.0f, RevealEdgeFeatherInTiles);
	const float InnerRadius = FMath::Max(0.0f, RevealRadiusInTiles - Feather);
	const float OuterRadius = RevealRadiusInTiles + Feather;

	TArray<int32> ChangedInstanceIndices;
	ChangedInstanceIndices.Reserve(
		FMath::Max(32, FMath::CeilToInt(2.0f * PI * OuterRadius * 3.0f)));

	for (int32 Y = 0; Y < LastGridHeight; ++Y)
	{
		for (int32 X = 0; X < LastGridWidth; ++X)
		{
			const int32 InstanceIndex = ToFogInstanceIndex(X, Y);
			if (!FogCoverageValues.IsValidIndex(InstanceIndex))
			{
				continue;
			}

			const float DeltaX = static_cast<float>(X) - RevealCenterInGrid.X;
			const float DeltaY = static_cast<float>(Y) - RevealCenterInGrid.Y;
			const float DistanceInTiles = FMath::Sqrt(DeltaX * DeltaX + DeltaY * DeltaY);

			float TargetCoverage = 0.0f;
			if (Feather <= KINDA_SMALL_NUMBER)
			{
				TargetCoverage = DistanceInTiles > RevealRadiusInTiles ? 1.0f : 0.0f;
			}
			else if (DistanceInTiles <= InnerRadius)
			{
				TargetCoverage = 0.0f;
			}
			else if (DistanceInTiles >= OuterRadius)
			{
				TargetCoverage = 1.0f;
			}
			else
			{
				const float NormalizedDistance =
					(DistanceInTiles - InnerRadius) / FMath::Max(KINDA_SMALL_NUMBER, OuterRadius - InnerRadius);
				TargetCoverage = SmoothStep01(NormalizedDistance);
			}

			const float PreviousCoverage = FogCoverageValues[InstanceIndex];
			float NewCoverage = TargetCoverage;

			if (!bSnapAll)
			{
				NewCoverage = FMath::FInterpTo(
					PreviousCoverage,
					TargetCoverage,
					DeltaSeconds,
					FogTileTransitionSpeed);

				if (FMath::IsNearlyEqual(NewCoverage, TargetCoverage, TransformUpdateTolerance))
				{
					NewCoverage = TargetCoverage;
				}
			}

			if (FMath::IsNearlyEqual(PreviousCoverage, NewCoverage, TransformUpdateTolerance))
			{
				continue;
			}

			FogCoverageValues[InstanceIndex] = NewCoverage;
			ChangedInstanceIndices.Add(InstanceIndex);
		}
	}

	for (int32 ChangeIndex = 0; ChangeIndex < ChangedInstanceIndices.Num(); ++ChangeIndex)
	{
		const int32 InstanceIndex = ChangedInstanceIndices[ChangeIndex];
		const int32 X = InstanceIndex % LastGridWidth;
		const int32 Y = InstanceIndex / LastGridWidth;
		const bool bPublishCompleteBatch = ChangeIndex == ChangedInstanceIndices.Num() - 1;

		FogInstances->UpdateInstanceTransform(
			InstanceIndex,
			MakeFogTransform(X, Y, FogCoverageValues[InstanceIndex]),
			true,
			bPublishCompleteBatch,
			true);
	}
}

FVector2D APMEFogOfWarActor::WorldToContinuousGrid(const FVector& WorldLocation) const
{
	if (!MazeGenerator || LastGridWidth <= 0 || LastGridHeight <= 0)
	{
		return FVector2D::ZeroVector;
	}

	const float TileSize = MazeGenerator->GetTileSize();
	if (TileSize <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}

	const FVector Local = WorldLocation - MazeGenerator->GetActorLocation();
	const float CenterOffsetX = static_cast<float>(LastGridWidth - 1) * 0.5f;
	const float CenterOffsetY = static_cast<float>(LastGridHeight - 1) * 0.5f;

	return FVector2D(
		Local.X / TileSize + CenterOffsetX,
		Local.Y / TileSize + CenterOffsetY);
}

FTransform APMEFogOfWarActor::MakeFogTransform(
	const int32 X,
	const int32 Y,
	const float Coverage) const
{
	if (!MazeGenerator)
	{
		return FTransform::Identity;
	}

	const float ClampedCoverage = FMath::Clamp(Coverage, 0.0f, 1.0f);
	const float TileSize = MazeGenerator->GetTileSize();
	const float FogZ = MazeGenerator->GetWallHeight() + FogTileThickness + 15.0f;
	const float HiddenThreshold = FMath::Clamp(
		FullyHiddenCoverageThreshold,
		TransformUpdateTolerance,
		0.95f);

	// An opaque cube scaled almost to zero remains visible as a black pixel.
	// Remove it from the visible fog layer before it reaches that state.
	if (ClampedCoverage <= HiddenThreshold)
	{
		const float HiddenDepth = FMath::Max(10000.0f, MazeGenerator->GetWallHeight() * 20.0f);
		const FVector HiddenLocation =
			MazeGenerator->GridToWorld(X, Y) + FVector(0.0f, 0.0f, -HiddenDepth);
		return FTransform(FRotator::ZeroRotator, HiddenLocation, FVector::OneVector);
	}

	// Remap the visible portion of the transition so that a tile never becomes
	// a tiny isolated dot. It disappears once HiddenThreshold is reached.
	const float VisibleCoverage = FMath::GetRangePct(
		FVector2D(HiddenThreshold, 1.0f),
		ClampedCoverage);
	const float CurvedCoverage = FMath::Sqrt(FMath::Clamp(VisibleCoverage, 0.0f, 1.0f));
	const float VisualScale = FMath::Lerp(
		FMath::Clamp(MinimumVisibleFogScale, 0.05f, 0.75f),
		1.0f,
		CurvedCoverage);

	const float OverlapScale = FMath::Clamp(FogTileOverlapScale, 1.0f, 1.15f);
	const FVector Location = MazeGenerator->GridToWorld(X, Y) + FVector(0.0f, 0.0f, FogZ);
	const FVector Scale(
		TileSize / 100.0f * VisualScale * OverlapScale,
		TileSize / 100.0f * VisualScale * OverlapScale,
		FogTileThickness / 100.0f);

	return FTransform(FRotator::ZeroRotator, Location, Scale);
}

int32 APMEFogOfWarActor::ToFogInstanceIndex(const int32 X, const int32 Y) const
{
	return Y * LastGridWidth + X;
}
