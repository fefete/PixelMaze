#include "PMEMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PMECharacterCustomizationWidget.h"
#include "PMEGameInstance.h"
#include "PMELocalizationLibrary.h"

void UPMEMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildRootLayout();
}

TSharedRef<SWidget> UPMEMainMenuWidget::RebuildWidget()
{
	BuildRootLayout();
	return Super::RebuildWidget();
}

void UPMEMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		GameInstance->OnConnectionError.RemoveAll(this);
		GameInstance->OnConnectionError.AddDynamic(this, &UPMEMainMenuWidget::HandleConnectionError);
		GameInstance->OnLanguageChanged.RemoveAll(this);
		GameInstance->OnLanguageChanged.AddDynamic(this, &UPMEMainMenuWidget::HandleLanguageChanged);
	}

	ShowMainMenu();
}

void UPMEMainMenuWidget::BuildRootLayout()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* FullBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FullBackground"));
	FullBackground->SetBrushColor(FLinearColor(0.008f, 0.006f, 0.025f, 1.0f));
	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(FullBackground);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UBorder* MenuPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	MenuPanel->SetBrushColor(FLinearColor(0.035f, 0.02f, 0.075f, 0.98f));
	MenuPanel->SetPadding(FMargin(38.0f, 32.0f));

	UCanvasPanelSlot* MenuPanelSlot = RootCanvas->AddChildToCanvas(MenuPanel);
	MenuPanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MenuPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuPanelSlot->SetPosition(FVector2D::ZeroVector);
	MenuPanelSlot->SetSize(FVector2D(620.0f, 760.0f));

	MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	MenuPanel->SetContent(MenuBox);
}

void UPMEMainMenuWidget::ShowMainMenu()
{
	CurrentScreen = EPMEMenuScreen::Main;
	ClearMenu();

	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.Title")), 40, true);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.Subtitle")), 18);
	AddSpacer(42.0f);

	UButton* SingleButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.SinglePlayer")));
	SingleButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleSinglePlayer);

	UButton* MultiplayerButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Multiplayer")));
	MultiplayerButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleMultiplayer);

	UButton* CustomizationButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Customization")));
	CustomizationButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleCustomization);

	UButton* QuitButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Quit")));
	QuitButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleQuit);

	if (const UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		if (!GameInstance->LastConnectionError.IsEmpty())
		{
			AddSpacer(18.0f);
			StatusText = AddText(GameInstance->LastConnectionError, 14);
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f, 1.0f)));
		}
	}

	AddLanguageSelector();
}

void UPMEMainMenuWidget::ShowMultiplayerModes()
{
	CurrentScreen = EPMEMenuScreen::MultiplayerModes;
	ClearMenu();

	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.Multiplayer")), 36, true);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.MaxPlayers")), 16);
	AddSpacer(45.0f);

	UButton* CoopButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Mode.Coop")));
	CoopButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleCoop);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.CoopDescription")), 14);

	AddSpacer(15.0f);

	UButton* VersusButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Mode.Versus")));
	VersusButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleVersus);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.VersusDescription")), 14);

	AddSpacer(35.0f);

	UButton* BackButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Back")));
	BackButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleBackToMain);
}

void UPMEMainMenuWidget::ShowConnectionMenu(const EPMEPlayMode Mode)
{
	CurrentScreen = EPMEMenuScreen::Connection;
	SelectedMultiplayerMode = Mode;
	ClearMenu();

	AddText(UPMELocalizationLibrary::GetText(PixelMazeMode::ToDisplayTextKey(Mode)), 36, true);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.DirectConnection")), 16);
	AddSpacer(38.0f);

	UButton* HostButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Host")));
	HostButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleHost);

	AddSpacer(25.0f);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.HostAddress")), 16);

	AddressBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	AddressBox->SetText(FText::FromString(TEXT("127.0.0.1:7777")));
	AddressBox->SetHintText(UPMELocalizationLibrary::GetText(TEXT("Menu.AddressHint")));
	AddressBox->SetForegroundColor(FLinearColor::White);
	if (UVerticalBoxSlot* AddressSlot = MenuBox->AddChildToVerticalBox(AddressBox))
	{
		AddressSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 14.0f));
		AddressSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	UButton* JoinButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Join")));
	JoinButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleJoin);

	AddSpacer(18.0f);
	StatusText = AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.HostWaiting")), 13);

	AddSpacer(24.0f);
	UButton* BackButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.Back")));
	BackButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleBackToModes);
}

void UPMEMainMenuWidget::RefreshCurrentScreen()
{
	switch (CurrentScreen)
	{
	case EPMEMenuScreen::MultiplayerModes:
		ShowMultiplayerModes();
		break;
	case EPMEMenuScreen::Connection:
		ShowConnectionMenu(SelectedMultiplayerMode);
		break;
	default:
		ShowMainMenu();
		break;
	}
}

void UPMEMainMenuWidget::ClearMenu()
{
	AddressBox = nullptr;
	StatusText = nullptr;
	if (MenuBox)
	{
		MenuBox->ClearChildren();
	}
}

UTextBlock* UPMEMainMenuWidget::AddText(const FText& Text, const int32 FontSize, const bool bHeading)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetJustification(ETextJustify::Center);
	TextBlock->SetAutoWrapText(true);
	TextBlock->SetColorAndOpacity(FSlateColor(
		bHeading
			? FLinearColor(0.98f, 0.77f, 0.16f, 1.0f)
			: FLinearColor(0.80f, 1.0f, 0.93f, 1.0f)));

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	if (UVerticalBoxSlot* vSlot = MenuBox->AddChildToVerticalBox(TextBlock))
	{
		vSlot->SetHorizontalAlignment(HAlign_Fill);
		vSlot->SetPadding(FMargin(4.0f));
	}

	return TextBlock;
}

UButton* UPMEMainMenuWidget::AddButton(const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.28f, 0.08f, 0.48f, 1.0f));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(Label);
	LabelText->SetJustification(ETextJustify::Center);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo FontInfo = LabelText->GetFont();
	FontInfo.Size = 19;
	LabelText->SetFont(FontInfo);
	Button->SetContent(LabelText);

	if (UVerticalBoxSlot* vSlot = MenuBox->AddChildToVerticalBox(Button))
	{
		vSlot->SetHorizontalAlignment(HAlign_Fill);
		vSlot->SetPadding(FMargin(0.0f, 7.0f));
	}

	return Button;
}

void UPMEMainMenuWidget::AddSpacer(const float Height)
{
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer->SetSize(FVector2D(1.0f, Height));
	MenuBox->AddChildToVerticalBox(Spacer);
}

void UPMEMainMenuWidget::AddLanguageSelector()
{
	AddSpacer(20.0f);
	AddText(UPMELocalizationLibrary::GetText(TEXT("Menu.Language")), 14);

	UButton* SpanishButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.LanguageSpanish")));
	SpanishButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleSpanish);

	UButton* EnglishButton = AddButton(UPMELocalizationLibrary::GetText(TEXT("Menu.LanguageEnglish")));
	EnglishButton->OnClicked.AddDynamic(this, &UPMEMainMenuWidget::HandleEnglish);
}

void UPMEMainMenuWidget::HandleSinglePlayer()
{
	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		GameInstance->StartSinglePlayer();
	}
}

void UPMEMainMenuWidget::HandleMultiplayer()
{
	ShowMultiplayerModes();
}

void UPMEMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UPMEMainMenuWidget::HandleCustomization()
{
	if (!CustomizationWidgetClass)
	{
		CustomizationWidgetClass = UPMECharacterCustomizationWidget::StaticClass();
	}

	if (!CustomizationWidget && CustomizationWidgetClass)
	{
		CustomizationWidget = CreateWidget<UPMECharacterCustomizationWidget>(
			GetOwningPlayer(),
			CustomizationWidgetClass);
	}

	if (CustomizationWidget)
	{
		CustomizationWidget->OnCustomizationClosed.RemoveAll(this);
		CustomizationWidget->OnCustomizationClosed.AddDynamic(
			this,
			&UPMEMainMenuWidget::HandleCustomizationClosed);
		SetVisibility(ESlateVisibility::Collapsed);
		CustomizationWidget->AddToViewport(200);
	}
}

void UPMEMainMenuWidget::HandleCustomizationClosed()
{
	CustomizationWidget = nullptr;
	SetVisibility(ESlateVisibility::Visible);
	ShowMainMenu();
}

void UPMEMainMenuWidget::HandleCoop()
{
	ShowConnectionMenu(EPMEPlayMode::Coop);
}

void UPMEMainMenuWidget::HandleVersus()
{
	ShowConnectionMenu(EPMEPlayMode::Versus);
}

void UPMEMainMenuWidget::HandleHost()
{
	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		GameInstance->HostMultiplayer(SelectedMultiplayerMode);
	}
}

void UPMEMainMenuWidget::HandleJoin()
{
	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		const FString Address = AddressBox ? AddressBox->GetText().ToString() : FString();
		GameInstance->JoinMultiplayer(Address, SelectedMultiplayerMode);
	}
}

void UPMEMainMenuWidget::HandleBackToMain()
{
	ShowMainMenu();
}

void UPMEMainMenuWidget::HandleBackToModes()
{
	ShowMultiplayerModes();
}

void UPMEMainMenuWidget::HandleSpanish()
{
	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		GameInstance->SetLanguage(EPMELanguage::Spanish);
	}
}

void UPMEMainMenuWidget::HandleEnglish()
{
	if (UPMEGameInstance* GameInstance = GetGameInstance<UPMEGameInstance>())
	{
		GameInstance->SetLanguage(EPMELanguage::English);
	}
}

void UPMEMainMenuWidget::HandleConnectionError(const FText& ErrorMessage)
{
	if (StatusText)
	{
		StatusText->SetText(ErrorMessage);
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.35f, 0.35f, 1.0f)));
	}
}

void UPMEMainMenuWidget::HandleLanguageChanged(const EPMELanguage Language)
{
	RefreshCurrentScreen();
}
