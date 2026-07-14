#include "PMEPlayerController.h"

#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PMEAttributeSet.h"
#include "PMEFogOfWarActor.h"
#include "PMEGameInstance.h"
#include "PMEGameModeBase.h"
#include "PMEGameState.h"
#include "PMEPlayerState.h"
#include "PMEUpgradeSelectionWidget.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace PixelMazeGameplayAudio
{
	static const TCHAR* MazeAmbientPath = TEXT(
		"/Game/PixelMaze/Audio/SW_BGM_Maze_BreathingMaze_Loop.SW_BGM_Maze_BreathingMaze_Loop");
	static const TCHAR* VictoryPath = TEXT("/Game/PixelMaze/Audio/SW_SFX_Victory.SW_SFX_Victory");
	static const TCHAR* DefeatPath = TEXT("/Game/PixelMaze/Audio/SW_SFX_Defeat.SW_SFX_Defeat");
	static const TCHAR* EnemyHitPlayerPath =
		TEXT("/Game/PixelMaze/Audio/SW_SFX_Enemy_HitPlayer.SW_SFX_Enemy_HitPlayer");
}

APMEPlayerController::APMEPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	FogOfWarClass = APMEFogOfWarActor::StaticClass();
	UpgradeWidgetClass = UPMEUpgradeSelectionWidget::StaticClass();
	MazeAmbientComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MazeAmbientComponent"));
	MazeAmbientComponent->bAutoActivate = false;
	MazeAmbientComponent->bAllowSpatialization = false;
}

void APMEPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	if (IsLocalController())
	{
		ResolveAudioAssets();
		if (MazeAmbientComponent && MazeAmbientMusic)
		{
			MazeAmbientComponent->SetSound(MazeAmbientMusic);
			MazeAmbientComponent->SetVolumeMultiplier(MazeAmbientVolume);
			MazeAmbientComponent->Play();
		}
		GetWorldTimerManager().SetTimerForNextTick(this, &APMEPlayerController::SpawnLocalFog);
	}
}

void APMEPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocalController())
	{
		UpdateRoundResultAudio();
		if (!bMapPulseActive)RefreshFogFromAttributes();
	}
}

void APMEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindAction(TEXT("RegenerateMaze"), IE_Pressed, this, &APMEPlayerController::RequestRestartMaze);
		InputComponent->BindAction(TEXT("ReturnToMenu"), IE_Pressed, this, &APMEPlayerController::ReturnToMenu);
	}
}

void APMEPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (IsLocalController() && LocalFogActor)LocalFogActor->SetTrackedPawn(InPawn);
}

void APMEPlayerController::SetFogRevealRadius(float NewRadius)
{
	const float ClampedRadius = FMath::Clamp(NewRadius, 0.5f, 30.0f);
	if (FMath::IsNearlyEqual(FogRevealRadiusInTiles, ClampedRadius, KINDA_SMALL_NUMBER)) return;
	FogRevealRadiusInTiles = ClampedRadius;
	if (LocalFogActor) LocalFogActor->SetRevealRadiusInTiles(FogRevealRadiusInTiles);
}

void APMEPlayerController::RefreshFogFromAttributes()
{
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	const UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	float Radius = ASC
		               ? ASC->GetNumericAttribute(UPMEAttributeSet::GetFogRevealRadiusAttribute())
		               : FogRevealRadiusInTiles;
	const APMEGameState* GS = GetWorld() ? GetWorld()->GetGameState<APMEGameState>() : nullptr;
	if (GS && GS->GetLevelModifier() == EPMELevelModifier::Darkness)Radius -= 2.0f;
	SetFogRevealRadius(FMath::Max(1.0f, Radius));
}

void APMEPlayerController::ClientActivateMapPulse_Implementation(float Duration)
{
	if (!IsLocalController())return;
	bMapPulseActive = true;
	if (LocalFogActor)LocalFogActor->SetRevealRadiusInTiles(30.0f);
	GetWorldTimerManager().ClearTimer(MapPulseTimer);
	GetWorldTimerManager().SetTimer(MapPulseTimer, this, &APMEPlayerController::RestoreFogAfterPulse,
	                                FMath::Max(0.2f, Duration), false);
}

void APMEPlayerController::RestoreFogAfterPulse()
{
	bMapPulseActive = false;
	RefreshFogFromAttributes();
}

void APMEPlayerController::ClientPlayEnemyHitSound_Implementation()
{
	if (!IsLocalController())return;
	ResolveAudioAssets();
	if (EnemyHitPlayerSound)UGameplayStatics::PlaySound2D(this, EnemyHitPlayerSound, EnemyHitPlayerVolume);
}

void APMEPlayerController::ClientShowUpgradeChoices_Implementation(const TArray<EPMEUpgradeType>& Choices)
{
	if (!IsLocalController())return;
	if (!UpgradeWidget && UpgradeWidgetClass)
	{
		UpgradeWidget = CreateWidget<UPMEUpgradeSelectionWidget>(this, UpgradeWidgetClass);
		if (UpgradeWidget)UpgradeWidget->AddToViewport(100);
	}
	if (UpgradeWidget)UpgradeWidget->ConfigureChoices(Choices);
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

void APMEPlayerController::ClientHideUpgradeChoices_Implementation()
{
	if (UpgradeWidget)
	{
		UpgradeWidget->RemoveFromParent();
		UpgradeWidget = nullptr;
	}
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void APMEPlayerController::SubmitUpgradeChoice(EPMEUpgradeType Choice)
{
	ServerChooseUpgrade(Choice);
	ClientHideUpgradeChoices();
}

void APMEPlayerController::ServerChooseUpgrade_Implementation(EPMEUpgradeType Choice)
{
	if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>())GM->HandleUpgradeSelected(
		GetPlayerState<APMEPlayerState>(), Choice);
}

void APMEPlayerController::SpawnLocalFog()
{
	if (!IsLocalController() || LocalFogActor || !FogOfWarClass)return;
	APawn* LocalPawn = GetPawn();
	if (!LocalPawn)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &APMEPlayerController::SpawnLocalFog);
		return;
	}
	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LocalFogActor = GetWorld()->SpawnActor<APMEFogOfWarActor>(FogOfWarClass, FTransform::Identity, P);
	if (LocalFogActor)
	{
		RefreshFogFromAttributes();
		LocalFogActor->InitializeFog(LocalPawn, FogRevealRadiusInTiles);
	}
}

void APMEPlayerController::RequestRestartMaze()
{
	if (HasAuthority())ServerRequestRestartMaze_Implementation();
	else ServerRequestRestartMaze();
}

void APMEPlayerController::ServerRequestRestartMaze_Implementation()
{
	if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>())if (GM->GetPlayMode() ==
		EPMEPlayMode::SinglePlayer || IsLocalController())GM->RestartCurrentMaze();
}

void APMEPlayerController::ReturnToMenu()
{
	if (UPMEGameInstance* GI = GetGameInstance<UPMEGameInstance>())GI->ReturnToMainMenu();
}

void APMEPlayerController::ResolveAudioAssets()
{
	if (!MazeAmbientMusic)MazeAmbientMusic = LoadObject<USoundBase>(nullptr, PixelMazeGameplayAudio::MazeAmbientPath);
	if (!VictorySound)VictorySound = LoadObject<USoundBase>(nullptr, PixelMazeGameplayAudio::VictoryPath);
	if (!DefeatSound)DefeatSound = LoadObject<USoundBase>(nullptr, PixelMazeGameplayAudio::DefeatPath);
	if (!EnemyHitPlayerSound)EnemyHitPlayerSound = LoadObject<USoundBase>(
		nullptr, PixelMazeGameplayAudio::EnemyHitPlayerPath);
}

void APMEPlayerController::UpdateRoundResultAudio()
{
	const APMEGameState* GS = GetWorld() ? GetWorld()->GetGameState<APMEGameState>() : nullptr;
	if (!GS)return;
	const bool Finished = GS->IsRoundFinished();
	if (!bHasObservedRoundState)
	{
		bHasObservedRoundState = true;
		bWasRoundFinished = Finished;
		return;
	}
	if (Finished && !bWasRoundFinished)PlayRoundResultSound();
	bWasRoundFinished = Finished;
}

void APMEPlayerController::PlayRoundResultSound()
{
	ResolveAudioAssets();
	const APMEGameState* GS = GetWorld() ? GetWorld()->GetGameState<APMEGameState>() : nullptr;
	if (!GS) return;
	bool bWon = GS->GetWinnerPlayerIndex() >= 0;
	if (bWon && GS->GetPlayMode() == EPMEPlayMode::Versus)
	{
		const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
		bWon = PS && PS->GetPlayerIndex() == GS->GetWinnerPlayerIndex();
	}
	USoundBase* ResultSound = bWon ? VictorySound.Get() : DefeatSound.Get();
	if (ResultSound) UGameplayStatics::PlaySound2D(this, ResultSound, ResultSoundVolume);
}
