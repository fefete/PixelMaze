#include "PMEFogOfWarActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PMEMazeGenerator.h"
#include "UObject/ConstructorHelpers.h"

APMEFogOfWarActor::APMEFogOfWarActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FogInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FogInstances"));
	FogInstances->SetupAttachment(SceneRoot);
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
	RefreshFogIfRequired(true);
}

void APMEFogOfWarActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AccumulatedTime += DeltaSeconds;
	if (UpdateInterval > 0.0f && AccumulatedTime < UpdateInterval)
	{
		return;
	}

	AccumulatedTime = 0.0f;
	RefreshFogIfRequired(false);
}

void APMEFogOfWarActor::InitializeFog(APawn* InTrackedPawn, const float InRevealRadiusInTiles)
{
	TrackedPawn = InTrackedPawn;
	RevealRadiusInTiles = FMath::Max(0.5f, InRevealRadiusInTiles);
	RefreshFogIfRequired(true);
}

void APMEFogOfWarActor::SetTrackedPawn(APawn* InTrackedPawn)
{
	TrackedPawn = InTrackedPawn;
	LastPawnCell = FIntPoint(-1, -1);
	RefreshFogIfRequired(true);
}

void APMEFogOfWarActor::SetRevealRadiusInTiles(const float NewRadius)
{
	RevealRadiusInTiles = FMath::Clamp(NewRadius, 0.5f, 30.0f);
	RefreshFogIfRequired(true);
}

void APMEFogOfWarActor::SetFogEnabled(const bool bEnabled)
{
	bFogEnabled = bEnabled;
	FogInstances->SetVisibility(bFogEnabled, true);

	if (bFogEnabled)
	{
		RefreshFogIfRequired(true);
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

void APMEFogOfWarActor::RefreshFogIfRequired(const bool bForce)
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

	const FIntPoint PawnCell = MazeGenerator->WorldToGrid(TrackedPawn->GetActorLocation());
	const int32 MazeSeed = MazeGenerator->GetActiveSeed();
	const int32 GridWidth = MazeGenerator->GetGridWidth();
	const int32 GridHeight = MazeGenerator->GetGridHeight();

	const bool bMazeChanged =
		!bPersistentGridBuilt ||
		MazeSeed != LastMazeSeed ||
		GridWidth != LastGridWidth ||
		GridHeight != LastGridHeight;

	const bool bRadiusChanged = !FMath::IsNearlyEqual(RevealRadiusInTiles, LastAppliedRadius);

	if (!bForce && !bMazeChanged && !bRadiusChanged && PawnCell == LastPawnCell)
	{
		return;
	}

	LastMazeSeed = MazeSeed;
	LastGridWidth = GridWidth;
	LastGridHeight = GridHeight;

	if (bMazeChanged)
	{
		BuildPersistentFogGrid();
	}

	// Crucially, no ClearInstances() occurs while the player moves. Every maze
	// tile owns a permanent instance and only changed transforms are submitted.
	// Until the render thread receives the complete batch, the previous fog
	// layout remains visible, so the maze can never flash fully uncovered.
	UpdateFogVisibility(PawnCell, bMazeChanged || bRadiusChanged || bForce);

	LastPawnCell = PawnCell;
	LastAppliedRadius = RevealRadiusInTiles;
}

void APMEFogOfWarActor::BuildPersistentFogGrid()
{
	bPersistentGridBuilt = false;
	CoveredTileStates.Reset();

	if (!MazeGenerator || LastGridWidth <= 0 || LastGridHeight <= 0)
	{
		return;
	}

	const int32 TotalTiles = LastGridWidth * LastGridHeight;

	FogInstances->ClearInstances();
	FogInstances->PreAllocateInstancesMemory(TotalTiles);
	CoveredTileStates.Init(1, TotalTiles);

	// Initially cover every tile. The reveal radius is applied afterwards by
	// moving only the visible instances below the map.
	for (int32 Y = 0; Y < LastGridHeight; ++Y)
	{
		for (int32 X = 0; X < LastGridWidth; ++X)
		{
			FogInstances->AddInstanceWorldSpace(MakeFogTransform(X, Y, true));
		}
	}

	bPersistentGridBuilt = FogInstances->GetInstanceCount() == TotalTiles;
}

void APMEFogOfWarActor::UpdateFogVisibility(const FIntPoint& PawnCell, const bool bForceAll)
{
	if (!bPersistentGridBuilt || !MazeGenerator)
	{
		return;
	}

	const float RadiusSquared = FMath::Square(RevealRadiusInTiles);
	TArray<int32> ChangedInstanceIndices;
	ChangedInstanceIndices.Reserve((FMath::CeilToInt(RevealRadiusInTiles) * 2 + 3) * 8);

	// First calculate the complete transition without touching the render
	// state. This lets the last UpdateInstanceTransform publish every change
	// together instead of exposing an intermediate fog layout.
	for (int32 Y = 0; Y < LastGridHeight; ++Y)
	{
		for (int32 X = 0; X < LastGridWidth; ++X)
		{
			const float DeltaX = static_cast<float>(X - PawnCell.X);
			const float DeltaY = static_cast<float>(Y - PawnCell.Y);
			const bool bShouldBeCovered = (DeltaX * DeltaX + DeltaY * DeltaY) > RadiusSquared;
			const int32 InstanceIndex = ToFogInstanceIndex(X, Y);

			if (!CoveredTileStates.IsValidIndex(InstanceIndex))
			{
				continue;
			}

			const uint8 DesiredState = bShouldBeCovered ? 1 : 0;
			if (!bForceAll && CoveredTileStates[InstanceIndex] == DesiredState)
			{
				continue;
			}

			CoveredTileStates[InstanceIndex] = DesiredState;
			ChangedInstanceIndices.Add(InstanceIndex);
		}
	}

	for (int32 ChangeIndex = 0; ChangeIndex < ChangedInstanceIndices.Num(); ++ChangeIndex)
	{
		const int32 InstanceIndex = ChangedInstanceIndices[ChangeIndex];
		const int32 X = InstanceIndex % LastGridWidth;
		const int32 Y = InstanceIndex / LastGridWidth;
		const bool bCovered = CoveredTileStates[InstanceIndex] != 0;
		const bool bPublishCompleteBatch = ChangeIndex == ChangedInstanceIndices.Num() - 1;

		FogInstances->UpdateInstanceTransform(
			InstanceIndex,
			MakeFogTransform(X, Y, bCovered),
			true,
			bPublishCompleteBatch,
			true);
	}
}

FTransform APMEFogOfWarActor::MakeFogTransform(const int32 X, const int32 Y, const bool bCovered) const
{
	if (!MazeGenerator)
	{
		return FTransform::Identity;
	}

	const float TileSize = MazeGenerator->GetTileSize();
	const float FogZ = MazeGenerator->GetWallHeight() + FogTileThickness + 15.0f;

	if (bCovered)
	{
		const FVector Location = MazeGenerator->GridToWorld(X, Y) + FVector(0.0f, 0.0f, FogZ);
		const FVector Scale(TileSize / 100.0f, TileSize / 100.0f, FogTileThickness / 100.0f);
		return FTransform(FRotator::ZeroRotator, Location, Scale);
	}

	// Keep the instance alive and its index stable, but place it well below the
	// maze with a tiny scale. Removing instances would recreate the HISM render
	// data and is the source of the one-frame full-map flash.
	const float HiddenDepth = FMath::Max(10000.0f, MazeGenerator->GetWallHeight() * 20.0f);
	const FVector HiddenLocation = MazeGenerator->GridToWorld(X, Y) + FVector(0.0f, 0.0f, -HiddenDepth);
	return FTransform(FRotator::ZeroRotator, HiddenLocation, FVector(0.001f));
}

int32 APMEFogOfWarActor::ToFogInstanceIndex(const int32 X, const int32 Y) const
{
	return Y * LastGridWidth + X;
}
