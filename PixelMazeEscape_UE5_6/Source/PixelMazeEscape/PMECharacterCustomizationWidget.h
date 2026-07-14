#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PMECharacterCosmeticTypes.h"
#include "PMECharacterCustomizationWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class SWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPMECustomizationClosedSignature);

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API UPMECharacterCustomizationWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Customization")
	FPMECustomizationClosedSignature OnCustomizationClosed;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildRootLayout();
	void RefreshCharacterOptions();
	void RefreshSelectionDetails();
	void SelectCharacterByIndex(int32 CharacterIndex);
	void CloseWidget(bool bApplySelection);

	UTextBlock* CreateText(
		const FText& Text,
		int32 FontSize,
		bool bHeading = false);

	UButton* CreateCharacterButton(
		const FPMECharacterCosmeticDefinition& Definition,
		int32 CharacterIndex);

	UTexture2D* LoadPreviewTexture(
		const FPMECharacterCosmeticDefinition& Definition) const;

	void SetPixelArtImage(
		UImage* ImageWidget,
		UTexture2D* Texture,
		const FVector2D& DesiredSize) const;

	UFUNCTION()
	void HandleCharacter01();
	UFUNCTION()
	void HandleCharacter02();
	UFUNCTION()
	void HandleCharacter03();
	UFUNCTION()
	void HandleCharacter04();
	UFUNCTION()
	void HandleCharacter05();
	UFUNCTION()
	void HandleCharacter06();
	UFUNCTION()
	void HandleCharacter07();
	UFUNCTION()
	void HandleCharacter08();
	UFUNCTION()
	void HandleCharacter09();
	UFUNCTION()
	void HandleCharacter10();
	UFUNCTION()
	void HandleApply();
	UFUNCTION()
	void HandleBack();
	UFUNCTION()
	void HandleProfileChanged();

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> CharacterGrid;

	UPROPERTY()
	TObjectPtr<UImage> PreviewImage;

	/** Size of the selected character preview inside the details panel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Pixel Maze|Customization|Preview",
		meta=(AllowPrivateAccess="true", ClampMin="128.0", ClampMax="512.0"))
	float CharacterPreviewSize = 280.0f;

	/** Size of each character thumbnail in the selection grid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category="Pixel Maze|Customization|Preview",
		meta=(AllowPrivateAccess="true", ClampMin="64.0", ClampMax="192.0"))
	float CharacterThumbnailSize = 112.0f;

	UPROPERTY()
	TObjectPtr<UTextBlock> SelectedNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> RequirementText;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> CharacterButtons;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> CharacterImages;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> CharacterLabels;

	TArray<FPMECharacterCosmeticDefinition> Definitions;
	int32 PendingCharacterIndex = 0;
};
