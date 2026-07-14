#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PMETypes.h"
#include "PMEPlayerController.generated.h"

class APMEFogOfWarActor;
class UAudioComponent;
class USoundBase;
class UPMEUpgradeSelectionWidget;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APMEPlayerController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Fog of War")
	void SetFogRevealRadius(float NewRadiusInTiles);
	UFUNCTION(Client, Reliable)
	void ClientPlayEnemyHitSound();
	UFUNCTION(Client, Reliable)
	void ClientActivateMapPulse(float Duration);
	UFUNCTION(Client, Reliable)
	void ClientShowUpgradeChoices(const TArray<EPMEUpgradeType>& Choices);
	UFUNCTION(Client, Reliable)
	void ClientHideUpgradeChoices();
	void SubmitUpgradeChoice(EPMEUpgradeType Choice);

protected:
	virtual void BeginPlay() override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Fog of War")
	TSubclassOf<APMEFogOfWarActor> FogOfWarClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="0.5", ClampMax="30.0"))
	float FogRevealRadiusInTiles = 5.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pixel Maze|Audio")
	TObjectPtr<UAudioComponent> MazeAmbientComponent;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> MazeAmbientMusic;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> VictorySound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> DefeatSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> EnemyHitPlayerSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MazeAmbientVolume = 0.42f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ResultSoundVolume = 0.78f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EnemyHitPlayerVolume = 0.85f;
	UPROPERTY(EditDefaultsOnly, Category="Pixel Maze|Upgrades")
	TSubclassOf<UPMEUpgradeSelectionWidget> UpgradeWidgetClass;

private:
	void SpawnLocalFog();
	void RequestRestartMaze();
	void ReturnToMenu();
	void ResolveAudioAssets();
	void UpdateRoundResultAudio();
	void PlayRoundResultSound();
	void RestoreFogAfterPulse();
	void RefreshFogFromAttributes();
	UFUNCTION(Server, Reliable)
	void ServerRequestRestartMaze();
	UFUNCTION(Server, Reliable)
	void ServerChooseUpgrade(EPMEUpgradeType Choice);
	UPROPERTY()
	TObjectPtr<APMEFogOfWarActor> LocalFogActor;
	UPROPERTY()
	TObjectPtr<UPMEUpgradeSelectionWidget> UpgradeWidget;
	FTimerHandle MapPulseTimer;
	bool bMapPulseActive = false;
	bool bHasObservedRoundState = false;
	bool bWasRoundFinished = false;
};
