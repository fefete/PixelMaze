#include "PMELocalizationLibrary.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"

namespace PixelMazeLocalization
{
	static const FName StringTableId(TEXT("PixelMaze"));
}

FText UPMELocalizationLibrary::GetText(const FName Key)
{
	const FString FullKey = FString::Printf(
		TEXT("%s.%s"),
		*GetCurrentLanguageCode(),
		*Key.ToString());

	return FText::FromStringTable(PixelMazeLocalization::StringTableId, FullKey);
}

EPMELanguage UPMELocalizationLibrary::GetCurrentLanguage()
{
	return GetCurrentLanguageCode().Equals(TEXT("es"), ESearchCase::IgnoreCase)
		       ? EPMELanguage::Spanish
		       : EPMELanguage::English;
}

FString UPMELocalizationLibrary::GetCurrentLanguageCode()
{
	const FString CultureName = FInternationalization::Get().GetCurrentCulture()->GetName();
	return CultureName.StartsWith(TEXT("es"), ESearchCase::IgnoreCase)
		       ? TEXT("es")
		       : TEXT("en");
}

TArray<EPMELanguage> UPMELocalizationLibrary::GetSupportedLanguages()
{
	return {EPMELanguage::English, EPMELanguage::Spanish};
}
