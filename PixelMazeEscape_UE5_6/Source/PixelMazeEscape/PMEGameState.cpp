#include "PMEGameState.h"
#include "Net/UnrealNetwork.h"
APMEGameState::APMEGameState() { bReplicates = true; }

void APMEGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APMEGameState, PlayMode);
	DOREPLIFETIME(APMEGameState, CurrentLevel);
	DOREPLIFETIME(APMEGameState, ActiveSeed);
	DOREPLIFETIME(APMEGameState, RoundPhase);
	DOREPLIFETIME(APMEGameState, LevelModifier);
	DOREPLIFETIME(APMEGameState, RoundStartServerTime);
	DOREPLIFETIME(APMEGameState, FrozenCompletionTime);
	DOREPLIFETIME(APMEGameState, BestTime);
	DOREPLIFETIME(APMEGameState, WinnerPlayerIndex);
	DOREPLIFETIME(APMEGameState, WinnerName);
	DOREPLIFETIME(APMEGameState, SharedObjectiveCount);
	DOREPLIFETIME(APMEGameState, SharedObjectiveRequired);
	DOREPLIFETIME(APMEGameState, Player1ObjectiveCount);
	DOREPLIFETIME(APMEGameState, Player1ObjectiveRequired);
	DOREPLIFETIME(APMEGameState, Player2ObjectiveCount);
	DOREPLIFETIME(APMEGameState, Player2ObjectiveRequired);
}

float APMEGameState::GetElapsedRoundTime() const
{
	if (IsRoundFinished())return FrozenCompletionTime;
	if (RoundPhase == EPMERoundPhase::WaitingForPlayers || RoundStartServerTime <= 0.0f)return 0.0f;
	return FMath::Max(0.0f, GetServerWorldTimeSeconds() - RoundStartServerTime);
}

void APMEGameState::ConfigureMatch(EPMEPlayMode NewMode)
{
	check(HasAuthority());
	PlayMode = NewMode;
	ForceNetUpdate();
}

void APMEGameState::SetWaitingForPlayers(bool bWaiting)
{
	check(HasAuthority());
	RoundPhase = bWaiting ? EPMERoundPhase::WaitingForPlayers : EPMERoundPhase::Exploring;
	WinnerPlayerIndex = 0;
	WinnerName.Reset();
	FrozenCompletionTime = 0;
	RoundStartServerTime = 0;
	ForceNetUpdate();
}

void APMEGameState::BeginRound(int32 NewLevel, int32 NewSeed, EPMELevelModifier NewModifier)
{
	check(HasAuthority());
	CurrentLevel = NewLevel;
	ActiveSeed = NewSeed;
	LevelModifier = NewModifier;
	RoundPhase = EPMERoundPhase::Exploring;
	WinnerPlayerIndex = 0;
	WinnerName.Reset();
	FrozenCompletionTime = 0;
	RoundStartServerTime = GetServerWorldTimeSeconds();
	SharedObjectiveCount = SharedObjectiveRequired = Player1ObjectiveCount = Player1ObjectiveRequired =
		Player2ObjectiveCount = Player2ObjectiveRequired = 0;
	ForceNetUpdate();
}

void APMEGameState::SetRoundPhase(EPMERoundPhase NewPhase)
{
	check(HasAuthority());
	RoundPhase = NewPhase;
	ForceNetUpdate();
}

void APMEGameState::FinishRound(float CompletionTime, int32 Index, const FString& Name)
{
	check(HasAuthority());
	RoundPhase = EPMERoundPhase::RoundResult;
	FrozenCompletionTime = FMath::Max(0.0f, CompletionTime);
	WinnerPlayerIndex = Index;
	WinnerName = Name;
	ForceNetUpdate();
}

void APMEGameState::SetBestTime(float NewBestTime)
{
	check(HasAuthority());
	BestTime = NewBestTime;
	ForceNetUpdate();
}

void APMEGameState::SetObjectiveProgress(int32 SD, int32 SR, int32 P1D, int32 P1R, int32 P2D, int32 P2R)
{
	check(HasAuthority());
	SharedObjectiveCount = SD;
	SharedObjectiveRequired = SR;
	Player1ObjectiveCount = P1D;
	Player1ObjectiveRequired = P1R;
	Player2ObjectiveCount = P2D;
	Player2ObjectiveRequired = P2R;
	ForceNetUpdate();
}
