# Patrol Route Safety

> **Actualización posterior:** este comportamiento fue ampliado por `CHANGELOG_PATROL_TRANSIT_ONLY.md`. El camino obligatorio ya no está completamente prohibido: ahora puede cruzarse como tramo de solo tránsito, sin pausas ni cambios de sentido.


## Problema corregido

Los enemigos podían generar rutas de patrulla sobre el único camino válido entre el punto de aparición de un jugador y la salida. Aunque la cápsula del enemigo usa overlap con Pawn, el contacto devuelve al jugador al inicio y podía convertir un corredor estrecho en un bloqueo funcional permanente.

## Solución

`APMEGameModeBase` calcula ahora mediante BFS la ruta transitable obligatoria entre cada punto inicial y la salida:

- Single Player y Co-Op: ruta desde el inicio común hasta la meta.
- Versus: unión de la ruta del Player 1 y la ruta del Player 2 hasta la meta.

Todas esas celdas se incluyen en `ForbiddenPatrolCells`. Tanto las posiciones iniciales de los enemigos como cada waypoint de sus patrullas quedan fuera de ese conjunto.

Como el generador crea un laberinto perfecto, existe una única ruta entre dos celdas. Reservar esa ruta garantiza que una patrulla no puede cerrar el camino necesario para completar el nivel.

## Reglas adicionales

- Dos patrullas no comparten tiles por defecto.
- Las zonas seguras de inicio y meta siguen respetándose.
- La seguridad tiene prioridad sobre la cantidad solicitada de enemigos.
- Si no existen suficientes ramales secundarios, el nivel genera menos enemigos.
- La persecución y el regreso a la patrulla se mantienen sin cambios.

## Propiedades Blueprint

En un Blueprint hijo de `PMEGameModeBase`:

- `Enemy Progress Route Clearance In Tiles`: añade un margen adicional alrededor de la ruta obligatoria. El valor `0` ya protege todos los tiles de la ruta. El valor `1` impide también patrullas en tiles transitables inmediatamente adyacentes.
- `Prevent Patrol Route Overlap`: evita que dos enemigos compartan tiles de patrulla.

## Archivos modificados

- `Source/PixelMazeEscape/PMEGameModeBase.h`
- `Source/PixelMazeEscape/PMEGameModeBase.cpp`
