#include "PMEHUD.h"

#include "AbilitySystemComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "PMEAttributeSet.h"
#include "PMEGameState.h"
#include "PMEInventoryComponent.h"
#include "PMELocalizationLibrary.h"
#include "PMEPlayerState.h"
#include "PMETypes.h"

namespace PixelMazeHudText
{
	FText Format(const FName Key, const FFormatNamedArguments& Arguments)
	{
		return FText::Format(UPMELocalizationLibrary::GetText(Key), Arguments);
	}

	FText Decimal(const float Value, const int32 Digits = 1)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = Digits;
		Options.MaximumFractionalDigits = Digits;
		return FText::AsNumber(Value, &Options);
	}

	FText InventorySlot(const UPMEInventoryComponent* Inventory, const int32 SlotIndex)
	{
		const EPMEPickupType Item = Inventory ? Inventory->GetItemInSlot(SlotIndex) : EPMEPickupType::None;
		const int32 Charges = Inventory ? Inventory->GetChargesInSlot(SlotIndex) : 0;
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Slot"), SlotIndex + 1);
		Arguments.Add(TEXT("Item"), UPMELocalizationLibrary::GetText(PixelMazePickup::ToTextKey(Item)));
		Arguments.Add(TEXT("Charges"), Charges);
		return Format(TEXT("HUD.ItemSlot"), Arguments);
	}
}

void APMEHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !GetWorld()) return;

	const APMEGameState* GameState = GetWorld()->GetGameState<APMEGameState>();
	if (!GameState) return;

	const APlayerController* OwnerController = GetOwningPlayerController();
	const APMEPlayerState* LocalPlayerState = OwnerController
		                                          ? OwnerController->GetPlayerState<APMEPlayerState>()
		                                          : nullptr;
	const UAbilitySystemComponent* ASC = LocalPlayerState ? LocalPlayerState->GetAbilitySystemComponent() : nullptr;
	const UPMEInventoryComponent* Inventory = LocalPlayerState ? LocalPlayerState->GetInventoryComponent() : nullptr;

	DrawPanel(18.0f, 18.0f, 430.0f, 190.0f);
	DrawLabel(UPMELocalizationLibrary::GetText(PixelMazeMode::ToDisplayTextKey(GameState->GetPlayMode())), 34.0f, 29.0f,
	          1.12f);

	FFormatNamedArguments LevelArguments;
	LevelArguments.Add(TEXT("Level"), GameState->GetCurrentLevel());
	DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Level"), LevelArguments), 34.0f, 55.0f, 1.0f);

	FFormatNamedArguments TimeArguments;
	TimeArguments.Add(TEXT("Time"), PixelMazeHudText::Decimal(GameState->GetElapsedRoundTime(), 1));
	DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Time"), TimeArguments), 34.0f, 80.0f, 0.92f);

	FFormatNamedArguments PhaseArguments;
	PhaseArguments.Add(
		TEXT("Phase"), UPMELocalizationLibrary::GetText(PixelMazeRoundPhase::ToTextKey(GameState->GetRoundPhase())));
	DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Phase"), PhaseArguments), 34.0f, 105.0f, 0.92f);

	FFormatNamedArguments ModifierArguments;
	ModifierArguments.Add(
		TEXT("Modifier"),
		UPMELocalizationLibrary::GetText(PixelMazeModifier::ToTextKey(GameState->GetLevelModifier())));
	DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Modifier"), ModifierArguments), 34.0f, 130.0f, 0.88f);

	FFormatNamedArguments SeedArguments;
	SeedArguments.Add(TEXT("Seed"), GameState->GetActiveSeed());
	DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Seed"), SeedArguments), 34.0f, 155.0f, 0.78f);

	const float BestTime = GameState->GetBestTime();
	if (BestTime > 0.0f)
	{
		FFormatNamedArguments BestArguments;
		BestArguments.Add(TEXT("Time"), PixelMazeHudText::Decimal(BestTime, 2));
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Best"), BestArguments), 34.0f, 177.0f, 0.78f);
	}
	else
	{
		DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.BestNone")), 34.0f, 177.0f, 0.78f);
	}

	if (LocalPlayerState)
	{
		const float PanelX = Canvas->SizeX - 390.0f;
		DrawPanel(PanelX, 18.0f, 372.0f, 214.0f);

		const float Health = ASC ? ASC->GetNumericAttribute(UPMEAttributeSet::GetHealthAttribute()) : 0.0f;
		const float MaxHealth = ASC ? ASC->GetNumericAttribute(UPMEAttributeSet::GetMaxHealthAttribute()) : 0.0f;
		FFormatNamedArguments HealthArguments;
		HealthArguments.Add(TEXT("Health"), FMath::RoundToInt(Health));
		HealthArguments.Add(TEXT("MaxHealth"), FMath::RoundToInt(MaxHealth));
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Health"), HealthArguments), PanelX + 18.0f, 31.0f, 1.05f);

		const int32 PlayerIndex = FMath::Max(1, LocalPlayerState->GetPlayerIndex());
		const int32 Objectives = GameState->GetPlayMode() == EPMEPlayMode::Versus
			                         ? GameState->GetPlayerObjectiveCount(PlayerIndex)
			                         : GameState->GetSharedObjectiveCount();
		const int32 Required = GameState->GetPlayMode() == EPMEPlayMode::Versus
			                       ? GameState->GetPlayerObjectiveRequired(PlayerIndex)
			                       : GameState->GetSharedObjectiveRequired();
		FFormatNamedArguments ObjectiveArguments;
		ObjectiveArguments.Add(TEXT("Done"), Objectives);
		ObjectiveArguments.Add(TEXT("Required"), Required);
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Objectives"), ObjectiveArguments), PanelX + 18.0f, 59.0f, 1.0f);

		FFormatNamedArguments ScoreArguments;
		ScoreArguments.Add(TEXT("Score"), LocalPlayerState->GetRunScore());
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.RunScore"), ScoreArguments), PanelX + 18.0f, 87.0f, 0.92f);

		DrawLabel(PixelMazeHudText::InventorySlot(Inventory, 0), PanelX + 18.0f, 119.0f, 0.86f);
		DrawLabel(PixelMazeHudText::InventorySlot(Inventory, 1), PanelX + 18.0f, 145.0f, 0.86f);

		FFormatNamedArguments StatArguments;
		StatArguments.Add(TEXT("Detections"), LocalPlayerState->GetDetectionCount());
		StatArguments.Add(TEXT("Hits"), LocalPlayerState->GetHitCount());
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.StatsCompact"), StatArguments), PanelX + 18.0f, 177.0f, 0.78f);

		if (LocalPlayerState->IsDowned())
		{
			DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.Downed")), PanelX + 18.0f, 199.0f, 0.92f);
		}
	}

	if (GameState->GetPlayMode() != EPMEPlayMode::SinglePlayer)
	{
		DrawPanel(Canvas->SizeX - 304.0f, 250.0f, 286.0f, 118.0f);
		DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.Score")), Canvas->SizeX - 284.0f, 263.0f, 1.0f);
		float ScoreY = 294.0f;
		for (APlayerState* BasePlayerState : GameState->PlayerArray)
		{
			const APMEPlayerState* PlayerState = Cast<APMEPlayerState>(BasePlayerState);
			if (!PlayerState) continue;
			FText StateSuffix = FText::GetEmpty();
			if (PlayerState->IsDowned()) StateSuffix = UPMELocalizationLibrary::GetText(TEXT("HUD.StateDowned"));
			else if (PlayerState->HasReachedExit()) StateSuffix = UPMELocalizationLibrary::GetText(TEXT("HUD.AtExit"));

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Player"), PlayerState->GetPlayerIndex());
			Arguments.Add(TEXT("Wins"), PlayerState->GetWins());
			Arguments.Add(TEXT("State"), StateSuffix);
			DrawLabel(PixelMazeHudText::Format(TEXT("HUD.PlayerScore"), Arguments), Canvas->SizeX - 284.0f, ScoreY,
			          0.86f);
			ScoreY += 28.0f;
		}
	}

	DrawPanel(18.0f, Canvas->SizeY - 72.0f, 910.0f, 50.0f);
	DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.ControlsGameplay")), 34.0f, Canvas->SizeY - 58.0f, 0.77f);

	if (GameState->IsWaitingForPlayers())
	{
		const float Width = 560.0f;
		const float Height = 142.0f;
		const float X = (Canvas->SizeX - Width) * 0.5f;
		const float Y = (Canvas->SizeY - Height) * 0.5f;
		DrawPanel(X, Y, Width, Height);
		DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.WaitingForPlayer2")), X + 82.0f, Y + 29.0f, 1.65f);
		DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.DirectIpPort")), X + 140.0f, Y + 88.0f, 1.0f);
		return;
	}

	if (GameState->GetRoundPhase() == EPMERoundPhase::Escape)
	{
		const float Width = 400.0f;
		const float X = (Canvas->SizeX - Width) * 0.5f;
		DrawPanel(X, 18.0f, Width, 58.0f);
		DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.ExitUnlocked")), X + 82.0f, 34.0f, 1.22f);
	}

	if (GameState->GetRoundPhase() == EPMERoundPhase::RoundResult || GameState->GetRoundPhase() ==
		EPMERoundPhase::RunComplete)
	{
		const float Width = 590.0f;
		const float Height = GameState->GetRoundPhase() == EPMERoundPhase::RunComplete ? 230.0f : 170.0f;
		const float X = (Canvas->SizeX - Width) * 0.5f;
		const float Y = (Canvas->SizeY - Height) * 0.5f;
		DrawPanel(X, Y, Width, Height);

		FText ResultText;
		if (GameState->GetWinnerPlayerIndex() < 0)
		{
			ResultText = UPMELocalizationLibrary::GetText(TEXT("HUD.Defeat"));
		}
		else if (GameState->GetPlayMode() == EPMEPlayMode::Versus)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Player"), GameState->GetWinnerPlayerIndex());
			ResultText = PixelMazeHudText::Format(TEXT("HUD.PlayerWins"), Arguments);
		}
		else if (GameState->GetPlayMode() == EPMEPlayMode::Coop)
		{
			ResultText = UPMELocalizationLibrary::GetText(TEXT("HUD.TeamEscaped"));
		}
		else
		{
			ResultText = UPMELocalizationLibrary::GetText(TEXT("HUD.Escaped"));
		}

		if (GameState->GetRoundPhase() == EPMERoundPhase::RunComplete && GameState->GetWinnerPlayerIndex() >= 0)
		{
			ResultText = UPMELocalizationLibrary::GetText(TEXT("HUD.RunComplete"));
		}
		DrawLabel(ResultText, X + 80.0f, Y + 27.0f, 1.58f);

		FFormatNamedArguments SecondsArguments;
		SecondsArguments.Add(TEXT("Time"), PixelMazeHudText::Decimal(GameState->GetElapsedRoundTime(), 2));
		DrawLabel(PixelMazeHudText::Format(TEXT("HUD.Seconds"), SecondsArguments), X + 155.0f, Y + 86.0f, 1.0f);

		if (GameState->GetRoundPhase() == EPMERoundPhase::RunComplete && LocalPlayerState)
		{
			FFormatNamedArguments SummaryArguments;
			SummaryArguments.Add(TEXT("Score"), LocalPlayerState->GetRunScore());
			SummaryArguments.Add(TEXT("Objectives"), LocalPlayerState->GetObjectivesActivated());
			SummaryArguments.Add(TEXT("Detections"), LocalPlayerState->GetDetectionCount());
			SummaryArguments.Add(TEXT("Hits"), LocalPlayerState->GetHitCount());
			DrawLabel(PixelMazeHudText::Format(TEXT("HUD.RunSummary"), SummaryArguments), X + 56.0f, Y + 128.0f, 0.88f);
			DrawLabel(UPMELocalizationLibrary::GetText(TEXT("HUD.RunCompleteHint")), X + 80.0f, Y + 184.0f, 0.82f);
		}
	}
}

void APMEHUD::DrawPanel(const float X, const float Y, const float Width, const float Height)
{
	DrawRect(FLinearColor(0.025f, 0.02f, 0.06f, 0.92f), X, Y, Width, Height);
	DrawRect(FLinearColor(0.38f, 0.12f, 0.62f, 1.0f), X, Y, Width, 4.0f);
	DrawRect(FLinearColor(0.38f, 0.12f, 0.62f, 1.0f), X, Y + Height - 4.0f, Width, 4.0f);
	DrawRect(FLinearColor(0.38f, 0.12f, 0.62f, 1.0f), X, Y, 4.0f, Height);
	DrawRect(FLinearColor(0.38f, 0.12f, 0.62f, 1.0f), X + Width - 4.0f, Y, 4.0f, Height);
}

void APMEHUD::DrawLabel(const FText& Text, const float X, const float Y, const float Scale)
{
	DrawText(Text.ToString(), FLinearColor(0.82f, 1.0f, 0.94f, 1.0f), X, Y,
	         GEngine ? GEngine->GetSmallFont() : nullptr, Scale, false);
}
