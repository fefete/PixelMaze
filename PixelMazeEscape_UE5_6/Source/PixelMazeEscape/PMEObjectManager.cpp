#include "PMEObjectManager.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "PMEGameModeBase.h"
#include "PMEGameState.h"
#include "PMEMazeGenerator.h"
#include "PMEObjectActor.h"
#include "PMEPickup.h"
#include "PMEPlayerState.h"

APMEObjectManager::APMEObjectManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	ObjectiveClass = APMEObjectActor::StaticClass();
	PickupClass = APMEPickup::StaticClass();
}

void APMEObjectManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APMEObjectManager, PlayMode);
	DOREPLIFETIME(APMEObjectManager, SharedRequired);
	DOREPLIFETIME(APMEObjectManager, SharedActivated);
	DOREPLIFETIME(APMEObjectManager, Player1Required);
	DOREPLIFETIME(APMEObjectManager, Player1Activated);
	DOREPLIFETIME(APMEObjectManager, Player2Required);
	DOREPLIFETIME(APMEObjectManager, Player2Activated);
}

void APMEObjectManager::InitializeObjectives(APMEMazeGenerator* InMaze, EPMEPlayMode InMode, int32 Level, int32 Seed,
                                             int32 ObjectiveCount, int32 PickupCount)
{
	check(HasAuthority());
	Maze = InMaze;
	PlayMode = InMode;
	for (APMEObjectActor* Objective : Objectives) if (IsValid(Objective)) Objective->Destroy();
	for (APMEPickup* Pickup : Pickups) if (IsValid(Pickup)) Pickup->Destroy();
	Objectives.Reset();
	Pickups.Reset();
	SharedRequired = SharedActivated = Player1Required = Player1Activated = Player2Required = Player2Activated = 0;
	if (!Maze) return;

	TArray<FIntPoint> Candidates = BuildCandidateCells();
	FRandomStream Stream(Seed ^ 0x6F2A91);
	const int32 SafeObjectiveCount = FMath::Clamp(ObjectiveCount, 1, 4);
	int32 ObjectiveId = 1;
	if (PlayMode == EPMEPlayMode::Versus)
	{
		Player1Required = 0;
		Player2Required = 0;
		for (int32 P = 1; P <= 2; ++P)
		{
			const FIntPoint Start = Maze->GetVersusPlayerStartCell(P);
			for (int32 I = 0; I < SafeObjectiveCount; ++I)
			{
				const FIntPoint Cell = PickCandidateCell(Candidates, Stream, Start, 4 + I * 2);
				if (Cell.X < 0) break;
				if (SpawnObjectiveAt(Cell, P, EPMEObjectType::Standard, ObjectiveId++))
				{
					if (P == 1) ++Player1Required;
					else ++Player2Required;
				}
			}
		}
	}
	else
	{
		SharedRequired = 0;
		const FIntPoint Start = Maze->GetPlayerStartCell();
		for (int32 I = 0; I < SafeObjectiveCount; ++I)
		{
			const FIntPoint Cell = PickCandidateCell(Candidates, Stream, Start, 4 + I * 2);
			if (Cell.X < 0) break;
			const EPMEObjectType Type = (PlayMode == EPMEPlayMode::Coop && I == SafeObjectiveCount - 1 &&
				                            SafeObjectiveCount > 1)
				                            ? EPMEObjectType::Synchronized
				                            : EPMEObjectType::Standard;
			if (SpawnObjectiveAt(Cell, 0, Type, ObjectiveId++)) ++SharedRequired;
		}
	}

	static const EPMEPickupType Types[] = {
		EPMEPickupType::Torch, EPMEPickupType::MapPulse, EPMEPickupType::Decoy, EPMEPickupType::LightBomb,
		EPMEPickupType::MasterKey
	};
	for (int32 I = 0; I < PickupCount && Candidates.Num() > 0; ++I)
	{
		const FIntPoint Cell = PickCandidateCell(Candidates, Stream, Maze->GetPlayerStartCell(), 3);
		if (Cell.X < 0) break;
		const EPMEPickupType Type = Types[Stream.RandRange(0,UE_ARRAY_COUNT(Types) - 1)];
		SpawnPickupAt(Cell, Type, 0);
	}
	RefreshReplicatedProgress();
	if (AreAllRequiredObjectivesComplete())
	{
		if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>()) GM->HandleAllObjectivesCompleted();
	}
}

TArray<FIntPoint> APMEObjectManager::BuildCandidateCells() const
{
	TArray<FIntPoint> Result;
	if (!Maze) return Result;
	const TArray<FIntPoint> Protected = {
		Maze->GetPlayerStartCell(), Maze->GetVersusPlayerStartCell(1), Maze->GetVersusPlayerStartCell(2),
		Maze->GetExitCell()
	};
	for (int32 Y = 1; Y < Maze->GetGridHeight() - 1; ++Y)
		for (int32 X = 1; X < Maze->GetGridWidth() - 1; ++X)
		{
			if (!Maze->IsWalkable(X, Y)) continue;
			const FIntPoint C(X, Y);
			bool bSafe = true;
			for (const FIntPoint& P : Protected)
				if (FMath::Abs(C.X - P.X) + FMath::Abs(C.Y - P.Y) < 3)
				{
					bSafe = false;
					break;
				}
			if (bSafe) Result.Add(C);
		}
	return Result;
}

FIntPoint APMEObjectManager::PickCandidateCell(TArray<FIntPoint>& Candidates, FRandomStream& Stream,
                                               FIntPoint Reference, int32 MinimumDistance) const
{
	TArray<int32> Valid;
	for (int32 I = 0; I < Candidates.Num(); ++I)
		if (FMath::Abs(Candidates[I].X - Reference.X) +
			FMath::Abs(Candidates[I].Y - Reference.Y) >= MinimumDistance)
			Valid.Add(I);
	if (Valid.Num() == 0) return FIntPoint(-1, -1);
	const int32 Pick = Valid[Stream.RandRange(0, Valid.Num() - 1)];
	const FIntPoint Cell = Candidates[Pick];
	Candidates.RemoveAtSwap(Pick);
	return Cell;
}

bool APMEObjectManager::SpawnObjectiveAt(FIntPoint Cell, int32 ID, EPMEObjectType Type, int32 Id)
{
	if (!GetWorld() || !Maze || !ObjectiveClass) return false;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (APMEObjectActor* O = GetWorld()->SpawnActor<APMEObjectActor>(ObjectiveClass,
	                                                                 Maze->GridToWorld(Cell.X, Cell.Y) + FVector(
		                                                                 0, 0, 55), FRotator::ZeroRotator, P))
	{
		O->InitializeObjective(this, Id, ID, Type);
		Objectives.Add(O);
		return true;
	}
	return false;
}

void APMEObjectManager::SpawnPickupAt(FIntPoint Cell, EPMEPickupType Type, int32 ID)
{
	if (!GetWorld() || !Maze) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (APMEPickup* O = GetWorld()->SpawnActor<APMEPickup>(PickupClass,
	                                                       Maze->GridToWorld(Cell.X, Cell.Y) + FVector(0, 0, 38),
	                                                       FRotator::ZeroRotator, P))
	{
		O->InitializePickup(Type, ID);
		Pickups.Add(O);
	}
}

void APMEObjectManager::HandleObjectiveActivated(APMEObjectActor* Objective, APMEPlayerState* Player)
{
	if (!HasAuthority() || !Objective || !Player) return;
	if (PlayMode == EPMEPlayMode::Versus)
	{
		if (Objective->GetOwnerPlayerIndex() == 1)++Player1Activated;
		else if (Objective->GetOwnerPlayerIndex() == 2)++Player2Activated;
	}
	else ++SharedActivated;
	Player->RecordObjective();
	RefreshReplicatedProgress();
	if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>())
	{
		GM->EmitNoise(Objective->GetActorLocation(), 9.0f, Objective,TEXT("Event.Noise.Objective"));
		GM->HandleObjectiveProgressChanged(Player);
		if (AreAllRequiredObjectivesComplete()) GM->HandleAllObjectivesCompleted();
	}
}

bool APMEObjectManager::CompleteOneObjectiveWithMasterKey(APMEPlayerState* Player)
{
	if (!HasAuthority() || !Player) return false;
	for (APMEObjectActor* Objective : Objectives)
	{
		if (Objective && !Objective->IsActivated() && (Objective->GetOwnerPlayerIndex() == 0 || Objective->
			GetOwnerPlayerIndex() == Player->GetPlayerIndex()))
		{
			Objective->ActivateObjective(Player);
			return true;
		}
	}
	return false;
}

int32 APMEObjectManager::GetRequiredObjectivesForPlayer(const APMEPlayerState* PS) const
{
	if (PlayMode != EPMEPlayMode::Versus)return SharedRequired;
	return PS && PS->GetPlayerIndex() == 2 ? Player2Required : Player1Required;
}

int32 APMEObjectManager::GetActivatedObjectivesForPlayer(const APMEPlayerState* PS) const
{
	if (PlayMode != EPMEPlayMode::Versus)return SharedActivated;
	return PS && PS->GetPlayerIndex() == 2 ? Player2Activated : Player1Activated;
}

bool APMEObjectManager::CanPlayerUseExit(const APMEPlayerState* PS) const
{
	return PS && GetActivatedObjectivesForPlayer(PS) >= GetRequiredObjectivesForPlayer(PS);
}

bool APMEObjectManager::AreAllRequiredObjectivesComplete() const
{
	return PlayMode == EPMEPlayMode::Versus
		       ? (Player1Activated >= Player1Required && Player2Activated >= Player2Required)
		       : (SharedActivated >= SharedRequired);
}

void APMEObjectManager::RefreshReplicatedProgress()
{
	ForceNetUpdate();
	if (APMEGameState* GS = GetWorld() ? GetWorld()->GetGameState<APMEGameState>() : nullptr)
		GS->SetObjectiveProgress(
			SharedActivated, SharedRequired, Player1Activated, Player1Required, Player2Activated, Player2Required);
}
