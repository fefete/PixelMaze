# Hotfix — niebla de guerra sin parpadeos

## Problema

La implementación anterior ejecutaba `ClearInstances()` cada vez que el jugador cambiaba de celda y volvía a añadir las casillas cubiertas. El render thread podía recibir temporalmente el HISM vacío y mostrar el laberinto completo durante una fracción de segundo.

## Solución

- Se crea una instancia de niebla permanente para cada casilla del mapa.
- Las instancias visibles conservan su posición sobre el laberinto.
- Las casillas descubiertas mantienen su índice, pero se desplazan bajo el mapa con una escala mínima.
- Al moverse el jugador solo se actualizan las casillas cuyo estado cambia.
- Todas las transformaciones se publican con una única llamada a `MarkRenderStateDirty()`.
- `ClearInstances()` solo se utiliza cuando cambia realmente la definición o el tamaño del laberinto, nunca durante el movimiento normal.

## Archivos modificados

- `Source/PixelMazeEscape/PMEFogOfWarActor.h`
- `Source/PixelMazeEscape/PMEFogOfWarActor.cpp`
- `README.md`
