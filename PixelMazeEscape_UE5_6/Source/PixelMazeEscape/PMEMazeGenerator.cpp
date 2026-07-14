#include "PMEMazeGenerator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Containers/Queue.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace PixelMaze
{
	constexpr uint8 Wall = 1;
	constexpr uint8 Floor = 0;
}

APMEMazeGenerator::APMEMazeGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 2.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FloorInstances"));
	FloorInstances->SetupAttachment(SceneRoot);
	FloorInstances->SetCollisionObjectType(ECC_WorldStatic);
	FloorInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorInstances->SetCollisionResponseToAllChannels(ECR_Block);
	// Horizontal sight traces must never be consumed by the floor.
	FloorInstances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	FloorInstances->SetCanEverAffectNavigation(false);
	FloorInstances->SetCastShadow(false);

	WallInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WallInstances"));
	WallInstances->SetupAttachment(SceneRoot);
	WallInstances->SetCollisionObjectType(ECC_WorldStatic);
	WallInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallInstances->SetCollisionResponseToAllChannels(ECR_Block);
	// Explicitly block the channel used by enemy perception.
	WallInstances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	WallInstances->SetCanEverAffectNavigation(false);
	WallInstances->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FloorInstances->SetStaticMesh(CubeMesh.Object);
		WallInstances->SetStaticMesh(CubeMesh.Object);
	}
}

void APMEMazeGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMEMazeGenerator, GeneratedLevel);
	DOREPLIFETIME(APMEMazeGenerator, ActiveSeed);
}

void APMEMazeGenerator::GenerateMaze(const int32 Level, const int32 OverrideSeed)
{
	const int32 SafeLevel = FMath::Max(1, Level);
	const int32 NewSeed = OverrideSeed != 0
		                      ? OverrideSeed
		                      : FMath::Max(1, static_cast<int32>(FPlatformTime::Cycles64() & 0x7fffffff));

	GeneratedLevel = SafeLevel;
	ActiveSeed = NewSeed;
	RebuildFromDefinition();

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void APMEMazeGenerator::OnRep_MazeDefinition()
{
	if (GeneratedLevel > 0 && ActiveSeed != 0)
	{
		RebuildFromDefinition();
	}
}

void APMEMazeGenerator::RebuildFromDefinition()
{
	ClearMaze();

	if (GeneratedLevel <= 0 || ActiveSeed == 0)
	{
		return;
	}

	const int32 CellsX = FMath::Clamp(BaseCellsX + ((GeneratedLevel - 1) * 2), 3, MaximumCellsX);
	const int32 CellsY = FMath::Clamp(BaseCellsY + ((GeneratedLevel - 1) * 2), 3, MaximumCellsY);

	FRandomStream RandomStream(ActiveSeed);
	BuildLogicalMaze(CellsX, CellsY, RandomStream);
	FindFarthestExit();
	FindVersusStartCells();
	ApplyGeneratedMaterials();
	BuildInstances();
}

void APMEMazeGenerator::ClearMaze()
{
	FloorInstances->ClearInstances();
	WallInstances->ClearInstances();
	Grid.Reset();
	GridWidth = 0;
	GridHeight = 0;
	StartCell = FIntPoint(1, 1);
	ExitCell = StartCell;
	VersusStartCells[0] = StartCell;
	VersusStartCells[1] = StartCell;
}

void APMEMazeGenerator::BuildLogicalMaze(const int32 CellsX, const int32 CellsY, FRandomStream& RandomStream)
{
	GridWidth = CellsX * 2 + 1;
	GridHeight = CellsY * 2 + 1;
	Grid.Init(PixelMaze::Wall, GridWidth * GridHeight);

	TArray<uint8> Visited;
	Visited.Init(0, CellsX * CellsY);

	auto LogicalIndex = [CellsX](const int32 X, const int32 Y)
	{
		return Y * CellsX + X;
	};

	auto LogicalToGrid = [](const FIntPoint& Logical)
	{
		return FIntPoint(Logical.X * 2 + 1, Logical.Y * 2 + 1);
	};

	TArray<FIntPoint> Stack;
	Stack.Reserve(CellsX * CellsY);
	Stack.Add(FIntPoint(0, 0));
	Visited[LogicalIndex(0, 0)] = 1;

	const FIntPoint InitialGridCell = LogicalToGrid(FIntPoint(0, 0));
	Grid[ToIndex(InitialGridCell.X, InitialGridCell.Y)] = PixelMaze::Floor;

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	while (!Stack.IsEmpty())
	{
		const FIntPoint Current = Stack.Last();
		TArray<FIntPoint, TInlineAllocator<4>> Candidates;

		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Candidate = Current + Direction;
			if (Candidate.X >= 0 && Candidate.X < CellsX &&
				Candidate.Y >= 0 && Candidate.Y < CellsY &&
				Visited[LogicalIndex(Candidate.X, Candidate.Y)] == 0)
			{
				Candidates.Add(Candidate);
			}
		}

		if (Candidates.IsEmpty())
		{
			Stack.Pop(EAllowShrinking::No);
			continue;
		}

		const FIntPoint Next = Candidates[RandomStream.RandRange(0, Candidates.Num() - 1)];
		Visited[LogicalIndex(Next.X, Next.Y)] = 1;

		const FIntPoint CurrentGrid = LogicalToGrid(Current);
		const FIntPoint NextGrid = LogicalToGrid(Next);
		const FIntPoint Between((CurrentGrid.X + NextGrid.X) / 2, (CurrentGrid.Y + NextGrid.Y) / 2);

		Grid[ToIndex(Between.X, Between.Y)] = PixelMaze::Floor;
		Grid[ToIndex(NextGrid.X, NextGrid.Y)] = PixelMaze::Floor;
		Stack.Add(Next);
	}

	StartCell = FIntPoint(1, 1);
}

void APMEMazeGenerator::FindFarthestExit()
{
	if (!IsWalkable(StartCell.X, StartCell.Y))
	{
		ExitCell = StartCell;
		return;
	}

	TArray<int32> Distances;
	Distances.Init(-1, GridWidth * GridHeight);

	TQueue<FIntPoint> Pending;
	Pending.Enqueue(StartCell);
	Distances[ToIndex(StartCell.X, StartCell.Y)] = 0;

	ExitCell = StartCell;
	int32 FurthestDistance = 0;

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	FIntPoint Current;
	while (Pending.Dequeue(Current))
	{
		const int32 CurrentDistance = Distances[ToIndex(Current.X, Current.Y)];
		if (CurrentDistance > FurthestDistance)
		{
			FurthestDistance = CurrentDistance;
			ExitCell = Current;
		}

		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Next = Current + Direction;
			if (!IsWalkable(Next.X, Next.Y))
			{
				continue;
			}

			const int32 NextIndex = ToIndex(Next.X, Next.Y);
			if (Distances[NextIndex] >= 0)
			{
				continue;
			}

			Distances[NextIndex] = CurrentDistance + 1;
			Pending.Enqueue(Next);
		}
	}
}


void APMEMazeGenerator::FindVersusStartCells()
{
	VersusStartCells[0] = StartCell;
	VersusStartCells[1] = StartCell;

	if (!IsWalkable(ExitCell.X, ExitCell.Y))
	{
		return;
	}

	TArray<int32> DistancesFromExit;
	DistancesFromExit.Init(-1, GridWidth * GridHeight);

	TQueue<FIntPoint> Pending;
	Pending.Enqueue(ExitCell);
	DistancesFromExit[ToIndex(ExitCell.X, ExitCell.Y)] = 0;

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	FIntPoint Current;
	int32 MaximumDistance = 0;
	while (Pending.Dequeue(Current))
	{
		const int32 CurrentDistance = DistancesFromExit[ToIndex(Current.X, Current.Y)];
		MaximumDistance = FMath::Max(MaximumDistance, CurrentDistance);

		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Next = Current + Direction;
			if (!IsWalkable(Next.X, Next.Y))
			{
				continue;
			}

			const int32 NextIndex = ToIndex(Next.X, Next.Y);
			if (DistancesFromExit[NextIndex] >= 0)
			{
				continue;
			}

			DistancesFromExit[NextIndex] = CurrentDistance + 1;
			Pending.Enqueue(Next);
		}
	}

	TArray<FIntPoint> WalkableCells;
	WalkableCells.Reserve(GridWidth * GridHeight);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			if (IsWalkable(X, Y) && FIntPoint(X, Y) != ExitCell)
			{
				WalkableCells.Add(FIntPoint(X, Y));
			}
		}
	}

	if (WalkableCells.Num() < 2)
	{
		return;
	}

	const int32 RequiredDistanceFromExit = FMath::Min(
		VersusMinimumDistanceFromExitInTiles,
		FMath::Max(1, MaximumDistance - 1));
	const int32 RequiredSeparationSquared = FMath::Square(VersusMinimumSpawnSeparationInTiles);

	bool bFoundPreferredPair = false;
	int64 BestScore = MIN_int64;
	FIntPoint BestFirst = WalkableCells[0];
	FIntPoint BestSecond = WalkableCells[1];

	// Exact equality of path distance makes both routes to the finish equally long.
	// The score then favours starts far from the exit and far apart from each other.
	for (int32 FirstIndex = 0; FirstIndex < WalkableCells.Num() - 1; ++FirstIndex)
	{
		const FIntPoint First = WalkableCells[FirstIndex];
		const int32 FirstDistance = DistancesFromExit[ToIndex(First.X, First.Y)];
		if (FirstDistance < RequiredDistanceFromExit)
		{
			continue;
		}

		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < WalkableCells.Num(); ++SecondIndex)
		{
			const FIntPoint Second = WalkableCells[SecondIndex];
			const int32 SecondDistance = DistancesFromExit[ToIndex(Second.X, Second.Y)];
			if (SecondDistance != FirstDistance)
			{
				continue;
			}

			const int32 DeltaX = First.X - Second.X;
			const int32 DeltaY = First.Y - Second.Y;
			const int32 SeparationSquared = DeltaX * DeltaX + DeltaY * DeltaY;
			const bool bMeetsPreferredSeparation = SeparationSquared >= RequiredSeparationSquared;

			const int64 Score =
				static_cast<int64>(bMeetsPreferredSeparation ? 1 : 0) * 1000000000000LL +
				static_cast<int64>(FirstDistance) * 1000000LL +
				static_cast<int64>(SeparationSquared);

			if (Score > BestScore)
			{
				BestScore = Score;
				BestFirst = First;
				BestSecond = Second;
				bFoundPreferredPair = true;
			}
		}
	}

	if (!bFoundPreferredPair)
	{
		// Extremely small mazes may not contain two cells at exactly the same
		// distance. Fall back to the most separated valid pair while minimising
		// the difference in route length.
		BestScore = MIN_int64;
		for (int32 FirstIndex = 0; FirstIndex < WalkableCells.Num() - 1; ++FirstIndex)
		{
			const FIntPoint First = WalkableCells[FirstIndex];
			const int32 FirstDistance = DistancesFromExit[ToIndex(First.X, First.Y)];

			for (int32 SecondIndex = FirstIndex + 1; SecondIndex < WalkableCells.Num(); ++SecondIndex)
			{
				const FIntPoint Second = WalkableCells[SecondIndex];
				const int32 SecondDistance = DistancesFromExit[ToIndex(Second.X, Second.Y)];
				const int32 RouteDifference = FMath::Abs(FirstDistance - SecondDistance);
				const int32 DeltaX = First.X - Second.X;
				const int32 DeltaY = First.Y - Second.Y;
				const int32 SeparationSquared = DeltaX * DeltaX + DeltaY * DeltaY;
				const int32 ShorterRoute = FMath::Min(FirstDistance, SecondDistance);

				const int64 Score =
					-static_cast<int64>(RouteDifference) * 1000000000LL +
					static_cast<int64>(ShorterRoute) * 1000000LL +
					static_cast<int64>(SeparationSquared);

				if (Score > BestScore)
				{
					BestScore = Score;
					BestFirst = First;
					BestSecond = Second;
				}
			}
		}
	}

	VersusStartCells[0] = BestFirst;
	VersusStartCells[1] = BestSecond;
}

void APMEMazeGenerator::BuildInstances()
{
	if (GridWidth <= 0 || GridHeight <= 0)
	{
		return;
	}

	FloorInstances->PreAllocateInstancesMemory(GridWidth * GridHeight);
	WallInstances->PreAllocateInstancesMemory(GridWidth * GridHeight);

	const FVector FloorScale(TileSize / 100.0f, TileSize / 100.0f, FloorThickness / 100.0f);
	const FVector WallScale(TileSize / 100.0f, TileSize / 100.0f, WallHeight / 100.0f);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FVector TileLocation = GridToWorld(X, Y);

			const FTransform FloorTransform(
				FRotator::ZeroRotator,
				TileLocation - FVector(0.0f, 0.0f, FloorThickness * 0.5f),
				FloorScale);
			FloorInstances->AddInstanceWorldSpace(FloorTransform);

			if (!IsWalkable(X, Y))
			{
				const FTransform WallTransform(
					FRotator::ZeroRotator,
					TileLocation + FVector(0.0f, 0.0f, WallHeight * 0.5f),
					WallScale);
				WallInstances->AddInstanceWorldSpace(WallTransform);
			}
		}
	}
}

void APMEMazeGenerator::ApplyGeneratedMaterials()
{
	if (UMaterialInterface* FloorMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/PixelMaze/Materials/M_Floor.M_Floor")))
	{
		FloorInstances->SetMaterial(0, FloorMaterial);
	}

	if (UMaterialInterface* WallMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/PixelMaze/Materials/M_Wall.M_Wall")))
	{
		WallInstances->SetMaterial(0, WallMaterial);
	}
}

FVector APMEMazeGenerator::GetPlayerStartWorldLocation() const
{
	return GridToWorld(StartCell.X, StartCell.Y) + FVector(0.0f, 0.0f, 50.0f);
}

FVector APMEMazeGenerator::GetVersusPlayerStartWorldLocation(const int32 PlayerIndex) const
{
	const FIntPoint Start = GetVersusPlayerStartCell(PlayerIndex);
	return GridToWorld(Start.X, Start.Y) + FVector(0.0f, 0.0f, 50.0f);
}

FIntPoint APMEMazeGenerator::GetVersusPlayerStartCell(const int32 PlayerIndex) const
{
	return VersusStartCells[FMath::Clamp(PlayerIndex, 1, 2) - 1];
}

FVector APMEMazeGenerator::GetExitWorldLocation() const
{
	return GridToWorld(ExitCell.X, ExitCell.Y) + FVector(0.0f, 0.0f, 50.0f);
}

bool APMEMazeGenerator::IsWalkable(const int32 X, const int32 Y) const
{
	return IsInBounds(X, Y) && Grid.IsValidIndex(ToIndex(X, Y)) && Grid[ToIndex(X, Y)] == PixelMaze::Floor;
}

FVector APMEMazeGenerator::GridToWorld(const int32 X, const int32 Y) const
{
	const float CenterOffsetX = static_cast<float>(GridWidth - 1) * 0.5f;
	const float CenterOffsetY = static_cast<float>(GridHeight - 1) * 0.5f;

	return GetActorLocation() + FVector(
		(static_cast<float>(X) - CenterOffsetX) * TileSize,
		(static_cast<float>(Y) - CenterOffsetY) * TileSize,
		0.0f);
}

FIntPoint APMEMazeGenerator::WorldToGrid(const FVector& WorldLocation) const
{
	if (GridWidth <= 0 || GridHeight <= 0 || TileSize <= KINDA_SMALL_NUMBER)
	{
		return FIntPoint(-1, -1);
	}

	const FVector Local = WorldLocation - GetActorLocation();
	const float CenterOffsetX = static_cast<float>(GridWidth - 1) * 0.5f;
	const float CenterOffsetY = static_cast<float>(GridHeight - 1) * 0.5f;

	return FIntPoint(
		FMath::RoundToInt(Local.X / TileSize + CenterOffsetX),
		FMath::RoundToInt(Local.Y / TileSize + CenterOffsetY));
}

bool APMEMazeGenerator::HasClearGridLineOfSight(
	const FVector& StartWorldLocation,
	const FVector& EndWorldLocation) const
{
	if (GridWidth <= 0 || GridHeight <= 0 || TileSize <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector StartLocal = StartWorldLocation - GetActorLocation();
	const FVector EndLocal = EndWorldLocation - GetActorLocation();
	const double CenterOffsetX = static_cast<double>(GridWidth - 1) * 0.5;
	const double CenterOffsetY = static_cast<double>(GridHeight - 1) * 0.5;

	// Shift by half a cell so integer boundaries match grid-cell edges.
	const double StartX = static_cast<double>(StartLocal.X) / TileSize + CenterOffsetX + 0.5;
	const double StartY = static_cast<double>(StartLocal.Y) / TileSize + CenterOffsetY + 0.5;
	const double EndX = static_cast<double>(EndLocal.X) / TileSize + CenterOffsetX + 0.5;
	const double EndY = static_cast<double>(EndLocal.Y) / TileSize + CenterOffsetY + 0.5;

	int32 CellX = FMath::FloorToInt(StartX);
	int32 CellY = FMath::FloorToInt(StartY);
	const int32 EndCellX = FMath::FloorToInt(EndX);
	const int32 EndCellY = FMath::FloorToInt(EndY);

	if (!IsWalkable(CellX, CellY) || !IsWalkable(EndCellX, EndCellY))
	{
		return false;
	}

	const double DeltaX = EndX - StartX;
	const double DeltaY = EndY - StartY;
	const int32 StepX = DeltaX > 0.0 ? 1 : (DeltaX < 0.0 ? -1 : 0);
	const int32 StepY = DeltaY > 0.0 ? 1 : (DeltaY < 0.0 ? -1 : 0);
	const double Infinite = TNumericLimits<double>::Max();
	const double TDeltaX = StepX == 0 ? Infinite : FMath::Abs(1.0 / DeltaX);
	const double TDeltaY = StepY == 0 ? Infinite : FMath::Abs(1.0 / DeltaY);

	double TMaxX = Infinite;
	if (StepX > 0)
	{
		TMaxX = (static_cast<double>(CellX + 1) - StartX) / DeltaX;
	}
	else if (StepX < 0)
	{
		TMaxX = (StartX - static_cast<double>(CellX)) / -DeltaX;
	}

	double TMaxY = Infinite;
	if (StepY > 0)
	{
		TMaxY = (static_cast<double>(CellY + 1) - StartY) / DeltaY;
	}
	else if (StepY < 0)
	{
		TMaxY = (StartY - static_cast<double>(CellY)) / -DeltaY;
	}

	const int32 MaxIterations = FMath::Max(1, GridWidth * GridHeight * 4);
	for (int32 Iteration = 0; Iteration < MaxIterations; ++Iteration)
	{
		if (CellX == EndCellX && CellY == EndCellY)
		{
			return true;
		}

		if (FMath::IsNearlyEqual(TMaxX, TMaxY, 1.e-9))
		{
			const int32 NextX = CellX + StepX;
			const int32 NextY = CellY + StepY;

			// A line passing exactly through a corner must not see through the
			// diagonal crack between two wall cells.
			if ((StepX != 0 && !IsWalkable(NextX, CellY)) ||
				(StepY != 0 && !IsWalkable(CellX, NextY)))
			{
				return false;
			}

			CellX = NextX;
			CellY = NextY;
			TMaxX += TDeltaX;
			TMaxY += TDeltaY;
		}
		else if (TMaxX < TMaxY)
		{
			CellX += StepX;
			TMaxX += TDeltaX;
		}
		else
		{
			CellY += StepY;
			TMaxY += TDeltaY;
		}

		if (!IsWalkable(CellX, CellY))
		{
			return false;
		}
	}

	return false;
}

bool APMEMazeGenerator::IsInBounds(const int32 X, const int32 Y) const
{
	return X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight;
}

int32 APMEMazeGenerator::ToIndex(const int32 X, const int32 Y) const
{
	return Y * GridWidth + X;
}
