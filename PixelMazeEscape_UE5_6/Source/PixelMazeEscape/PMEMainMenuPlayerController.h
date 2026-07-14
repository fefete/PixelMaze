#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PMEMainMenuPlayerController.generated.h"

class UAudioComponent;
class UPMEMainMenuWidget;
class USoundBase;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APMEMainMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Menu")
	TSubclassOf<UPMEMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pixel Maze|Audio")
	TObjectPtr<UAudioComponent> MenuMusicComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> MenuMusic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MenuMusicVolume = 0.62f;

private:
	void ResolveAudioAssets();

	UPROPERTY()
	TObjectPtr<UPMEMainMenuWidget> MainMenuWidget;
};
