# Arquitectura

## Flujo de arranque

1. El mapa vacío `/Engine/Maps/Entry` arranca con `APMEMainMenuGameMode`.
2. `APMEMainMenuPlayerController` crea en C++ el widget `UPMEMainMenuWidget`.
3. Single Player abre el mismo mapa con `APMEGameModeBase` y `Mode=SinglePlayer`.
4. Host abre un listen server con `Mode=Coop` o `Mode=Versus` y la opción `listen`.
5. Join realiza `ClientTravel` a la IP y puerto indicados.
6. El GameMode espera a dos jugadores en los modos multijugador y después genera una semilla autoritativa.
7. El servidor replica nivel y semilla. Cada máquina reconstruye la misma matriz con `FRandomStream`.
8. La salida solo valida overlaps en el servidor.

## Modos de juego

### Single Player

Un jugador alcanza la salida. Tras la transición comienza un nivel mayor con una nueva semilla.

### Co-Op

- Máximo dos jugadores.
- Ambos aparecen exactamente en la misma posición inicial.
- Los personajes ignoran la colisión Pawn contra Pawn para no bloquearse.
- Cada jugador queda inmovilizado al alcanzar la salida.
- La ronda termina cuando los dos han escapado.

### Versus

- Máximo dos jugadores.
- Cada jugador aparece en una posición inicial distinta y equidistante de la salida cuando el laberinto lo permite.
- El primer jugador que alcanza la salida gana la ronda.
- `APMEPlayerState` replica el número de victorias.

## Autoridad y replicación

- `APMEGameModeBase`: solo existe en el servidor y decide generación, salida, ganador y cambios de ronda.
- `APMEGameState`: replica modo, nivel, semilla, reloj, espera y resultado.
- `APMEPlayerState`: replica índice de jugador, victorias y llegada a meta.
- `APMEMazeGenerator`: replica únicamente nivel y semilla; los HISM no se envían individualmente.
- `APMEExitActor`: replica posición base y estado visible, pero el overlap se procesa únicamente en autoridad.
- `APMEPlayerCharacter`: usa la replicación de movimiento de `ACharacter`.

## Representación de la cuadrícula

Cada celda lógica ocupa una posición impar de la matriz renderizada:

- celda lógica `(0,0)` → cuadrícula `(1,1)`
- celda lógica `(1,0)` → cuadrícula `(3,1)`
- la posición `(2,1)` es la pared intermedia que puede tallarse

El DFS aleatorio talla los pasillos y un BFS selecciona como meta la celda transitable más lejana.

## Niebla de guerra

`APMEFogOfWarActor` es local y no se replica. Cada `APMEPlayerController` crea su propia instancia y le asigna su Pawn local.

La niebla se representa mediante un HISM de baldosas negras situado por encima de las paredes. Solo se reconstruye cuando:

- el jugador cambia de celda;
- cambia la semilla o tamaño del laberinto;
- cambia el radio visible.

El radio está expuesto a Blueprint en dos puntos:

- `APMEPlayerController::FogRevealRadiusInTiles`.
- `APMEFogOfWarActor::RevealRadiusInTiles`.

También puede cambiarse en ejecución con `SetFogRevealRadius` o `SetRevealRadiusInTiles`.

## Menú

El menú se genera íntegramente desde C++/UMG y no necesita un Widget Blueprint binario. La jerarquía es:

- Single Player
- Multiplayer
  - Co-Op
    - Host
    - Join por IP
  - Versus
    - Host
    - Join por IP

## Red

La conexión es directa por IP usando el `IpNetDriver` estándar:

- puerto predeterminado: `7777`;
- red local: usar la IPv4 privada del host;
- Internet: abrir/redirigir UDP 7777 y usar la IP pública o una VPN LAN.

El servidor rechaza una tercera conexión en `PreLogin`.


## Patrullas y caminos de solo tránsito

`APMEGameModeBase` calcula mediante BFS la ruta obligatoria desde cada spawn hasta la salida. En Versus se toma la unión de las dos rutas. Esos tiles se entregan al generador de patrullas como `TransitOnlyPatrolCells`.

La búsqueda de patrulla es un DFS limitado y determinista que cumple estas invariantes:

- El punto inicial y el final de la ruta no pertenecen al conjunto de solo tránsito.
- La ruta no repite tiles y todos sus puntos son contiguos.
- Los tiles obligatorios solo pueden aparecer en posiciones internas.
- Por tanto, todo tramo obligatorio tiene una entrada y una salida lateral antes de que la ruta pueda aceptarse.
- Las rutas de distintos enemigos no comparten tiles cuando `Prevent Patrol Route Overlap` está activado.

`APMEPatrolEnemy` recibe una máscara paralela a sus waypoints. Al alcanzar un waypoint de solo tránsito asigna una pausa de cero segundos y continúa con el siguiente. Como los extremos no son de solo tránsito, el recorrido ping-pong nunca invierte allí.

La percepción se suspende mientras el enemigo entra, ocupa o abandona un tramo de solo tránsito. De esta forma una persecución no puede fijar como punto de retorno una posición situada en el corredor obligatorio.
