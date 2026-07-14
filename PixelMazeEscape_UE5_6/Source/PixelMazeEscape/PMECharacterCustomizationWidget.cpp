#include "PMECharacterCustomizationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"
#include "PMECharacterCustomizationSubsystem.h"
#include "PMELocalizationLibrary.h"

void UPMECharacterCustomizationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildRootLayout();
}

TSharedRef<SWidget> UPMECharacterCustomizationWidget::RebuildWidget()
{
	BuildRootLayout();
	return Super::RebuildWidget();
}

void UPMECharacterCustomizationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UPMECharacterCustomizationSubsystem* Customization =
		GetGameInstance()->GetSubsystem<
			UPMECharacterCustomizationSubsystem>())
	{
		Customization->OnProfileChanged.RemoveAll(this);
		Customization->OnProfileChanged.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleProfileChanged);

		Definitions = Customization->GetCharacterDefinitions();
		const FName SelectedId =
			Customization->GetSelectedCharacterId();

		const int32 SelectedIndex = Definitions.IndexOfByPredicate(
			[SelectedId](
			const FPMECharacterCosmeticDefinition& Definition)
			{
				return Definition.CharacterId == SelectedId;
			});

		PendingCharacterIndex =
			SelectedIndex == INDEX_NONE ? 0 : SelectedIndex;
	}

	RefreshCharacterOptions();
}

void UPMECharacterCustomizationWidget::NativeDestruct()
{
	if (UPMECharacterCustomizationSubsystem* Customization =
		GetGameInstance()->GetSubsystem<
			UPMECharacterCustomizationSubsystem>())
	{
		Customization->OnProfileChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UPMECharacterCustomizationWidget::BuildRootLayout()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(
			this,
			TEXT("WidgetTree"));
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas =
		WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("CustomizationRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background =
		WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("CustomizationBackground"));
	Background->SetBrushColor(
		FLinearColor(0.008f, 0.006f, 0.025f, 1.0f));

	UCanvasPanelSlot* BackgroundSlot =
		RootCanvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(
		FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UBorder* Panel =
		WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("CustomizationPanel"));
	Panel->SetBrushColor(
		FLinearColor(0.035f, 0.02f, 0.075f, 0.99f));
	Panel->SetPadding(FMargin(32.0f));

	UCanvasPanelSlot* PanelSlot =
		RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetSize(FVector2D(1100.0f, 760.0f));
	PanelSlot->SetPosition(FVector2D::ZeroVector);

	UVerticalBox* RootBox =
		WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("CustomizationRootBox"));
	Panel->SetContent(RootBox);

	UTextBlock* Title = CreateText(
		UPMELocalizationLibrary::GetText(
			TEXT("Customization.Title")),
		34,
		true);
	RootBox->AddChildToVerticalBox(Title);

	UTextBlock* Subtitle = CreateText(
		UPMELocalizationLibrary::GetText(
			TEXT("Customization.Subtitle")),
		15);
	RootBox->AddChildToVerticalBox(Subtitle);

	USpacer* HeaderSpacer =
		WidgetTree->ConstructWidget<USpacer>(
			USpacer::StaticClass());
	HeaderSpacer->SetSize(FVector2D(1.0f, 18.0f));
	RootBox->AddChildToVerticalBox(HeaderSpacer);

	UHorizontalBox* ContentRow =
		WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("CustomizationContent"));

	if (UVerticalBoxSlot* ContentSlot =
		RootBox->AddChildToVerticalBox(ContentRow))
	{
		ContentSlot->SetSize(
			FSlateChildSize(ESlateSizeRule::Fill));
	}

	CharacterGrid =
		WidgetTree->ConstructWidget<UUniformGridPanel>(
			UUniformGridPanel::StaticClass(),
			TEXT("CharacterGrid"));
	CharacterGrid->SetSlotPadding(FMargin(7.0f));

	if (UHorizontalBoxSlot* GridSlot =
		ContentRow->AddChildToHorizontalBox(CharacterGrid))
	{
		GridSlot->SetSize(
			FSlateChildSize(ESlateSizeRule::Fill));
		GridSlot->SetPadding(
			FMargin(0.0f, 0.0f, 24.0f, 0.0f));
	}

	UBorder* PreviewPanel =
		WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("PreviewPanel"));
	PreviewPanel->SetBrushColor(
		FLinearColor(0.07f, 0.035f, 0.13f, 1.0f));
	PreviewPanel->SetPadding(FMargin(22.0f));

	if (UHorizontalBoxSlot* PreviewSlot =
		ContentRow->AddChildToHorizontalBox(PreviewPanel))
	{
		PreviewSlot->SetSize(
			FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UVerticalBox* PreviewBox =
		WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("PreviewBox"));
	PreviewPanel->SetContent(PreviewBox);

	UTextBlock* PreviewTitle = CreateText(
		UPMELocalizationLibrary::GetText(
			TEXT("Customization.Preview")),
		18,
		true);
	PreviewBox->AddChildToVerticalBox(PreviewTitle);

	USpacer* PreviewTitleSpacer =
		WidgetTree->ConstructWidget<USpacer>(
			USpacer::StaticClass());
	PreviewTitleSpacer->SetSize(FVector2D(1.0f, 10.0f));
	PreviewBox->AddChildToVerticalBox(PreviewTitleSpacer);

	USizeBox* PreviewSize =
		WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass());
	PreviewSize->SetWidthOverride(CharacterPreviewSize);
	PreviewSize->SetHeightOverride(CharacterPreviewSize);

	PreviewImage =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("PreviewImage"));
	PreviewSize->SetContent(PreviewImage);
	PreviewBox->AddChildToVerticalBox(PreviewSize);

	SelectedNameText = CreateText(FText::GetEmpty(), 23, true);
	PreviewBox->AddChildToVerticalBox(SelectedNameText);

	RequirementText = CreateText(FText::GetEmpty(), 15);
	RequirementText->SetAutoWrapText(true);
	PreviewBox->AddChildToVerticalBox(RequirementText);

	USpacer* ButtonsSpacer =
		WidgetTree->ConstructWidget<USpacer>(
			USpacer::StaticClass());
	ButtonsSpacer->SetSize(FVector2D(1.0f, 18.0f));
	RootBox->AddChildToVerticalBox(ButtonsSpacer);

	UHorizontalBox* ButtonsRow =
		WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass());

	if (UVerticalBoxSlot* ButtonsRowSlot =
		RootBox->AddChildToVerticalBox(ButtonsRow))
	{
		ButtonsRowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UButton* ApplyButton =
		WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			TEXT("ApplyCharacterButton"));
	ApplyButton->SetBackgroundColor(
		FLinearColor(0.18f, 0.46f, 0.24f, 1.0f));
	ApplyButton->SetContent(CreateText(
		UPMELocalizationLibrary::GetText(
			TEXT("Customization.Apply")),
		18,
		true));
	ApplyButton->OnClicked.AddDynamic(
		this,
		&UPMECharacterCustomizationWidget::HandleApply);

	if (UHorizontalBoxSlot* ApplySlot =
		ButtonsRow->AddChildToHorizontalBox(ApplyButton))
	{
		ApplySlot->SetPadding(FMargin(8.0f));
	}

	UButton* BackButton =
		WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			TEXT("BackCharacterButton"));
	BackButton->SetBackgroundColor(
		FLinearColor(0.32f, 0.08f, 0.42f, 1.0f));
	BackButton->SetContent(CreateText(
		UPMELocalizationLibrary::GetText(
			TEXT("Customization.Back")),
		18));
	BackButton->OnClicked.AddDynamic(
		this,
		&UPMECharacterCustomizationWidget::HandleBack);

	if (UHorizontalBoxSlot* BackSlot =
		ButtonsRow->AddChildToHorizontalBox(BackButton))
	{
		BackSlot->SetPadding(FMargin(8.0f));
	}
}

void UPMECharacterCustomizationWidget::RefreshCharacterOptions()
{
	if (!CharacterGrid)
	{
		return;
	}

	CharacterGrid->ClearChildren();
	CharacterButtons.Reset();
	CharacterImages.Reset();
	CharacterLabels.Reset();

	if (Definitions.IsEmpty())
	{
		if (UPMECharacterCustomizationSubsystem* Customization =
			GetGameInstance()->GetSubsystem<
				UPMECharacterCustomizationSubsystem>())
		{
			Definitions = Customization->GetCharacterDefinitions();
		}
	}

	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		UButton* Button =
			CreateCharacterButton(Definitions[Index], Index);

		CharacterButtons.Add(Button);

		if (UUniformGridSlot* vSlot =
			CharacterGrid->AddChildToUniformGrid(
				Button,
				Index / 5,
				Index % 5))
		{
			vSlot->SetHorizontalAlignment(HAlign_Fill);
			vSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	RefreshSelectionDetails();
}

void UPMECharacterCustomizationWidget::RefreshSelectionDetails()
{
	if (!Definitions.IsValidIndex(PendingCharacterIndex))
	{
		return;
	}

	UPMECharacterCustomizationSubsystem* Customization =
		GetGameInstance()->GetSubsystem<
			UPMECharacterCustomizationSubsystem>();
	if (!Customization)
	{
		return;
	}

	const FPMECharacterCosmeticDefinition& Definition =
		Definitions[PendingCharacterIndex];
	const bool bUnlocked =
		Customization->IsCharacterUnlocked(
			Definition.CharacterId);

	if (SelectedNameText)
	{
		SelectedNameText->SetText(
			UPMELocalizationLibrary::GetText(
				Definition.DisplayNameTextKey));
	}

	if (PreviewImage)
	{
		SetPixelArtImage(
			PreviewImage,
			LoadPreviewTexture(Definition),
			FVector2D(
				CharacterPreviewSize,
				CharacterPreviewSize));

		// Locked characters remain visible as a preview, but are dimmed
		// enough to communicate that they cannot be equipped yet.
		PreviewImage->SetColorAndOpacity(
			bUnlocked
				? FLinearColor::White
				: FLinearColor(0.48f, 0.48f, 0.48f, 1.0f));
	}

	if (RequirementText)
	{
		if (bUnlocked)
		{
			RequirementText->SetText(
				UPMELocalizationLibrary::GetText(
					TEXT("Customization.Unlocked")));
			RequirementText->SetColorAndOpacity(
				FSlateColor(
					FLinearColor(0.40f, 1.0f, 0.52f, 1.0f)));
		}
		else
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(
				TEXT("Requirement"),
				UPMELocalizationLibrary::GetText(
					PixelMazeAchievement::ToTextKey(
						Definition.RequiredAchievement)));

			RequirementText->SetText(
				FText::Format(
					UPMELocalizationLibrary::GetText(
						TEXT("Customization.UnlockRequirement")),
					Arguments));
			RequirementText->SetColorAndOpacity(
				FSlateColor(
					FLinearColor(1.0f, 0.55f, 0.30f, 1.0f)));
		}
	}

	for (int32 Index = 0;
	     Index < CharacterButtons.Num();
	     ++Index)
	{
		if (!CharacterButtons[Index])
		{
			continue;
		}

		const bool bSelected =
			Index == PendingCharacterIndex;
		CharacterButtons[Index]->SetBackgroundColor(
			bSelected
				? FLinearColor(0.55f, 0.16f, 0.72f, 1.0f)
				: FLinearColor(0.20f, 0.07f, 0.32f, 1.0f));
	}
}

void UPMECharacterCustomizationWidget::SelectCharacterByIndex(
	const int32 CharacterIndex)
{
	if (!Definitions.IsValidIndex(CharacterIndex))
	{
		return;
	}

	PendingCharacterIndex = CharacterIndex;
	RefreshSelectionDetails();
}

void UPMECharacterCustomizationWidget::CloseWidget(
	const bool bApplySelection)
{
	if (bApplySelection &&
		Definitions.IsValidIndex(PendingCharacterIndex))
	{
		if (UPMECharacterCustomizationSubsystem* Customization =
			GetGameInstance()->GetSubsystem<
				UPMECharacterCustomizationSubsystem>())
		{
			Customization->SelectCharacter(
				Definitions[PendingCharacterIndex].CharacterId);
		}
	}

	OnCustomizationClosed.Broadcast();
	RemoveFromParent();
}

UTexture2D*
UPMECharacterCustomizationWidget::LoadPreviewTexture(
	const FPMECharacterCosmeticDefinition& Definition) const
{
	UTexture2D* Texture =
		Definition.PreviewTexture.LoadSynchronous();

	if (!Texture)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Character customization: preview texture is missing for '%s'."),
			*Definition.CharacterId.ToString());
	}

	return Texture;
}

void UPMECharacterCustomizationWidget::SetPixelArtImage(
	UImage* ImageWidget,
	UTexture2D* Texture,
	const FVector2D& DesiredSize) const
{
	if (!ImageWidget)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = DesiredSize;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.SetResourceObject(Texture);

	ImageWidget->SetBrush(Brush);
	ImageWidget->SetVisibility(
		Texture
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden);
}

UTextBlock* UPMECharacterCustomizationWidget::CreateText(
	const FText& Text,
	const int32 FontSize,
	const bool bHeading)
{
	UTextBlock* TextBlock =
		WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass());

	TextBlock->SetText(Text);
	TextBlock->SetJustification(ETextJustify::Center);
	TextBlock->SetColorAndOpacity(
		FSlateColor(
			bHeading
				? FLinearColor(0.98f, 0.77f, 0.16f, 1.0f)
				: FLinearColor(0.83f, 0.96f, 0.92f, 1.0f)));

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

UButton* UPMECharacterCustomizationWidget::CreateCharacterButton(
	const FPMECharacterCosmeticDefinition& Definition,
	const int32 CharacterIndex)
{
	UButton* Button =
		WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass());
	Button->SetBackgroundColor(
		FLinearColor(0.20f, 0.07f, 0.32f, 1.0f));

	UVerticalBox* OptionBox =
		WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass());
	Button->SetContent(OptionBox);

	USizeBox* ImageSize =
		WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass());
	ImageSize->SetWidthOverride(CharacterThumbnailSize);
	ImageSize->SetHeightOverride(CharacterThumbnailSize);

	UImage* Image =
		WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass());
	SetPixelArtImage(
		Image,
		LoadPreviewTexture(Definition),
		FVector2D(
			CharacterThumbnailSize,
			CharacterThumbnailSize));
	ImageSize->SetContent(Image);
	OptionBox->AddChildToVerticalBox(ImageSize);
	CharacterImages.Add(Image);

	UTextBlock* Label = CreateText(
		UPMELocalizationLibrary::GetText(
			Definition.DisplayNameTextKey),
		14);
	OptionBox->AddChildToVerticalBox(Label);
	CharacterLabels.Add(Label);

	UPMECharacterCustomizationSubsystem* Customization =
		GetGameInstance()->GetSubsystem<
			UPMECharacterCustomizationSubsystem>();

	const bool bUnlocked =
		Customization &&
		Customization->IsCharacterUnlocked(
			Definition.CharacterId);

	if (!bUnlocked)
	{
		Image->SetColorAndOpacity(
			FLinearColor(0.14f, 0.14f, 0.14f, 1.0f));

		UTextBlock* LockedText = CreateText(
			UPMELocalizationLibrary::GetText(
				TEXT("Customization.Locked")),
			12);
		LockedText->SetColorAndOpacity(
			FSlateColor(
				FLinearColor(1.0f, 0.45f, 0.30f, 1.0f)));
		OptionBox->AddChildToVerticalBox(LockedText);
	}

	switch (CharacterIndex)
	{
	case 0:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter01);
		break;
	case 1:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter02);
		break;
	case 2:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter03);
		break;
	case 3:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter04);
		break;
	case 4:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter05);
		break;
	case 5:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter06);
		break;
	case 6:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter07);
		break;
	case 7:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter08);
		break;
	case 8:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter09);
		break;
	case 9:
		Button->OnClicked.AddDynamic(
			this,
			&UPMECharacterCustomizationWidget::HandleCharacter10);
		break;
	default:
		break;
	}

	return Button;
}

void UPMECharacterCustomizationWidget::HandleCharacter01()
{
	SelectCharacterByIndex(0);
}

void UPMECharacterCustomizationWidget::HandleCharacter02()
{
	SelectCharacterByIndex(1);
}

void UPMECharacterCustomizationWidget::HandleCharacter03()
{
	SelectCharacterByIndex(2);
}

void UPMECharacterCustomizationWidget::HandleCharacter04()
{
	SelectCharacterByIndex(3);
}

void UPMECharacterCustomizationWidget::HandleCharacter05()
{
	SelectCharacterByIndex(4);
}

void UPMECharacterCustomizationWidget::HandleCharacter06()
{
	SelectCharacterByIndex(5);
}

void UPMECharacterCustomizationWidget::HandleCharacter07()
{
	SelectCharacterByIndex(6);
}

void UPMECharacterCustomizationWidget::HandleCharacter08()
{
	SelectCharacterByIndex(7);
}

void UPMECharacterCustomizationWidget::HandleCharacter09()
{
	SelectCharacterByIndex(8);
}

void UPMECharacterCustomizationWidget::HandleCharacter10()
{
	SelectCharacterByIndex(9);
}

void UPMECharacterCustomizationWidget::HandleApply()
{
	if (!Definitions.IsValidIndex(PendingCharacterIndex))
	{
		return;
	}

	if (UPMECharacterCustomizationSubsystem* Customization =
		GetGameInstance()->GetSubsystem<
			UPMECharacterCustomizationSubsystem>())
	{
		if (Customization->IsCharacterUnlocked(
			Definitions[PendingCharacterIndex].CharacterId))
		{
			CloseWidget(true);
		}
	}
}

void UPMECharacterCustomizationWidget::HandleBack()
{
	CloseWidget(false);
}

void UPMECharacterCustomizationWidget::HandleProfileChanged()
{
	RefreshCharacterOptions();
}
