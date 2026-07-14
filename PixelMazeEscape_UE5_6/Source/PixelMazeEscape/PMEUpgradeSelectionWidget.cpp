#include "PMEUpgradeSelectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "PMELocalizationLibrary.h"
#include "PMEPlayerController.h"

TSharedRef<SWidget> UPMEUpgradeSelectionWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) BuildWidgetTree();
	return Super::RebuildWidget();
}

void UPMEUpgradeSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget) BuildWidgetTree();
	if (ChoiceButtons[0]) ChoiceButtons[0]->OnClicked.AddDynamic(this, &UPMEUpgradeSelectionWidget::Choose0);
	if (ChoiceButtons[1]) ChoiceButtons[1]->OnClicked.AddDynamic(this, &UPMEUpgradeSelectionWidget::Choose1);
	if (ChoiceButtons[2]) ChoiceButtons[2]->OnClicked.AddDynamic(this, &UPMEUpgradeSelectionWidget::Choose2);
	RefreshText();
}

void UPMEUpgradeSelectionWidget::BuildWidgetTree()
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("UpgradeBorder"));
	Border->SetPadding(FMargin(32));
	Border->SetBrushColor(FLinearColor(0.02f, 0.01f, 0.06f, 0.96f));
	WidgetTree->RootWidget = Border;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("UpgradeBox"));
	Border->SetContent(Box);
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeTitle"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 28.0f;
	TitleText->SetFont(TitleFont);
	Box->AddChildToVerticalBox(TitleText);
	for (int32 I = 0; I < 3; ++I)
	{
		ChoiceButtons[I] = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),
		                                                        *FString::Printf(TEXT("ChoiceButton%d"), I));
		ChoiceTexts[I] = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
		                                                         *FString::Printf(TEXT("ChoiceText%d"), I));
		ChoiceTexts[I]->SetJustification(ETextJustify::Center);
		ChoiceTexts[I]->SetAutoWrapText(true);
		FSlateFontInfo ChoiceFont = ChoiceTexts[I]->GetFont();
		ChoiceFont.Size = 20.0f;
		ChoiceTexts[I]->SetFont(ChoiceFont);
		ChoiceButtons[I]->AddChild(ChoiceTexts[I]);
		UVerticalBoxSlot* VSlot = Box->AddChildToVerticalBox(ChoiceButtons[I]);
		VSlot->SetPadding(FMargin(0, 12));
		VSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UPMEUpgradeSelectionWidget::ConfigureChoices(const TArray<EPMEUpgradeType>& InChoices)
{
	Choices = InChoices;
	RefreshText();
}

void UPMEUpgradeSelectionWidget::RefreshText()
{
	if (TitleText)TitleText->SetText(UPMELocalizationLibrary::GetText(TEXT("Upgrade.Title")));
	for (int32 I = 0; I < 3; ++I)
	{
		if (!ChoiceTexts[I])continue;
		if (Choices.IsValidIndex(I))
		{
			const FText Name = UPMELocalizationLibrary::GetText(PixelMazeUpgrade::ToNameKey(Choices[I]));
			const FText Desc = UPMELocalizationLibrary::GetText(PixelMazeUpgrade::ToDescriptionKey(Choices[I]));
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Name"), Name);
			Arguments.Add(TEXT("Description"), Desc);
			ChoiceTexts[I]->SetText(FText::Format(UPMELocalizationLibrary::GetText(TEXT("Upgrade.OptionFormat")),
			                                      Arguments));
			ChoiceButtons[I]->SetIsEnabled(true);
		}
		else
		{
			ChoiceTexts[I]->SetText(FText::GetEmpty());
			ChoiceButtons[I]->SetIsEnabled(false);
		}
	}
}

void UPMEUpgradeSelectionWidget::SubmitChoice(int32 Index)
{
	if (!Choices.IsValidIndex(Index))return;
	if (APMEPlayerController* PC = Cast<APMEPlayerController>(GetOwningPlayer()))PC->
		SubmitUpgradeChoice(Choices[Index]);
}

void UPMEUpgradeSelectionWidget::Choose0() { SubmitChoice(0); }
void UPMEUpgradeSelectionWidget::Choose1() { SubmitChoice(1); }
void UPMEUpgradeSelectionWidget::Choose2() { SubmitChoice(2); }
