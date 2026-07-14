#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PMETypes.h"
#include "PMEMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
class SWidget;

enum class EPMEMenuScreen : uint8
{
	Main,
	MultiplayerModes,
	Connection
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildRootLayout();
	void ShowMainMenu();
	void ShowMultiplayerModes();
	void ShowConnectionMenu(EPMEPlayMode Mode);
	void RefreshCurrentScreen();
	void ClearMenu();
	UTextBlock* AddText(const FText& Text, int32 FontSize, bool bHeading = false);
	UButton* AddButton(const FText& Label);
	void AddSpacer(float Height);
	void AddLanguageSelector();

	UFUNCTION()
	void HandleSinglePlayer();

	UFUNCTION()
	void HandleMultiplayer();

	UFUNCTION()
	void HandleQuit();

	UFUNCTION()
	void HandleCoop();

	UFUNCTION()
	void HandleVersus();

	UFUNCTION()
	void HandleHost();

	UFUNCTION()
	void HandleJoin();

	UFUNCTION()
	void HandleBackToMain();

	UFUNCTION()
	void HandleBackToModes();

	UFUNCTION()
	void HandleSpanish();

	UFUNCTION()
	void HandleEnglish();

	UFUNCTION()
	void HandleConnectionError(const FText& ErrorMessage);

	UFUNCTION()
	void HandleLanguageChanged(EPMELanguage Language);

	UPROPERTY()
	TObjectPtr<UVerticalBox> MenuBox;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AddressBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	EPMEPlayMode SelectedMultiplayerMode = EPMEPlayMode::Coop;
	EPMEMenuScreen CurrentScreen = EPMEMenuScreen::Main;
};
