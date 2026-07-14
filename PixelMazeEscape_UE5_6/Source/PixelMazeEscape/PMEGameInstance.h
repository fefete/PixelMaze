#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "PMETypes.h"
#include "PMEGameInstance.generated.h"

class UNetDriver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPMEConnectionErrorSignature, const FText&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPMELanguageChangedSignature, EPMELanguage, Language);

UCLASS()
class PIXELMAZEESCAPE_API UPMEGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    UPROPERTY(BlueprintReadOnly, Category="Pixel Maze")
    int32 CurrentLevel = 1;

    UPROPERTY(BlueprintReadOnly, Category="Pixel Maze")
    float BestCompletionTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Network")
    EPMEPlayMode RequestedPlayMode = EPMEPlayMode::SinglePlayer;

    UPROPERTY(BlueprintReadOnly, Category="Pixel Maze|Network")
    FText LastConnectionError;

    UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Network")
    FPMEConnectionErrorSignature OnConnectionError;

    UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Localization")
    FPMELanguageChangedSignature OnLanguageChanged;

    UFUNCTION(BlueprintCallable, Category="Pixel Maze")
    void RegisterCompletion(float CompletionTime);

    UFUNCTION(BlueprintCallable, Category="Pixel Maze")
    void AdvanceLevel();

    UFUNCTION(BlueprintCallable, Category="Pixel Maze")
    void ResetRun();

    UFUNCTION(BlueprintCallable, Category="Pixel Maze|Menu")
    void StartSinglePlayer();

    UFUNCTION(BlueprintCallable, Category="Pixel Maze|Menu")
    void HostMultiplayer(EPMEPlayMode Mode);

    UFUNCTION(BlueprintCallable, Category="Pixel Maze|Menu")
    void JoinMultiplayer(const FString& Address, EPMEPlayMode Mode);

    UFUNCTION(BlueprintCallable, Category="Pixel Maze|Menu")
    void ReturnToMainMenu();

    /** Sets and persists the selected culture. Each local client chooses independently. */
    UFUNCTION(BlueprintCallable, Category="Pixel Maze|Localization")
    bool SetLanguage(EPMELanguage Language);

    UFUNCTION(BlueprintPure, Category="Pixel Maze|Localization")
    EPMELanguage GetLanguage() const;

private:
    void OpenGameplayMap(EPMEPlayMode Mode, bool bListenServer);
    void HandleNetworkFailure(
        UWorld* World,
        UNetDriver* NetDriver,
        ENetworkFailure::Type FailureType,
        const FString& ErrorString);

    void RefreshConnectionErrorText();

    FName LastConnectionErrorKey = NAME_None;
};
