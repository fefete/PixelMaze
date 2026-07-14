#include "PMEGameModeBase.h"

#include "AbilitySystemComponent.h"

#include "Algo/Reverse.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Containers/Queue.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PMEAttributeSet.h"
#include "PMEDecoyActor.h"
#include "PMEExitActor.h"
#include "PMEGameplayEffects.h"
#include "PMEGameInstance.h"
#include "PMEGameState.h"
#include "PMEInventoryComponent.h"
#include "PMEHUD.h"
#include "PMELocalizationLibrary.h"
#include "PMEMazeGenerator.h"
#include "PMEObjectManager.h"
#include "PMEPatrolEnemy.h"
#include "PMEPlayerCharacter.h"
#include "PMEPlayerController.h"
#include "PMEPlayerState.h"
#include "PMERunSubsystem.h"
#include "TimerManager.h"

APMEGameModeBase::APMEGameModeBase()
{
	DefaultPawnClass = APMEPlayerCharacter::StaticClass();
	PlayerControllerClass = APMEPlayerController::StaticClass();
	PlayerStateClass = APMEPlayerState::StaticClass();
	GameStateClass = APMEGameState::StaticClass();
	HUDClass = APMEHUD::StaticClass();

	MazeGeneratorClass = APMEMazeGenerator::StaticClass();
	ExitActorClass = APMEExitActor::StaticClass();
	PatrolEnemyClass = APMEPatrolEnemy::StaticClass();
	ObjectiveManagerClass = APMEObjectManager::StaticClass();
	DecoyClass = APMEDecoyActor::StaticClass();
	bUseSeamlessTravel = false;
}

void APMEGameModeBase::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	const FString ModeOption = UGameplayStatics::ParseOption(Options, TEXT("Mode"));
	PlayMode = PixelMazeMode::FromOptionString(ModeOption);

	Super::InitGame(MapName, Options, ErrorMessage);
}

void APMEGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	EnsureWorldLighting();
	if (const UPMEGameInstance* GI = GetGameInstance<UPMEGameInstance>()) CurrentLevel =
		FMath::Max(1, GI->CurrentLevel);
	MazeGameState = GetGameState<APMEGameState>();

	if (MazeGameState)
	{
		MazeGameState->ConfigureMatch(PlayMode);
	}

	if (!MazeGenerator && MazeGeneratorClass)
	{
		MazeGenerator = GetWorld()->SpawnActor<APMEMazeGenerator>(MazeGeneratorClass, FTransform::Identity);
	}

	if (!ExitActor && ExitActorClass)
	{
		ExitActor = GetWorld()->SpawnActor<APMEExitActor>(ExitActorClass, FTransform::Identity);
		if (ExitActor)
		{
			ExitActor->SetExitEnabled(false);
		}
	}

	if (PlayMode == EPMEPlayMode::SinglePlayer || GetConnectedPlayerCount() >= 2)
	{
		StartMaze(true);
	}
	else
	{
		EnterWaitingForPlayers();
	}
}

void APMEGameModeBase::PreLogin(
	const FString& Options,
	const FString& Address,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	const FString RequestedModeOption = UGameplayStatics::ParseOption(Options, TEXT("RequestedMode"));
	if (!RequestedModeOption.IsEmpty() &&
		PixelMazeMode::FromOptionString(RequestedModeOption) != PlayMode)
	{
		ErrorMessage = TEXT("ModeMismatch");
		return;
	}

	if (GetConnectedPlayerCount() >= 2)
	{
		ErrorMessage = TEXT("ServerFull");
	}
}

void APMEGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (APMEPlayerState* NewPlayerState = NewPlayer ? NewPlayer->GetPlayerState<APMEPlayerState>() : nullptr)
	{
		AssignPlayerIndex(NewPlayerState);
	}

	EnsurePlayerPawn(NewPlayer);

	if (PlayMode == EPMEPlayMode::SinglePlayer)
	{
		PlaceAllPlayersAtStart();
		return;
	}

	if (GetConnectedPlayerCount() >= 2)
	{
		StartMaze(true);
	}
	else
	{
		EnterWaitingForPlayers();
	}
}

void APMEGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (PlayMode != EPMEPlayMode::SinglePlayer)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &APMEGameModeBase::EnterWaitingForPlayers);
	}
}

void APMEGameModeBase::StartMaze(const bool bUseNewSeed)
{
	if (!MazeGenerator || !ExitActor || !MazeGameState) return;
	if (PlayMode != EPMEPlayMode::SinglePlayer && GetConnectedPlayerCount() < 2)
	{
		EnterWaitingForPlayers();
		return;
	}

	bRoundTransitionPending = false;
	PlayersWithUpgradeSelection.Reset();
	GetWorldTimerManager().ClearTimer(TransitionTimer);
	DestroyPatrolEnemies();
	for (APMEDecoyActor* Decoy : ActiveDecoys) if (IsValid(Decoy)) Decoy->Destroy();
	ActiveDecoys.Reset();
	if (ObjectiveManager)
	{
		ObjectiveManager->Destroy();
		ObjectiveManager = nullptr;
	}
	LastEnemyContactTimes.Reset();

	const int32 Seed = bUseNewSeed ? 0 : MazeGenerator->GetActiveSeed();
	MazeGenerator->GenerateMaze(CurrentLevel, Seed);
	CurrentModifier = SelectLevelModifier(CurrentLevel);
	ExitActor->ResetExit(MazeGenerator->GetExitWorldLocation());
	ExitActor->SetExitEnabled(false);

	for (APlayerState* BasePlayerState : MazeGameState->PlayerArray)
	{
		if (APMEPlayerState* PS = Cast<APMEPlayerState>(BasePlayerState))
		{
			PS->ResetForRound();
			if (PS->GetInventoryComponent()) PS->GetInventoryComponent()->ClearInventory();
			if (PS->GetUpgradeStack(EPMEUpgradeType::ExtraStartingItem) > 0 && PS->GetInventoryComponent())
			{
				PS->GetInventoryComponent()->AddItem(EPMEPickupType::Torch, 1);
			}
		}
	}

	MazeGameState->BeginRound(CurrentLevel, MazeGenerator->GetActiveSeed(), CurrentModifier);
	PlaceAllPlayersAtStart();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TSubclassOf<APMEObjectManager> ManagerClass;
	if (ObjectiveManagerClass)
	{
		ManagerClass = ObjectiveManagerClass;
	}
	else
	{
		ManagerClass = APMEObjectManager::StaticClass();
	}
	ObjectiveManager = GetWorld()->SpawnActor<APMEObjectManager>(ManagerClass, FTransform::Identity, SpawnParameters);
	if (ObjectiveManager)
	{
		const int32 ObjectiveCount = FMath::Clamp(BaseObjectiveCount + (CurrentLevel - 1) / 2, 1, 4);
		const int32 PickupCount = FMath::Clamp(BasePickupCount + CurrentLevel / 2, 1, 8);
		ObjectiveManager->InitializeObjectives(MazeGenerator, PlayMode, CurrentLevel, MazeGenerator->GetActiveSeed(),
		                                       ObjectiveCount, PickupCount);
	}

	SpawnPatrolEnemies();
	SetAllPlayersMoveIgnored(false);
	SetPatrolEnemiesEnabled(true);
}

void APMEGameModeBase::HandlePlayerReachedExit(APMEPlayerCharacter* PlayerCharacter)
{
	if (!HasAuthority() || !PlayerCharacter || !MazeGameState || MazeGameState->IsWaitingForPlayers() || MazeGameState->
		IsRoundFinished()) return;
	APMEPlayerState* ReachingPS = PlayerCharacter->GetPlayerState<APMEPlayerState>();
	if (!ReachingPS || ReachingPS->HasReachedExit() || ReachingPS->IsDowned()) return;
	if (ObjectiveManager && !ObjectiveManager->CanPlayerUseExit(ReachingPS)) return;

	const float CompletionTime = MazeGameState->GetElapsedRoundTime();
	ReachingPS->MarkReachedExit(CompletionTime);
	const int32 TimeScore = FMath::Max(0, 1500 - FMath::RoundToInt(CompletionTime * 12.0f));
	ReachingPS->AddScore(500 + TimeScore);

	if (PlayMode == EPMEPlayMode::Coop)
	{
		if (AController* Controller = PlayerCharacter->GetController()) Controller->SetIgnoreMoveInput(true);
		PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
		PlayerCharacter->GetCharacterMovement()->DisableMovement();
		if (AreAllCoopPlayersAtExit()) FinishRound(nullptr, CompletionTime, false);
		return;
	}
	if (PlayMode == EPMEPlayMode::Versus) ReachingPS->AddWin();
	FinishRound(ReachingPS, CompletionTime, false);
}

void APMEGameModeBase::HandlePlayerCaughtByEnemy(APMEPlayerCharacter* PlayerCharacter)
{
	if (!HasAuthority() || !PlayerCharacter || !MazeGenerator || !MazeGameState || MazeGameState->IsWaitingForPlayers()
		|| MazeGameState->IsRoundFinished()) return;
	APMEPlayerState* PS = PlayerCharacter->GetPlayerState<APMEPlayerState>();
	if (!PS || PS->HasReachedExit() || PS->IsDowned()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	const TWeakObjectPtr<APMEPlayerCharacter> Key(PlayerCharacter);
	if (const float* Last = LastEnemyContactTimes.Find(Key)) if (Now - *Last < EnemyContactResetCooldown) return;
	LastEnemyContactTimes.Add(Key, Now);

	if (APMEPlayerController* PC = Cast<APMEPlayerController>(PlayerCharacter->GetController())) PC->
		ClientPlayEnemyHitSound();
	PS->RecordHit();
	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		const UGameplayEffect* Damage = UPMEGE_DamageOne::StaticClass()->GetDefaultObject<UGameplayEffect>();
		ASC->ApplyGameplayEffectToSelf(Damage, 1.0f, ASC->MakeEffectContext());
	}

	const float Health = PS->GetAbilitySystemComponent()
		                     ? PS->GetAbilitySystemComponent()->GetNumericAttribute(
			                     UPMEAttributeSet::GetHealthAttribute())
		                     : 0.0f;

	if (Health <= 0.0f)
	{
		if (PlayMode == EPMEPlayMode::Coop)
		{
			PS->SetDowned(true);
			PlayerCharacter->ApplyDownedState(true);
			if (AreAllPlayersDowned()) FinishRound(nullptr, MazeGameState->GetElapsedRoundTime(), true);
			return;
		}
		if (PlayMode == EPMEPlayMode::Versus)
		{
			APMEPlayerState* Other = FindOtherActivePlayer(PS);
			if (Other) Other->AddWin();
			FinishRound(Other, MazeGameState->GetElapsedRoundTime(), Other == nullptr);
			return;
		}
		FinishRound(nullptr, MazeGameState->GetElapsedRoundTime(), true);
		return;
	}

	const int32 PlayerIndex = FMath::Clamp(PS->GetPlayerIndex(), 1, 2);
	const FVector RespawnLocation = PlayMode == EPMEPlayMode::Versus
		                                ? MazeGenerator->GetVersusPlayerStartWorldLocation(PlayerIndex)
		                                : MazeGenerator->GetPlayerStartWorldLocation();
	if (APMEPlayerController* PC = Cast<APMEPlayerController>(PlayerCharacter->GetController())) PC->
		ResetIgnoreMoveInput();
	PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
	PlayerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	PlayerCharacter->SetActorLocation(RespawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerCharacter->SetActorRotation(FRotator::ZeroRotator);
	PlayerCharacter->ForceNetUpdate();
}

void APMEGameModeBase::FinishRound(APMEPlayerState* WinnerPlayerState, const float CompletionTime, const bool bDefeat)
{
	if (bRoundTransitionPending || !MazeGameState) return;
	bRoundTransitionPending = true;
	SetAllPlayersMoveIgnored(true);
	SetPatrolEnemiesEnabled(false);
	if (ExitActor) ExitActor->SetExitEnabled(false);

	int32 WinnerIndex = bDefeat ? -1 : 0;
	FString WinnerName;
	if (WinnerPlayerState)
	{
		WinnerIndex = WinnerPlayerState->GetPlayerIndex();
		WinnerName = WinnerPlayerState->GetPlayerName();
	}
	else if (!bDefeat && PlayMode == EPMEPlayMode::Coop) WinnerName =
		UPMELocalizationLibrary::GetText(TEXT("Game.Team")).ToString();

	if (UPMEGameInstance* GI = GetGameInstance<UPMEGameInstance>())
	{
		if (!bDefeat) GI->RegisterCompletion(CompletionTime);
		MazeGameState->SetBestTime(GI->BestCompletionTime);
	}
	MazeGameState->FinishRound(CompletionTime, WinnerIndex, WinnerName);

	if (bDefeat || CurrentLevel >= MaximumRunLevels)
	{
		GetWorldTimerManager().SetTimer(TransitionTimer, this, &APMEGameModeBase::FinishRun, TransitionDelay, false);
	}
	else
	{
		GetWorldTimerManager().SetTimer(TransitionTimer, this, &APMEGameModeBase::BeginUpgradeSelection,
		                                TransitionDelay, false);
	}
}

void APMEGameModeBase::AdvanceToNextMaze()
{
	if (UPMEGameInstance* GI = GetGameInstance<UPMEGameInstance>())
	{
		GI->AdvanceLevel();
		CurrentLevel = GI->CurrentLevel;
	}
	else
	{
		++CurrentLevel;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		if (APMEPlayerController* PC = Cast<APMEPlayerController>(It->Get())) PC->ClientHideUpgradeChoices();
	StartMaze(true);
}

void APMEGameModeBase::RestartCurrentMaze()
{
	if (!HasAuthority() || bRoundTransitionPending)
	{
		return;
	}

	StartMaze(true);
}

void APMEGameModeBase::EnterWaitingForPlayers()
{
	if (PlayMode == EPMEPlayMode::SinglePlayer || !MazeGameState)
	{
		return;
	}

	bRoundTransitionPending = false;
	GetWorldTimerManager().ClearTimer(TransitionTimer);
	DestroyPatrolEnemies();
	LastEnemyContactTimes.Reset();
	MazeGameState->SetWaitingForPlayers(true);
	SetAllPlayersMoveIgnored(true);

	if (ExitActor)
	{
		ExitActor->SetExitEnabled(false);
	}
}

void APMEGameModeBase::PlaceAllPlayersAtStart()
{
	if (!MazeGenerator || MazeGenerator->GetGridWidth() <= 0)
	{
		return;
	}

	bool bNeedsRetry = false;

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!PlayerController)
		{
			continue;
		}

		APMEPlayerState* MazePlayerState = PlayerController->GetPlayerState<APMEPlayerState>();
		if (MazePlayerState && MazePlayerState->GetPlayerIndex() <= 0)
		{
			AssignPlayerIndex(MazePlayerState);
		}

		const int32 PlayerIndex = MazePlayerState
			                          ? FMath::Clamp(MazePlayerState->GetPlayerIndex(), 1, 2)
			                          : 1;

		const FVector StartLocation = PlayMode == EPMEPlayMode::Versus
			                              ? MazeGenerator->GetVersusPlayerStartWorldLocation(PlayerIndex)
			                              : MazeGenerator->GetPlayerStartWorldLocation();

		APawn* Pawn = PlayerController->GetPawn();
		if (!Pawn)
		{
			RestartPlayerAtTransform(PlayerController, FTransform(FRotator::ZeroRotator, StartLocation));
			Pawn = PlayerController->GetPawn();
		}

		if (!Pawn)
		{
			bNeedsRetry = true;
			continue;
		}

		Pawn->SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);
		Pawn->SetActorRotation(FRotator::ZeroRotator);
	}

	if (bNeedsRetry)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &APMEGameModeBase::PlaceAllPlayersAtStart);
	}
}

void APMEGameModeBase::EnsurePlayerPawn(APlayerController* PlayerController)
{
	if (!PlayerController || PlayerController->GetPawn())
	{
		return;
	}

	FVector SpawnLocation(0.0f, 0.0f, 100.0f);
	if (MazeGenerator && MazeGenerator->GetGridWidth() > 0)
	{
		const APMEPlayerState* MazePlayerState = PlayerController->GetPlayerState<APMEPlayerState>();
		const int32 PlayerIndex = MazePlayerState
			                          ? FMath::Clamp(MazePlayerState->GetPlayerIndex(), 1, 2)
			                          : 1;

		SpawnLocation = PlayMode == EPMEPlayMode::Versus
			                ? MazeGenerator->GetVersusPlayerStartWorldLocation(PlayerIndex)
			                : MazeGenerator->GetPlayerStartWorldLocation();
	}

	RestartPlayerAtTransform(
		PlayerController,
		FTransform(FRotator::ZeroRotator, SpawnLocation));
}

void APMEGameModeBase::SetAllPlayersMoveIgnored(const bool bIgnored)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (APlayerController* PlayerController = Iterator->Get())
		{
			PlayerController->ResetIgnoreMoveInput();
			if (bIgnored)
			{
				PlayerController->SetIgnoreMoveInput(true);
			}

			if (APMEPlayerCharacter* Character = Cast<APMEPlayerCharacter>(PlayerController->GetPawn()))
			{
				Character->GetCharacterMovement()->SetMovementMode(
					bIgnored ? MOVE_None : MOVE_Walking);
			}
		}
	}
}

void APMEGameModeBase::AssignPlayerIndex(APMEPlayerState* NewPlayerState)
{
	if (!NewPlayerState || !MazeGameState)
	{
		return;
	}

	bool bPlayerOneUsed = false;
	bool bPlayerTwoUsed = false;

	for (APlayerState* BasePlayerState : MazeGameState->PlayerArray)
	{
		const APMEPlayerState* ExistingPlayerState = Cast<APMEPlayerState>(BasePlayerState);
		if (!ExistingPlayerState || ExistingPlayerState == NewPlayerState)
		{
			continue;
		}

		bPlayerOneUsed |= ExistingPlayerState->GetPlayerIndex() == 1;
		bPlayerTwoUsed |= ExistingPlayerState->GetPlayerIndex() == 2;
	}

	NewPlayerState->AssignPlayerIndex(!bPlayerOneUsed ? 1 : (!bPlayerTwoUsed ? 2 : 2));
}

void APMEGameModeBase::SpawnPatrolEnemies()
{
	DestroyPatrolEnemies();

	if (!HasAuthority() || !bSpawnPatrolEnemies || !PatrolEnemyClass || !MazeGenerator ||
		MazeGenerator->GetGridWidth() <= 0 || MaximumEnemyCount <= 0)
	{
		return;
	}

	const int32 ModifierEnemyBonus = CurrentModifier == EPMELevelModifier::Hunters ? 2 : 0;
	const int32 RequestedEnemyCount = FMath::Clamp(
		BaseEnemyCount + FMath::Max(0, CurrentLevel - 1) * EnemiesAddedPerLevel + ModifierEnemyBonus,
		0,
		MaximumEnemyCount);

	if (RequestedEnemyCount <= 0)
	{
		return;
	}

	const TArray<FIntPoint> ProtectedCells = GetEnemyProtectedCells();

	// Exact start-to-exit routes are transit-only rather than permanently
	// forbidden. Patrols may cross these cells, but their route must start and
	// end outside them. The enemy actor also skips pauses and perception while
	// crossing, so the mandatory corridor always reopens automatically.
	const TSet<FIntPoint> TransitOnlyPatrolCells = BuildTransitOnlyPatrolCells();
	const TSet<FIntPoint> HardForbiddenPatrolCells =
		BuildHardForbiddenPatrolCells(TransitOnlyPatrolCells);

	TArray<FIntPoint> CandidateCells;
	CandidateCells.Reserve(MazeGenerator->GetGridWidth() * MazeGenerator->GetGridHeight());

	for (int32 Y = 0; Y < MazeGenerator->GetGridHeight(); ++Y)
	{
		for (int32 X = 0; X < MazeGenerator->GetGridWidth(); ++X)
		{
			const FIntPoint Cell(X, Y);
			if (!MazeGenerator->IsWalkable(X, Y) ||
				HardForbiddenPatrolCells.Contains(Cell) ||
				TransitOnlyPatrolCells.Contains(Cell) ||
				IsCellInsideEnemySafeZone(Cell, ProtectedCells) ||
				CountWalkableNeighbours(Cell) < 2)
			{
				continue;
			}

			CandidateCells.Add(Cell);
		}
	}

	FRandomStream RandomStream(MazeGenerator->GetActiveSeed() ^ 0x4D415A45);
	for (int32 Index = CandidateCells.Num() - 1; Index > 0; --Index)
	{
		CandidateCells.Swap(Index, RandomStream.RandRange(0, Index));
	}

	TArray<FIntPoint> SelectedSpawnCells;
	TSet<FIntPoint> OccupiedPatrolCells;
	const int32 RequiredSeparationSquared = FMath::Square(EnemyMinimumSeparationInTiles);

	for (const FIntPoint& Candidate : CandidateCells)
	{
		bool bTooCloseToAnotherEnemy = false;
		for (const FIntPoint& Selected : SelectedSpawnCells)
		{
			const int32 DeltaX = Candidate.X - Selected.X;
			const int32 DeltaY = Candidate.Y - Selected.Y;
			if (DeltaX * DeltaX + DeltaY * DeltaY < RequiredSeparationSquared)
			{
				bTooCloseToAnotherEnemy = true;
				break;
			}
		}

		if (bTooCloseToAnotherEnemy)
		{
			continue;
		}

		const TArray<FIntPoint> PatrolCells = BuildPatrolRoute(
			Candidate,
			RandomStream,
			HardForbiddenPatrolCells,
			TransitOnlyPatrolCells,
			OccupiedPatrolCells);
		if (PatrolCells.Num() < 2)
		{
			continue;
		}

		TArray<FVector> PatrolWorldPoints;
		TArray<uint8> TransitOnlyPointFlags;
		PatrolWorldPoints.Reserve(PatrolCells.Num());
		TransitOnlyPointFlags.Reserve(PatrolCells.Num());
		for (const FIntPoint& PatrolCell : PatrolCells)
		{
			PatrolWorldPoints.Add(
				MazeGenerator->GridToWorld(PatrolCell.X, PatrolCell.Y) + FVector(0.0f, 0.0f, 50.0f));
			TransitOnlyPointFlags.Add(TransitOnlyPatrolCells.Contains(PatrolCell) ? 1 : 0);
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APMEPatrolEnemy* Enemy = GetWorld()->SpawnActor<APMEPatrolEnemy>(
			PatrolEnemyClass,
			PatrolWorldPoints[0],
			FRotator::ZeroRotator,
			SpawnParameters);

		if (!Enemy)
		{
			continue;
		}

		EPMEEnemyArchetype Archetype = EPMEEnemyArchetype::Guardian;
		if (CurrentLevel >= 3 && PatrolEnemies.Num() % 3 == 1) Archetype = EPMEEnemyArchetype::Listener;
		if (CurrentLevel >= 4 && PatrolEnemies.Num() % 4 == 3) Archetype = EPMEEnemyArchetype::Stalker;
		const float FuryMultiplier = CurrentModifier == EPMELevelModifier::Fury ? 1.25f : 1.0f;

		Enemy->InitializePatrol(
			MazeGenerator,
			PatrolWorldPoints,
			TransitOnlyPointFlags,
			TransitOnlyPatrolCells,
			EnemyMovementSpeed * FuryMultiplier,
			EnemyWaypointPauseDuration,
			EnemySightRadiusInTiles,
			EnemyChaseDistanceInTiles * FuryMultiplier,
			EnemyChaseMovementSpeed * FuryMultiplier,
			EnemyReturnMovementSpeed * FuryMultiplier,
			EnemySightCheckInterval,
			EnemyChasePathRefreshInterval,
			EnemyHearingRadiusInTiles,
			EnemyInvestigationDuration,
			Archetype);
		PatrolEnemies.Add(Enemy);
		SelectedSpawnCells.Add(Candidate);

		if (bPreventPatrolRouteOverlap)
		{
			for (const FIntPoint& PatrolCell : PatrolCells)
			{
				OccupiedPatrolCells.Add(PatrolCell);
			}
		}

		if (PatrolEnemies.Num() >= RequestedEnemyCount)
		{
			break;
		}
	}

	// Safety takes precedence over enemy count. Some small or very linear
	// mazes may have too few side branches, so fewer enemies are spawned rather
	// than placing one on the only route to the finish.
	if (PatrolEnemies.Num() < RequestedEnemyCount)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Pixel Maze: spawned %d/%d enemies because no safe transit patrol route was available."),
			PatrolEnemies.Num(),
			RequestedEnemyCount);
	}
}

void APMEGameModeBase::DestroyPatrolEnemies()
{
	for (APMEPatrolEnemy* Enemy : PatrolEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
		}
	}

	PatrolEnemies.Reset();
}

void APMEGameModeBase::SetPatrolEnemiesEnabled(const bool bEnabled)
{
	for (APMEPatrolEnemy* Enemy : PatrolEnemies)
	{
		if (IsValid(Enemy))
		{
			Enemy->SetPatrolEnabled(bEnabled);
		}
	}
}

TArray<FIntPoint> APMEGameModeBase::BuildPatrolRoute(
	const FIntPoint StartCell,
	FRandomStream& RandomStream,
	const TSet<FIntPoint>& HardForbiddenPatrolCells,
	const TSet<FIntPoint>& TransitOnlyPatrolCells,
	const TSet<FIntPoint>& OccupiedPatrolCells) const
{
	TArray<FIntPoint> EmptyRoute;
	if (!MazeGenerator ||
		!MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
		HardForbiddenPatrolCells.Contains(StartCell) ||
		TransitOnlyPatrolCells.Contains(StartCell) ||
		(bPreventPatrolRouteOverlap && OccupiedPatrolCells.Contains(StartCell)))
	{
		return EmptyRoute;
	}

	const TArray<FIntPoint> ProtectedCells = GetEnemyProtectedCells();
	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	const int32 DesiredRouteLength = FMath::Max(2, EnemyPatrolRouteLengthInTiles);
	const int32 MaximumSearchNodes = FMath::Max(256, DesiredRouteLength * 256);
	int32 RemainingSearchNodes = MaximumSearchNodes;

	TArray<FIntPoint> CurrentRoute;
	TArray<FIntPoint> BestRoute;
	TSet<FIntPoint> VisitedCells;
	CurrentRoute.Reserve(DesiredRouteLength);
	CurrentRoute.Add(StartCell);
	VisitedCells.Add(StartCell);

	TFunction<void(const FIntPoint&)> SearchRoute;
	SearchRoute = [&](const FIntPoint& Current)
	{
		if (RemainingSearchNodes-- <= 0 || BestRoute.Num() >= DesiredRouteLength)
		{
			return;
		}

		// Both patrol endpoints must be outside the mandatory route. Therefore
		// every contiguous transit-only section is automatically bounded by a
		// normal cell on entry and exit, and can never become a reversal point.
		if (CurrentRoute.Num() >= 2 && !TransitOnlyPatrolCells.Contains(Current))
		{
			if (CurrentRoute.Num() > BestRoute.Num())
			{
				BestRoute = CurrentRoute;
			}

			if (BestRoute.Num() >= DesiredRouteLength)
			{
				return;
			}
		}

		if (CurrentRoute.Num() >= DesiredRouteLength)
		{
			return;
		}

		TArray<FIntPoint, TInlineAllocator<4>> Candidates;
		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Next = Current + Direction;
			if (!MazeGenerator->IsWalkable(Next.X, Next.Y) ||
				HardForbiddenPatrolCells.Contains(Next) ||
				VisitedCells.Contains(Next) ||
				(bPreventPatrolRouteOverlap && OccupiedPatrolCells.Contains(Next)) ||
				IsCellInsideEnemySafeZone(Next, ProtectedCells))
			{
				continue;
			}

			Candidates.Add(Next);
		}

		// Deterministic Fisher-Yates shuffle. Transit cells are legal internal
		// points, but never legal endpoints because only non-transit states are
		// copied into BestRoute.
		for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
		{
			Candidates.Swap(Index, RandomStream.RandRange(0, Index));
		}

		// Prefer trying a crossing first when one is available. The random
		// order within each group remains deterministic for the maze seed.
		Candidates.StableSort([&TransitOnlyPatrolCells](const FIntPoint& A, const FIntPoint& B)
		{
			return TransitOnlyPatrolCells.Contains(A) && !TransitOnlyPatrolCells.Contains(B);
		});

		for (const FIntPoint& Next : Candidates)
		{
			CurrentRoute.Add(Next);
			VisitedCells.Add(Next);
			SearchRoute(Next);
			VisitedCells.Remove(Next);
			CurrentRoute.Pop(EAllowShrinking::No);

			if (BestRoute.Num() >= DesiredRouteLength || RemainingSearchNodes <= 0)
			{
				break;
			}
		}
	};

	SearchRoute(StartCell);
	return BestRoute;
}

TSet<FIntPoint> APMEGameModeBase::BuildTransitOnlyPatrolCells() const
{
	TSet<FIntPoint> TransitCells;
	if (!MazeGenerator)
	{
		return TransitCells;
	}

	const FIntPoint ExitCell = MazeGenerator->GetExitCell();
	TArray<FIntPoint> ProgressPath;

	if (PlayMode == EPMEPlayMode::Versus)
	{
		for (int32 PlayerIndex = 1; PlayerIndex <= 2; ++PlayerIndex)
		{
			ProgressPath.Reset();
			if (BuildWalkableGridPath(
				MazeGenerator->GetVersusPlayerStartCell(PlayerIndex),
				ExitCell,
				ProgressPath))
			{
				AddPathToCellSet(ProgressPath, TransitCells);
			}
		}
	}
	else
	{
		if (BuildWalkableGridPath(
			MazeGenerator->GetPlayerStartCell(),
			ExitCell,
			ProgressPath))
		{
			AddPathToCellSet(ProgressPath, TransitCells);
		}
	}

	return TransitCells;
}

bool APMEGameModeBase::BuildWalkableGridPath(
	const FIntPoint StartCell,
	const FIntPoint GoalCell,
	TArray<FIntPoint>& OutPath) const
{
	OutPath.Reset();

	if (!MazeGenerator ||
		!MazeGenerator->IsWalkable(StartCell.X, StartCell.Y) ||
		!MazeGenerator->IsWalkable(GoalCell.X, GoalCell.Y))
	{
		return false;
	}

	const int32 GridWidth = MazeGenerator->GetGridWidth();
	const int32 GridHeight = MazeGenerator->GetGridHeight();
	const int32 GridCellCount = GridWidth * GridHeight;
	if (GridWidth <= 0 || GridHeight <= 0 || GridCellCount <= 0)
	{
		return false;
	}

	auto ToIndex = [GridWidth](const FIntPoint Cell)
	{
		return Cell.Y * GridWidth + Cell.X;
	};

	TArray<int32> ParentIndices;
	ParentIndices.Init(INDEX_NONE, GridCellCount);

	TArray<uint8> Visited;
	Visited.Init(0, GridCellCount);

	TQueue<FIntPoint> Pending;
	Pending.Enqueue(StartCell);
	Visited[ToIndex(StartCell)] = 1;

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	FIntPoint Current;
	bool bFoundGoal = false;
	while (Pending.Dequeue(Current))
	{
		if (Current == GoalCell)
		{
			bFoundGoal = true;
			break;
		}

		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Next = Current + Direction;
			if (!MazeGenerator->IsWalkable(Next.X, Next.Y))
			{
				continue;
			}

			const int32 NextIndex = ToIndex(Next);
			if (!Visited.IsValidIndex(NextIndex) || Visited[NextIndex] != 0)
			{
				continue;
			}

			Visited[NextIndex] = 1;
			ParentIndices[NextIndex] = ToIndex(Current);
			Pending.Enqueue(Next);
		}
	}

	if (!bFoundGoal)
	{
		return false;
	}

	int32 CurrentIndex = ToIndex(GoalCell);
	const int32 StartIndex = ToIndex(StartCell);
	while (CurrentIndex != INDEX_NONE)
	{
		const int32 X = CurrentIndex % GridWidth;
		const int32 Y = CurrentIndex / GridWidth;
		OutPath.Add(FIntPoint(X, Y));

		if (CurrentIndex == StartIndex)
		{
			break;
		}

		if (!ParentIndices.IsValidIndex(CurrentIndex))
		{
			OutPath.Reset();
			return false;
		}
		CurrentIndex = ParentIndices[CurrentIndex];
	}

	if (OutPath.IsEmpty() || OutPath.Last() != StartCell)
	{
		OutPath.Reset();
		return false;
	}

	Algo::Reverse(OutPath);
	return true;
}

TSet<FIntPoint> APMEGameModeBase::BuildHardForbiddenPatrolCells(
	const TSet<FIntPoint>& TransitOnlyPatrolCells) const
{
	TSet<FIntPoint> ForbiddenCells;
	if (!MazeGenerator)
	{
		return ForbiddenCells;
	}

	if (!bAllowPatrolTransitAcrossProgressRoutes)
	{
		for (const FIntPoint& TransitCell : TransitOnlyPatrolCells)
		{
			ForbiddenCells.Add(TransitCell);
		}
		return ForbiddenCells;
	}

	const int32 Clearance = FMath::Max(0, EnemyProgressRouteClearanceInTiles);
	if (Clearance <= 0)
	{
		return ForbiddenCells;
	}

	for (const FIntPoint& PathCell : TransitOnlyPatrolCells)
	{
		for (int32 OffsetY = -Clearance; OffsetY <= Clearance; ++OffsetY)
		{
			for (int32 OffsetX = -Clearance; OffsetX <= Clearance; ++OffsetX)
			{
				if (FMath::Abs(OffsetX) + FMath::Abs(OffsetY) > Clearance)
				{
					continue;
				}

				const FIntPoint Candidate = PathCell + FIntPoint(OffsetX, OffsetY);
				if (Candidate != PathCell &&
					MazeGenerator->IsWalkable(Candidate.X, Candidate.Y) &&
					!TransitOnlyPatrolCells.Contains(Candidate))
				{
					ForbiddenCells.Add(Candidate);
				}
			}
		}
	}

	return ForbiddenCells;
}

void APMEGameModeBase::AddPathToCellSet(
	const TArray<FIntPoint>& Path,
	TSet<FIntPoint>& InOutCells) const
{
	for (const FIntPoint& PathCell : Path)
	{
		InOutCells.Add(PathCell);
	}
}

TArray<FIntPoint> APMEGameModeBase::GetEnemyProtectedCells() const
{
	TArray<FIntPoint> ProtectedCells;
	if (!MazeGenerator)
	{
		return ProtectedCells;
	}

	ProtectedCells.AddUnique(MazeGenerator->GetExitCell());

	if (PlayMode == EPMEPlayMode::Versus)
	{
		ProtectedCells.AddUnique(MazeGenerator->GetVersusPlayerStartCell(1));
		ProtectedCells.AddUnique(MazeGenerator->GetVersusPlayerStartCell(2));
	}
	else
	{
		ProtectedCells.AddUnique(MazeGenerator->GetPlayerStartCell());
	}

	return ProtectedCells;
}

bool APMEGameModeBase::IsCellInsideEnemySafeZone(
	const FIntPoint Cell,
	const TArray<FIntPoint>& ProtectedCells) const
{
	for (const FIntPoint& ProtectedCell : ProtectedCells)
	{
		const int32 ManhattanDistance =
			FMath::Abs(Cell.X - ProtectedCell.X) + FMath::Abs(Cell.Y - ProtectedCell.Y);
		if (ManhattanDistance <= EnemySafeZoneRadiusInTiles)
		{
			return true;
		}
	}

	return false;
}

int32 APMEGameModeBase::CountWalkableNeighbours(const FIntPoint Cell) const
{
	if (!MazeGenerator)
	{
		return 0;
	}

	const FIntPoint Directions[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	int32 Count = 0;
	for (const FIntPoint& Direction : Directions)
	{
		const FIntPoint Neighbour = Cell + Direction;
		Count += MazeGenerator->IsWalkable(Neighbour.X, Neighbour.Y) ? 1 : 0;
	}

	return Count;
}

bool APMEGameModeBase::AreAllCoopPlayersAtExit() const
{
	if (!MazeGameState || MazeGameState->PlayerArray.Num() < 2)
	{
		return false;
	}

	int32 ReachedCount = 0;
	for (APlayerState* BasePlayerState : MazeGameState->PlayerArray)
	{
		const APMEPlayerState* MazePlayerState = Cast<APMEPlayerState>(BasePlayerState);
		if (MazePlayerState && MazePlayerState->HasReachedExit())
		{
			++ReachedCount;
		}
	}

	return ReachedCount >= 2;
}

int32 APMEGameModeBase::GetConnectedPlayerCount()
{
	return MazeGameState ? MazeGameState->PlayerArray.Num() : GetNumPlayers();
}


void APMEGameModeBase::HandleObjectiveProgressChanged(APMEPlayerState* ActivatingPlayer)
{
	if (!ObjectiveManager || !MazeGameState || MazeGameState->IsRoundFinished()) return;

	const int32 Completed = ObjectiveManager->GetActivatedObjectivesForPlayer(ActivatingPlayer);
	const float ObjectiveAlertMultiplier = 1.0f + 0.08f * Completed;
	for (APMEPatrolEnemy* Enemy : PatrolEnemies)
	{
		if (Enemy) Enemy->SetAlertMultiplier(ObjectiveAlertMultiplier);
	}

	if (PlayMode == EPMEPlayMode::Versus && ObjectiveManager->CanPlayerUseExit(ActivatingPlayer)) EnterEscapePhase();
}

void APMEGameModeBase::HandleAllObjectivesCompleted()
{
	EnterEscapePhase();
}

void APMEGameModeBase::EnterEscapePhase()
{
	if (!MazeGameState || MazeGameState->GetRoundPhase() == EPMERoundPhase::Escape || MazeGameState->IsRoundFinished())
		return;
	MazeGameState->SetRoundPhase(EPMERoundPhase::Escape);
	if (ExitActor) ExitActor->SetExitEnabled(true);
	ApplyEscapeEnemyEscalation();
	if (MazeGenerator) EmitNoise(MazeGenerator->GetExitWorldLocation(), 16.0f, ExitActor,
	                             TEXT("Event.Noise.ExitUnlocked"));
}

void APMEGameModeBase::ApplyEscapeEnemyEscalation()
{
	for (APMEPatrolEnemy* Enemy : PatrolEnemies) if (Enemy) Enemy->SetAlertMultiplier(1.35f);
}

void APMEGameModeBase::EmitNoise(const FVector& Location, const float RadiusInTiles, AActor* Source,
                                 const FName NoiseTag)
{
	if (!HasAuthority() || !MazeGenerator) return;
	float Radius = RadiusInTiles;
	if (CurrentModifier == EPMELevelModifier::Echo || CurrentModifier == EPMELevelModifier::SensitiveHearing) Radius *=
		1.5f;
	for (APMEPatrolEnemy* Enemy : PatrolEnemies) if (Enemy) Enemy->HearNoise(Location, Radius, Source, NoiseTag);
}

void APMEGameModeBase::SpawnDecoy(APMEPlayerCharacter* SourceCharacter)
{
	if (!HasAuthority() || !SourceCharacter || !DecoyClass) return;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location = SourceCharacter->GetActorLocation() + SourceCharacter->GetActorForwardVector() * 75.0f;
	if (APMEDecoyActor* Decoy = GetWorld()->SpawnActor<APMEDecoyActor>(DecoyClass, Location, FRotator::ZeroRotator, P))
	{
		Decoy->InitializeDecoy(6.0f, 0.8f, 11.0f);
		ActiveDecoys.Add(Decoy);
	}
}

void APMEGameModeBase::StunEnemiesAround(const FVector& Location, const float RadiusInTiles, const float Duration)
{
	if (!HasAuthority() || !MazeGenerator) return;
	const float RadiusWorld = RadiusInTiles * MazeGenerator->GetTileSize();
	for (APMEPatrolEnemy* Enemy : PatrolEnemies)
		if (Enemy && FVector::DistSquared2D(Enemy->GetActorLocation(), Location) <= FMath::Square(RadiusWorld)) Enemy->
			ApplyStun(Duration);
	EmitNoise(Location, RadiusInTiles * 1.5f, nullptr, TEXT("Event.Noise.LightBomb"));
}

void APMEGameModeBase::UseMasterKey(APMEPlayerState* PlayerState)
{
	if (ObjectiveManager) ObjectiveManager->CompleteOneObjectiveWithMasterKey(PlayerState);
}

void APMEGameModeBase::TryReviveNearestTeammate(APMEPlayerCharacter* Reviver)
{
	if (!HasAuthority() || PlayMode != EPMEPlayMode::Coop || !Reviver || !MazeGenerator) return;
	APMEPlayerState* ReviverPS = Reviver->GetPlayerState<APMEPlayerState>();
	if (!ReviverPS || ReviverPS->IsDowned()) return;
	const float Radius = ReviveRadiusInTiles * MazeGenerator->GetTileSize();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APMEPlayerCharacter* Target = Cast<APMEPlayerCharacter>(It->Get() ? It->Get()->GetPawn() : nullptr);
		APMEPlayerState* TargetPS = Target ? Target->GetPlayerState<APMEPlayerState>() : nullptr;
		if (!Target || !TargetPS || TargetPS == ReviverPS || !TargetPS->IsDowned()) continue;
		if (FVector::DistSquared2D(Target->GetActorLocation(), Reviver->GetActorLocation()) > FMath::Square(Radius))
			continue;
		TargetPS->SetDowned(false);
		if (UAbilitySystemComponent* ASC = TargetPS->GetAbilitySystemComponent())
			ASC->SetNumericAttributeBase(UPMEAttributeSet::GetHealthAttribute(),
			                             FMath::Max(1.0f, ASC->GetNumericAttribute(
				                                        UPMEAttributeSet::GetMaxHealthAttribute()) * 0.5f));
		Target->ApplyDownedState(false);
		ReviverPS->AddScore(300);
		EmitNoise(Target->GetActorLocation(), 5.0f, Reviver, TEXT("Event.Noise.Revive"));
		return;
	}
}

void APMEGameModeBase::BeginUpgradeSelection()
{
	if (!MazeGameState) return;
	PlayersWithUpgradeSelection.Reset();
	MazeGameState->SetRoundPhase(EPMERoundPhase::UpgradeSelection);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APMEPlayerController* PC = Cast<APMEPlayerController>(It->Get());
		APMEPlayerState* PS = PC ? PC->GetPlayerState<APMEPlayerState>() : nullptr;
		if (PC && PS) PC->ClientShowUpgradeChoices(BuildUpgradeChoices(PS));
	}
	GetWorldTimerManager().SetTimer(TransitionTimer, this, &APMEGameModeBase::ResolveUpgradeTimeout,
	                                FMath::Max(0.1f, UpgradeSelectionTimeout), false);
}

TArray<EPMEUpgradeType> APMEGameModeBase::BuildUpgradeChoices(const APMEPlayerState* PlayerState) const
{
	TArray<EPMEUpgradeType> Pool = {
		EPMEUpgradeType::WiderVision, EPMEUpgradeType::QuietSteps, EPMEUpgradeType::StrongHeart,
		EPMEUpgradeType::FasterMovement, EPMEUpgradeType::BetterSprint, EPMEUpgradeType::LongerTorch,
		EPMEUpgradeType::BiggerMapPulse, EPMEUpgradeType::SlowerDetection, EPMEUpgradeType::ExtraStartingItem
	};
	FRandomStream Stream(
		(MazeGenerator ? MazeGenerator->GetActiveSeed() : CurrentLevel * 101) ^ ((PlayerState
			? PlayerState->GetPlayerIndex()
			: 1) * 7919));
	TArray<EPMEUpgradeType> Result;
	while (Result.Num() < 3 && Pool.Num() > 0)
	{
		const int32 I = Stream.RandRange(0, Pool.Num() - 1);
		Result.Add(Pool[I]);
		Pool.RemoveAtSwap(I);
	}
	return Result;
}

void APMEGameModeBase::HandleUpgradeSelected(APMEPlayerState* PlayerState, const EPMEUpgradeType UpgradeType)
{
	if (!HasAuthority() || !MazeGameState || MazeGameState->GetRoundPhase() != EPMERoundPhase::UpgradeSelection || !
		PlayerState) return;
	const TWeakObjectPtr<APMEPlayerState> Key(PlayerState);
	if (PlayersWithUpgradeSelection.Contains(Key)) return;
	PlayerState->AddUpgrade(UpgradeType);
	PlayersWithUpgradeSelection.Add(Key);
	if (HaveAllPlayersSelectedUpgrade())
	{
		GetWorldTimerManager().ClearTimer(TransitionTimer);
		GetWorldTimerManager().SetTimerForNextTick(this, &APMEGameModeBase::AdvanceToNextMaze);
	}
}

void APMEGameModeBase::ResolveUpgradeTimeout()
{
	for (APlayerState* Base : MazeGameState->PlayerArray)
	{
		APMEPlayerState* PS = Cast<APMEPlayerState>(Base);
		if (!PS || PlayersWithUpgradeSelection.Contains(TWeakObjectPtr<APMEPlayerState>(PS))) continue;
		const TArray<EPMEUpgradeType> Choices = BuildUpgradeChoices(PS);
		if (Choices.Num() > 0)
		{
			PS->AddUpgrade(Choices[0]);
			PlayersWithUpgradeSelection.Add(TWeakObjectPtr<APMEPlayerState>(PS));
		}
	}
	AdvanceToNextMaze();
}

bool APMEGameModeBase::HaveAllPlayersSelectedUpgrade() const
{
	int32 Count = 0;
	for (APlayerState* Base : MazeGameState->PlayerArray) if (Cast<APMEPlayerState>(Base)) ++Count;
	return Count > 0 && PlayersWithUpgradeSelection.Num() >= Count;
}

void APMEGameModeBase::FinishRun()
{
	if (!MazeGameState) return;
	MazeGameState->SetRoundPhase(EPMERoundPhase::RunComplete);
	SetAllPlayersMoveIgnored(true);
	SetPatrolEnemiesEnabled(false);

	if (UPMEGameInstance* GI = GetGameInstance<UPMEGameInstance>())
	{
		if (UPMERunSubsystem* Run = GI->GetSubsystem<UPMERunSubsystem>())
		{
			int32 CombinedScore = 0;
			for (APlayerState* Base : MazeGameState->PlayerArray)
			{
				if (const APMEPlayerState* PS = Cast<APMEPlayerState>(Base)) CombinedScore += PS->GetRunScore();
			}
			Run->AddScore(CombinedScore);
		}
	}
}

EPMELevelModifier APMEGameModeBase::SelectLevelModifier(const int32 Level) const
{
	switch (Level)
	{
	case 2: return EPMELevelModifier::Darkness;
	case 3: return EPMELevelModifier::Echo;
	case 4: return EPMELevelModifier::Fury;
	case 5: return EPMELevelModifier::Hunters;
	default: return EPMELevelModifier::None;
	}
}

bool APMEGameModeBase::AreAllPlayersDowned() const
{
	int32 Players = 0;
	int32 Downed = 0;
	for (APlayerState* Base : MazeGameState->PlayerArray) if (const APMEPlayerState* PS = Cast<APMEPlayerState>(Base))
	{
		++Players;
		if (PS->IsDowned()) ++Downed;
	}
	return Players > 0 && Players == Downed;
}

APMEPlayerState* APMEGameModeBase::FindOtherActivePlayer(const APMEPlayerState* PlayerState) const
{
	for (APlayerState* Base : MazeGameState->PlayerArray) if (APMEPlayerState* PS = Cast<APMEPlayerState>(Base)) if (PS
		!= PlayerState && !PS->IsDowned()) return PS;
	return nullptr;
}

void APMEGameModeBase::EnsureWorldLighting()
{
	if (!UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()))
	{
		ADirectionalLight* DirectionalLight = GetWorld()->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			FVector(0.0f, 0.0f, 800.0f),
			FRotator(-55.0f, -35.0f, 0.0f));

		if (DirectionalLight && DirectionalLight->GetLightComponent())
		{
			DirectionalLight->GetLightComponent()->SetIntensity(4.0f);
			DirectionalLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		}
	}

	if (!UGameplayStatics::GetActorOfClass(this, ASkyLight::StaticClass()))
	{
		ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>();
		if (SkyLight && SkyLight->GetLightComponent())
		{
			SkyLight->GetLightComponent()->SetIntensity(0.8f);
			SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			SkyLight->GetLightComponent()->RecaptureSky();
		}
	}
}
