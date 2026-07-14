#include "PMERunSubsystem.h"

void UPMERunSubsystem::ResetRun(const int32 InMaximumLevels)
{
	CurrentLevel = 1;
	MaximumLevels = FMath::Clamp(InMaximumLevels, 1, 20);
	TotalScore = 0;
	RunSeed = FMath::Rand();
}

void UPMERunSubsystem::AdvanceLevel()
{
	CurrentLevel = FMath::Min(CurrentLevel + 1, MaximumLevels);
}

void UPMERunSubsystem::AddScore(const int32 Amount)
{
	TotalScore = FMath::Max(0, TotalScore + Amount);
}
