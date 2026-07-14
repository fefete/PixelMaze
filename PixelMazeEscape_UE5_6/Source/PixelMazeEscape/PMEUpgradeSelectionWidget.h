#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PMETypes.h"
#include "PMEUpgradeSelectionWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class PIXELMAZEESCAPE_API UPMEUpgradeSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureChoices(const TArray<EPMEUpgradeType>& InChoices);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;

private:
	void BuildWidgetTree();
	void RefreshText();
	void SubmitChoice(int32 Index);
	UFUNCTION()
	void Choose0();
	UFUNCTION()
	void Choose1();
	UFUNCTION()
	void Choose2();
	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY()
	TObjectPtr<UButton> ChoiceButtons[3];
	UPROPERTY()
	TObjectPtr<UTextBlock> ChoiceTexts[3];
	TArray<EPMEUpgradeType> Choices;
};
