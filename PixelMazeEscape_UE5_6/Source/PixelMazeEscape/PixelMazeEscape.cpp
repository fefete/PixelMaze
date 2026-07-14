#include "PixelMazeEscape.h"

#include "Internationalization/StringTableRegistry.h"
#include "Modules/ModuleManager.h"

class FPixelMazeEscapeModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		// Relative to the project's Content directory. The CSV is staged by
		// DefaultGame.ini so it also works in packaged builds.
		const FName TableId(TEXT("PixelMaze"));
		if (!FStringTableRegistry::Get().FindStringTable(TableId).IsValid())
		{
			LOCTABLE_FROMFILE_GAME(
				"PixelMaze",
				"PixelMaze",
				"Localization/StringTables/PixelMaze.csv");
		}
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPixelMazeEscapeModule, PixelMazeEscape, "PixelMazeEscape");
