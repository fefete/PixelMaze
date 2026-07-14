# Pixel Maze Escape — Unreal Engine 5.6

Juego C++ de vista cenital y estilo pixel art donde uno o dos jugadores deben escapar de laberintos generados proceduralmente.

## Funciones incluidas

- Menú principal C++/UMG.
- Single Player.
- Multiplayer listen-server con conexión directa por IP.
- Máximo de dos jugadores.
- Co-Op: ambos salen en el mismo punto y los dos deben alcanzar la salida.
- Versus: cada jugador aparece en una zona distinta y gana quien llega primero.
- Marcador de victorias en Versus.
- Espera automática hasta que entra el segundo jugador.
- Laberinto determinista replicado mediante nivel + semilla.
- Niebla de guerra local con radio configurable desde Blueprint.
- Generador recursive backtracker / randomized DFS.
- Salida situada en la celda más distante mediante BFS.
- Cámara ortográfica, materiales Unlit y texturas pixel art 32×32.
- Suelo, paredes y niebla construidos con HISM.
- Segundo aspecto de jugador incluido.
- Enemigos pixel art con patrulla básica de ida y vuelta.
- Movimiento de enemigos autoritativo y replicado en multijugador.
- Contactar con un enemigo devuelve al jugador a su punto de inicio.
- Los enemigos detectan por línea de visión, persiguen una distancia limitada y regresan a su patrulla.
- BGM ochentera suave para el menú.
- Ambiente del laberinto “Breathing Maze” en loop, con pulso grave y ecos lejanos.
- Pasos, victoria, derrota, vocalización enemiga y contacto enemigo/jugador.

## Requisitos

- Unreal Engine **5.6**.
- Visual Studio 2022 con «Desarrollo de juegos con C++».
- Windows 10/11 para los scripts `.bat` incluidos.

## Puesta en marcha

1. Descomprime la carpeta.
2. Haz clic derecho en `PixelMazeEscape.uproject` y selecciona **Generate Visual Studio project files**.
3. Compila `PixelMazeEscapeEditor` en `Development Editor / Win64`.
4. Abre el proyecto. El script `Content/Python/init_unreal.py` importa los PNG y WAV, genera los materiales y configura las dos BGM como loops.
5. Pulsa **Play**. El proyecto comienza en el menú principal.

Si la importación no se ejecuta, usa **Tools > Execute Python Script** y abre:

```text
Content/Python/setup_pixelmaze_assets.py
```

## Probar el multijugador en el editor

La prueba más fiable para conexión directa es ejecutar dos procesos independientes:

1. Empaqueta o inicia una instancia Standalone.
2. En la primera, selecciona Multiplayer → Co-Op/Versus → Host.
3. En la segunda, selecciona el mismo modo, escribe `127.0.0.1:7777` y pulsa Conectar.

En dos equipos de la misma red, el cliente debe escribir la IPv4 privada del host, por ejemplo:

```text
192.168.1.25:7777
```

Para jugar por Internet es necesario permitir y redirigir el puerto UDP 7777 hacia el equipo anfitrión, o usar una solución de VPN LAN.

## Reglas

### Single Player

Al alcanzar la salida comienza automáticamente el siguiente nivel.

### Co-Op

- El host espera a que entre el segundo jugador.
- Ambos aparecen exactamente en la misma posición.
- Al llegar a la meta, cada jugador queda esperando.
- El siguiente nivel empieza cuando los dos han llegado.

### Versus

- El host espera a que entre el segundo jugador.
- Cada jugador aparece en una celda diferente, con rutas equilibradas hacia la salida.
- El primero en alcanzar la salida obtiene una victoria.
- Tras mostrar el resultado comienza una nueva ronda.

## Niebla de guerra configurable en Blueprint

La configuración predeterminada está en `APMEPlayerController`:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Fog of War")
float FogRevealRadiusInTiles = 5.0f;
```

Para modificarla sin tocar C++:

1. Crea un Blueprint hijo de `PMEPlayerController`.
2. Cambia **Fog Reveal Radius In Tiles** en Class Defaults.
3. Asigna ese Blueprint como Player Controller Class en un GameMode hijo de `PMEGameModeBase`.

También puedes crear un Blueprint hijo de `PMEFogOfWarActor`, cambiar `Reveal Radius In Tiles` y asignarlo en `Fog Of War Class` del PlayerController.

En ejecución están disponibles:

- `Set Fog Reveal Radius` en el PlayerController.
- `Set Reveal Radius In Tiles` en el actor de niebla.
- `Set Fog Enabled` para activar o desactivar la ocultación.

El valor se mide en baldosas, no en centímetros.

### Actualización de niebla sin parpadeos

La niebla mantiene una instancia HISM permanente por casilla. Al cambiar de baldosa no se ejecuta `ClearInstances()` ni se reconstruye toda la malla: únicamente se actualizan las casillas que entran o salen del radio visible y el lote se publica al renderizador una sola vez. El estado anterior permanece dibujado hasta que el nuevo está listo, evitando que el mapa aparezca completo durante un frame.

`Update Interval` sigue siendo configurable desde un Blueprint hijo de `PMEFogOfWarActor`, pero ya no controla una reconstrucción completa; solo la frecuencia con la que se comprueba si el jugador ha cambiado de casilla.

## Enemigos patrulleros

Cada ronda genera enemigos en posiciones deterministas a partir de la misma semilla del laberinto. El servidor elige celdas transitables alejadas de los puntos de aparición y de la salida, construye una ruta sin atravesar paredes y mueve cada enemigo entre sus extremos.

Al tocar a un jugador:

- En Single Player y Co-Op vuelve al inicio común.
- En Versus vuelve al punto inicial asignado a ese jugador.
- No se pierde la puntuación de rondas anteriores.
- Existe un pequeño cooldown para impedir múltiples reinicios por un único contacto.

Desde un Blueprint hijo de `PMEGameModeBase` se pueden configurar:

- `Spawn Patrol Enemies`.
- `Patrol Enemy Class`.
- `Base Enemy Count`.
- `Enemies Added Per Level`.
- `Maximum Enemy Count`.
- `Enemy Patrol Route Length In Tiles`.
- `Enemy Movement Speed`.
- `Enemy Waypoint Pause Duration`.
- `Enemy Safe Zone Radius In Tiles`.
- `Enemy Minimum Separation In Tiles`.
- `Enemy Contact Reset Cooldown`.

También puedes crear un Blueprint hijo de `PMEPatrolEnemy` para cambiar el tamaño, colisión, material, velocidad base, pausa o tipo de recorrido. La niebla de guerra se dibuja por encima de los enemigos, por lo que no son visibles fuera del radio revelado.

### Seguridad de las rutas de patrulla

La ruta obligatoria entre cada punto inicial y la salida se clasifica como **solo tránsito**. En Versus se utiliza la unión de los caminos de ambos jugadores. Un enemigo puede cruzar esos tiles, pero se aplican estas reglas:

- Nunca aparece inicialmente sobre un tile obligatorio.
- Los extremos de la patrulla siempre quedan en ramales laterales.
- Todo tramo sobre el camino obligatorio debe tener una entrada y una salida hacia tiles no obligatorios.
- No espera ni pausa sobre un tile de solo tránsito.
- Nunca cambia de dirección sobre ese camino.
- No inicia una persecución mientras entra, ocupa o abandona el tramo obligatorio.
- La cápsula del enemigo mantiene `Overlap` contra `Pawn`, por lo que tampoco crea un bloqueo físico sólido.
- Si no puede construirse una ruta que vuelva a salir del camino obligatorio, se genera un enemigo menos.

Desde un Blueprint hijo de `PMEGameModeBase` puedes configurar:

- `Allow Patrol Transit Across Progress Routes`: permite o impide que las patrullas crucen los caminos obligatorios. Está activado por defecto.
- `Enemy Progress Route Clearance In Tiles`: margen adicional alrededor del camino obligatorio. Con `0` se permiten los cruces seguros. Un valor de `1` también excluye los tiles laterales adyacentes y, por tanto, puede impedir esos cruces.
- `Prevent Patrol Route Overlap`: impide que dos patrullas compartan el mismo tile.

## Audio integrado

Los WAV fuente están en `SourceAudio/`. El importador crea automáticamente los siguientes `SoundWave` en `/Game/PixelMaze/Audio`:

- `SW_BGM_Menu_80sSoft_Loop`.
- `SW_BGM_Maze_BreathingMaze_Loop` — Opción B seleccionada.
- `SW_SFX_Player_Step`.
- `SW_SFX_Victory`.
- `SW_SFX_Defeat`.
- `SW_SFX_Enemy_Step`.
- `SW_SFX_Enemy_Vocal`.
- `SW_SFX_Enemy_HitPlayer`.

Comportamiento:

- El menú reproduce automáticamente la BGM ochentera.
- Cada cliente reproduce localmente el ambiente del laberinto, evitando duplicaciones en listen server.
- Los pasos del jugador dependen de la distancia realmente recorrida y no se disparan al ser teletransportado por un enemigo.
- Los pasos y vocalizaciones enemigas se calculan localmente usando la posición replicada del enemigo.
- En Single Player y Co-Op se reproduce victoria al terminar la ronda.
- En Versus el ganador reproduce victoria y el otro jugador derrota.
- El sonido de contacto con enemigo se envía únicamente al cliente afectado.

Las referencias y volúmenes son configurables desde Blueprints hijos de:

- `PMEMainMenuPlayerController`: `Menu Music` y `Menu Music Volume`.
- `PMEPlayerController`: ambiente, victoria, derrota, impacto y sus volúmenes.
- `PMEPlayerCharacter`: sonido, distancia, volumen y variación de pitch de los pasos.
- `PMEPatrolEnemy`: pasos, vocalización, intervalos y volúmenes.

Para reimportar todos los audios, ejecuta:

```python
import setup_pixelmaze_assets
setup_pixelmaze_assets.ensure_assets(force_rebuild=True)
```

## Controles

| Acción | Teclado | Mando |
|---|---|---|
| Movimiento | WASD / flechas | Stick izquierdo |
| Nuevo laberinto | R | Triangle / Y |
| Volver al menú | Escape | Start |

## Clases principales

- `PMEMainMenuGameMode`, `PMEMainMenuPlayerController`, `PMEMainMenuWidget`: interfaz y viajes.
- `PMEGameInstance`: Single, Host, Join por IP y retorno al menú.
- `PMEGameModeBase`: autoridad de partida, límite de jugadores y reglas.
- `PMEGameState`: estado replicado de la ronda.
- `PMEPlayerState`: índice, llegada y victorias.
- `PMEMazeGenerator`: generación determinista y HISM.
- `PMEFogOfWarActor`: ocultación local por radio.
- `PMEPatrolEnemy`: enemigo replicado con ruta de patrulla ping-pong.
- `PMEPlayerCharacter`: movimiento, cámara y aspecto por jugador.
- `PMEExitActor`: meta validada por el servidor.
- `PMEHUD`: reloj, estado, puntuación y resultados.

## Assets generados

`SourceArt/` contiene:

- `floor.png`
- `wall.png`
- `player.png`
- `player2.png`
- `enemy.png`
- `exit.png`
- `fog.png`
- `tileset_4x1.png`
- `icon.png`
- `title_screen.png`

El script crea `M_Floor`, `M_Wall`, `M_Player`, `M_Player2`, `M_Enemy`, `M_Exit` y `M_Fog`.

## Limitaciones deliberadas

- No incluye matchmaking, listado público de partidas, Steam Sessions ni EOS.
- La unión se realiza escribiendo la IP del anfitrión.
- La niebla es un radio de visibilidad actual; no conserva permanentemente las zonas exploradas.
- Se usa `/Engine/Maps/Entry` como nivel vacío para evitar mapas binarios obligatorios.

Consulta `Docs/ARCHITECTURE.md` para el flujo de red y replicación.

## Corrección del menú principal

El paquete incluye la corrección para widgets UMG creados completamente en C++: el árbol visual se construye antes de `RebuildWidget()`, por lo que el menú aparece al iniciar `/Engine/Maps/Entry`.

En el Output Log debe aparecer:

```text
Pixel Maze Escape: Main menu added to viewport.
```

## Spawn competitivo de Versus

En Versus, Player 1 y Player 2 aparecen en celdas diferentes. El generador busca dos posiciones con la misma distancia de camino hasta la salida y maximiza la separación entre ellas. Los mínimos de separación y distancia a la meta pueden ajustarse en un Blueprint hijo de `APMEMazeGenerator`.

## String Table e idiomas

Todas las cadenas visibles del menú, HUD, resultados y errores de conexión se obtienen de una única tabla CSV:

```text
Content/Localization/StringTables/PixelMaze.csv
```

La tabla usa claves con prefijo de idioma:

```text
en.Menu.SinglePlayer
es.Menu.SinglePlayer
en.HUD.PlayerWins
es.HUD.PlayerWins
```

El menú principal incluye los botones **Español** y **English**. La elección se aplica inmediatamente y Unreal la guarda en la configuración local del usuario, de modo que se conserva al reiniciar el juego. En multijugador, cada cliente mantiene su propio idioma; no se replica el idioma del host.

Los textos dinámicos utilizan argumentos nombrados. Por ejemplo:

```text
en.HUD.PlayerWins = PLAYER {Player} WINS!
es.HUD.PlayerWins = ¡GANA EL JUGADOR {Player}!
```

Desde Blueprint están disponibles:

- `Get Text` en `PMELocalizationLibrary`: obtiene una entrada usando la clave lógica sin prefijo, por ejemplo `Menu.Quit`.
- `Get Current Language` y `Get Current Language Code`.
- `Get Supported Languages`.
- `Set Language` y `Get Language` en `PMEGameInstance`.
- El evento `On Language Changed` en `PMEGameInstance`.

La tabla se registra en `PixelMazeEscape.cpp` con el identificador `PixelMaze`. `DefaultGame.ini` incluye la carpeta CSV en el empaquetado y prepara las culturas `en` y `es`.

Para añadir un idioma nuevo:

1. Añade sus filas a `PixelMaze.csv` usando un nuevo prefijo, por ejemplo `fr.`.
2. Añade el idioma a `EPMELanguage`.
3. Amplía la resolución de cultura en `PMELocalizationLibrary.cpp` y `PMEGameInstance::SetLanguage`.
4. Añade la cultura a `CulturesToStage` en `DefaultGame.ini`.

# Gameplay loop con Gameplay Ability System

Esta versión sustituye el loop directo de “buscar la salida” por una run de cinco laberintos:

```text
Explorar → recoger objetos → activar sellos → desbloquear la salida
→ sobrevivir a la fase de escape → elegir mejora → siguiente laberinto
```

## GAS

`APMEPlayerState` implementa `IAbilitySystemInterface` y contiene:

- `UPMEAbilitySystemComponent`, con replicación `Mixed`.
- `UPMEAttributeSet` para vida, velocidad, visión, ruido, detección y sprint.
- `UPMEInventoryComponent`, con dos huecos replicados.
- Mejoras acumulables y estadísticas de la run.

Las habilidades incluidas son:

- `UPMEGA_Sprint`.
- `UPMEGA_Torch`.
- `UPMEGA_MapPulse`.
- `UPMEGA_Decoy`.
- `UPMEGA_LightBomb`.
- `UPMEGA_MasterKey`.
- `UPMEGA_Revive`.

Los Gameplay Effects nativos incluidos son daño, sprint y antorcha. Las clases de habilidades se pueden sustituir desde un Blueprint hijo de `PMEPlayerState`.

## Objetivos y salida

- La salida comienza bloqueada.
- Cada nivel genera entre dos y cuatro sellos.
- Single Player y Co-Op comparten progreso.
- El último sello de Co-Op requiere que ambos jugadores permanezcan simultáneamente sobre él.
- En Versus cada jugador tiene sus propios sellos y solo puede utilizar la salida tras completar los suyos.
- Cada sello aumenta ligeramente la alerta enemiga.
- Al completar los objetivos empieza la fase `Escape`, se activa la salida y los enemigos aumentan su velocidad.

## Vida y derrota

- Cada jugador comienza con tres corazones.
- Un contacto enemigo aplica un Gameplay Effect de daño.
- Mientras quede vida, el jugador vuelve a su spawn y conserva los sellos activados.
- En Single Player, perder todos los corazones termina la run.
- En Co-Op, el jugador queda caído y puede ser reanimado con `E`; la derrota ocurre cuando ambos están caídos.
- En Versus, al perder todos los corazones gana el rival.

## Objetos

El inventario tiene dos huecos:

- Antorcha: aumenta temporalmente el radio visible.
- Pulso de mapa: revela temporalmente una gran zona.
- Señuelo: genera pulsos de ruido que atraen enemigos.
- Bomba de luz: aturde enemigos cercanos.
- Llave maestra: completa un sello pendiente.

## Enemigos y ruido

Los enemigos conservan patrulla, línea de visión, persecución limitada y regreso. Además incluyen:

- Estado `Investigate` para ir al origen de un ruido.
- Pasos del jugador, sprint, objetivos, señuelos, bombas, salida y reanimación como fuentes de ruido.
- Guardianes, oyentes y acechadores con valores distintos de visión, oído y persecución.
- Estado `Stunned` para la bomba de luz.
- Aumento de alerta al activar sellos y al abrir la salida.

## Mejoras entre laberintos

Después de cada nivel, cada jugador elige una de tres mejoras:

- Mayor visión.
- Pasos silenciosos.
- Corazón fuerte.
- Más velocidad.
- Sprint mejorado.
- Antorcha más duradera.
- Pulso de mapa más largo.
- Menor detección enemiga.
- Objeto inicial.

Las mejoras permanecen durante toda la run y son independientes para cada jugador.

## Controles nuevos

| Acción | Teclado | Mando |
|---|---|---|
| Sprint | Shift izquierdo | Pulsar stick izquierdo |
| Usar hueco 1 | 1 | X / Square |
| Usar hueco 2 | 2 | B / Circle |
| Reanimar compañero | E | A / Cross |

## Clases nuevas

- `PMEAbilitySystemComponent`.
- `PMEAttributeSet`.
- `PMEGameplayAbilities`.
- `PMEGameplayEffects`.
- `PMEInventoryComponent`.
- `PMEObjectManager` y `PMEObjectActor`.
- `PMEPickup`.
- `PMEDecoyActor`.
- `PMEUpgradeSelectionWidget`.
- `PMERunSubsystem`.
- `PMEDataAssets`.

Los textos nuevos del HUD, objetos, fases, modificadores y mejoras se encuentran en `Content/Localization/StringTables/PixelMaze.csv` en español e inglés.
