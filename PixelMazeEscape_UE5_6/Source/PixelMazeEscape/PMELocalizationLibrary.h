#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PMETypes.h"
#include "PMELocalizationLibrary.generated.h"

/**
 * Access point for every user-facing text in Pixel Maze Escape.
 *
 * The project uses one CSV string table named "PixelMaze". Entries are
 * prefixed with the selected two-letter culture code, for example:
 *   es.Menu.SinglePlayer
 *   en.Menu.SinglePlayer
 *
 * This makes the table easy to edit and allows the language to be switched
 * immediately at runtime without rebuilding any binary assets.
 */
UCLASS()
class PIXELMAZEESCAPE_API UPMELocalizationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns a localized String Table entry for the currently selected language. */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Localization")
	static FText GetText(FName Key);

	/** Returns the active supported language. Unsupported system cultures fall back to English. */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Localization")
	static EPMELanguage GetCurrentLanguage();

	/** Returns the two-letter culture code used by the String Table: "en" or "es". */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Localization")
	static FString GetCurrentLanguageCode();

	/** Languages currently provided by Content/Localization/StringTables/PixelMaze.csv. */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Localization")
	static TArray<EPMELanguage> GetSupportedLanguages();
};
