# String Table y selección de idioma

## Objetivo

Eliminar las cadenas visibles hardcodeadas y permitir que cada usuario seleccione su idioma.

## Implementación

- Una tabla CSV registrada como `PixelMaze`:
  - `Content/Localization/StringTables/PixelMaze.csv`
  - 83 entradas en total.
  - Español e inglés incluidos.
- Nueva clase `UPMELocalizationLibrary` accesible desde C++ y Blueprint.
- Nueva enumeración Blueprint `EPMELanguage`.
- `UPMEGameInstance::SetLanguage` cambia la cultura y la guarda en la configuración del usuario.
- Evento Blueprint `OnLanguageChanged`.
- Selector Español/English en el menú principal.
- El menú reconstruye la pantalla actual al cambiar el idioma.
- El HUD consulta la tabla en cada dibujo y cambia inmediatamente.
- Los errores de red conservan su clave lógica para poder traducirse de nuevo al cambiar de idioma.
- Los números usan `FText::AsNumber`, respetando el formato numérico de la cultura activa.
- La carpeta CSV se incluye en builds empaquetadas mediante `DirectoriesToAlwaysStageAsUFS`.

## Claves y argumentos

El código solicita claves lógicas como `HUD.Time`. La librería añade automáticamente `en.` o `es.`.

Los textos variables usan argumentos nombrados:

- `{Level}`
- `{Time}`
- `{Seed}`
- `{Player}`
- `{Wins}`
- `{State}`

Esto permite cambiar el orden de la frase en cada idioma sin modificar C++.

## Archivos nuevos

- `Source/PixelMazeEscape/PMELocalizationLibrary.h`
- `Source/PixelMazeEscape/PMELocalizationLibrary.cpp`
- `Content/Localization/StringTables/PixelMaze.csv`

## Archivos principales modificados

- `PixelMazeEscape.cpp`
- `PMETypes.h`
- `PMEGameInstance.h/.cpp`
- `PMEMainMenuWidget.h/.cpp`
- `PMEHUD.h/.cpp`
- `PMEPlayerState.cpp`
- `PMEGameModeBase.cpp`
- `Config/DefaultGame.ini`
