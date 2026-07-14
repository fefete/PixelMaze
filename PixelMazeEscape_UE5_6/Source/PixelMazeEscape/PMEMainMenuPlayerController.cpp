#include "PMEMainMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "PMEMainMenuWidget.h"
#include "Sound/SoundBase.h"

namespace PixelMazeMenuAudio
{
	static const TCHAR* MenuMusicPath =
		TEXT("/Game/PixelMaze/Audio/SW_BGM_Menu_80sSoft_Loop.SW_BGM_Menu_80sSoft_Loop");
}

APMEMainMenuPlayerController::APMEMainMenuPlayerController()
{
	MainMenuWidgetClass = UPMEMainMenuWidget::StaticClass();

	MenuMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MenuMusicComponent"));
	MenuMusicComponent->bAutoActivate = false;
	MenuMusicComponent->bAllowSpatialization = false;
	MenuMusicComponent->bIsUISound = true;
}

void APMEMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	ResolveAudioAssets();
	if (MenuMusicComponent && MenuMusic)
	{
		MenuMusicComponent->SetSound(MenuMusic);
		MenuMusicComponent->SetVolumeMultiplier(MenuMusicVolume);
		MenuMusicComponent->Play();
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Pixel Maze Escape: MainMenuWidgetClass is not configured."));
		return;
	}

	MainMenuWidget = CreateWidget<UPMEMainMenuWidget>(this, MainMenuWidgetClass);
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(100);

		bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		UE_LOG(LogTemp, Log, TEXT("Pixel Maze Escape: Main menu added to viewport."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Pixel Maze Escape: Failed to create main menu widget."));
	}
}

void APMEMainMenuPlayerController::ResolveAudioAssets()
{
	if (!MenuMusic)
	{
		MenuMusic = LoadObject<USoundBase>(nullptr, PixelMazeMenuAudio::MenuMusicPath);
	}
}
