# Cambio: caminos obligatorios de solo tránsito

## Problema

Excluir completamente el camino obligatorio evitaba bloqueos, pero reducía demasiado la variedad de patrullas. Permitir waypoints normales sobre ese camino podía volver a dejar al jugador atrapado por pausas o cambios de dirección del enemigo.

## Solución

Los caminos desde cada spawn hasta la salida se clasifican ahora como `TransitOnlyPatrolCells`. Una patrulla puede cruzarlos, pero nunca puede comenzar ni terminar en ellos.

La generación usa una búsqueda DFS determinista y solo acepta una ruta cuando sus dos extremos están fuera del camino obligatorio. Por construcción, cualquier secuencia de tiles obligatorios queda encerrada entre una entrada y una salida lateral. Si la ruta entra en el camino y no encuentra una salida válida, se descarta.

## Comportamiento del enemigo

- Pausa de cero segundos en waypoints de solo tránsito.
- No puede invertir el sentido allí porque nunca son extremos.
- No ejecuta comprobaciones de visión mientras entra, ocupa o abandona el cruce.
- No puede guardar un punto de regreso de persecución dentro del camino obligatorio.
- La colisión con Pawn continúa configurada como overlap.

## Configuración Blueprint

En `PMEGameModeBase`:

- `Allow Patrol Transit Across Progress Routes`: activo por defecto. Si se desactiva, se recupera el comportamiento anterior y todo el camino obligatorio queda prohibido.
- `Enemy Progress Route Clearance In Tiles`: con cero permite cruces. Valores superiores reservan también los laterales próximos y pueden impedir que exista una salida válida.
- `Prevent Patrol Route Overlap`: evita compartir tiles entre enemigos.

## Archivos modificados

- `Source/PixelMazeEscape/PMEGameModeBase.h`
- `Source/PixelMazeEscape/PMEGameModeBase.cpp`
- `Source/PixelMazeEscape/PMEPatrolEnemy.h`
- `Source/PixelMazeEscape/PMEPatrolEnemy.cpp`
