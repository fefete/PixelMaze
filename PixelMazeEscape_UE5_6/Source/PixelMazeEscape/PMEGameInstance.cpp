#include "PMEGameInstance.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "PMELocalizationLibrary.h"
#include "PMERunSubsystem.h"

namespace PixelMazeTravel
{
	static const FName EmptyMap(TEXT("/Engine/Maps/Entry"));
	static const FName MenuMap(TEXT("/Game/Maps/MainMenu"));
	static const FName GameplayMap(TEXT("/Game/Maps/GameplayMap"));
	static const TCHAR* GameplayGameMode = TEXT("/Script/PixelMazeEscape.PMEGameModeBase");
	static const TCHAR* MenuGameMode = TEXT("/Script/PixelMazeEscape.PMEMainMenuGameMode");
}

void UPMEGameInstance::Init()
{
	Super::Init();

	// Keep supported operating-system cultures. Any other culture falls back
	// to English so every String Table lookup always resolves to a known key.
	const FString CurrentCulture = UPMELocalizationLibrary::GetCurrentLanguageCode();
	UKismetInternationalizationLibrary::SetCurrentCulture(CurrentCulture, false);

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UPMEGameInstance::HandleNetworkFailure);
	}
}

void UPMEGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UPMEGameInstance::RegisterCompletion(const float CompletionTime)
{
	if (CompletionTime > 0.0f && (BestCompletionTime <= 0.0f || CompletionTime < BestCompletionTime))
	{
		BestCompletionTime = CompletionTime;
	}
}

void UPMEGameInstance::AdvanceLevel()
{
	++CurrentLevel;
	if (UPMERunSubsystem* Run = GetSubsystem<UPMERunSubsystem>()) Run->AdvanceLevel();
}

void UPMEGameInstance::ResetRun()
{
	CurrentLevel = 1;
	BestCompletionTime = 0.0f;
	if (UPMERunSubsystem* Run = GetSubsystem<UPMERunSubsystem>()) Run->ResetRun(5);
}

void UPMEGameInstance::StartSinglePlayer()
{
	ResetRun();
	RequestedPlayMode = EPMEPlayMode::SinglePlayer;
	OpenGameplayMap(RequestedPlayMode, false);
}

void UPMEGameInstance::HostMultiplayer(const EPMEPlayMode Mode)
{
	if (Mode == EPMEPlayMode::SinglePlayer)
	{
		StartSinglePlayer();
		return;
	}

	ResetRun();
	LastConnectionError = FText::GetEmpty();
	LastConnectionErrorKey = NAME_None;
	RequestedPlayMode = Mode;
	OpenGameplayMap(Mode, true);
}

void UPMEGameInstance::JoinMultiplayer(const FString& Address, const EPMEPlayMode Mode)
{
	FString CleanAddress = Address.TrimStartAndEnd();
	if (CleanAddress.IsEmpty())
	{
		CleanAddress = TEXT("127.0.0.1:7777");
	}
	else if (!CleanAddress.Contains(TEXT(":")))
	{
		CleanAddress += TEXT(":7777");
	}

	RequestedPlayMode = Mode;
	LastConnectionError = FText::GetEmpty();
	LastConnectionErrorKey = NAME_None;
	CleanAddress += FString::Printf(
		TEXT("?RequestedMode=%s"),
		*PixelMazeMode::ToOptionString(Mode));

	if (APlayerController* PlayerController = GetFirstLocalPlayerController())
	{
		PlayerController->ClientTravel(CleanAddress, ETravelType::TRAVEL_Absolute);
	}
}

void UPMEGameInstance::ReturnToMainMenu()
{
	LastConnectionError = FText::GetEmpty();
	LastConnectionErrorKey = NAME_None;
	const FString Options = FString::Printf(TEXT("game=%s"), PixelMazeTravel::MenuGameMode);

	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		if (APlayerController* PlayerController = GetFirstLocalPlayerController())
		{
			const FString TravelURL = FString::Printf(
				TEXT("%s?%s"),
				*PixelMazeTravel::MenuMap.ToString(),
				*Options);
			PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
		return;
	}

	UGameplayStatics::OpenLevel(this, PixelMazeTravel::EmptyMap, true, Options);
}

bool UPMEGameInstance::SetLanguage(const EPMELanguage Language)
{
	const FString CultureCode = Language == EPMELanguage::Spanish
		                            ? TEXT("es")
		                            : TEXT("en");

	const bool bChanged = UKismetInternationalizationLibrary::SetCurrentCulture(CultureCode, true);
	if (bChanged)
	{
		RefreshConnectionErrorText();
		OnLanguageChanged.Broadcast(Language);
	}

	return bChanged;
}

EPMELanguage UPMEGameInstance::GetLanguage() const
{
	return UPMELocalizationLibrary::GetCurrentLanguage();
}

void UPMEGameInstance::OpenGameplayMap(const EPMEPlayMode Mode, const bool bListenServer)
{
	FString Options = FString::Printf(
		TEXT("game=%s?Mode=%s"),
		PixelMazeTravel::GameplayGameMode,
		*PixelMazeMode::ToOptionString(Mode));

	if (bListenServer)
	{
		Options += TEXT("?listen");
	}

	UGameplayStatics::OpenLevel(this, PixelMazeTravel::GameplayMap, true, Options);
}

void UPMEGameInstance::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	if (ErrorString.Contains(TEXT("ServerFull")))
	{
		LastConnectionErrorKey = TEXT("Error.ServerFull");
	}
	else if (ErrorString.Contains(TEXT("ModeMismatch")))
	{
		LastConnectionErrorKey = TEXT("Error.ModeMismatch");
	}
	else
	{
		LastConnectionErrorKey = TEXT("Error.ConnectionGeneric");
	}

	RefreshConnectionErrorText();
	OnConnectionError.Broadcast(LastConnectionError);
}

void UPMEGameInstance::RefreshConnectionErrorText()
{
	LastConnectionError = LastConnectionErrorKey.IsNone()
		                      ? FText::GetEmpty()
		                      : UPMELocalizationLibrary::GetText(LastConnectionErrorKey);
}
