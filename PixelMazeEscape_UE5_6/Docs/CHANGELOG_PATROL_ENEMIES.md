# Enemigos patrulleros

## Añadido

- Nueva clase `APMEPatrolEnemy`.
- Patrulla ping-pong sobre una ruta de casillas transitables.
- Movimiento autoritativo en servidor y replicación a clientes.
- Generación determinista según la semilla del laberinto.
- Zonas seguras alrededor de spawns y salida.
- Separación mínima entre enemigos.
- Incremento configurable de enemigos por nivel.
- Reinicio del jugador al tocar un enemigo.
- Cooldown de contacto para evitar múltiples teletransportes.
- Asset `SourceArt/enemy.png` y material automático `M_Enemy`.

## Archivos principales

- `Source/PixelMazeEscape/PMEPatrolEnemy.h`
- `Source/PixelMazeEscape/PMEPatrolEnemy.cpp`
- `Source/PixelMazeEscape/PMEGameModeBase.h`
- `Source/PixelMazeEscape/PMEGameModeBase.cpp`
- `Source/PixelMazeEscape/PMEMazeGenerator.h`
- `Content/Python/setup_pixelmaze_assets.py`
- `SourceArt/enemy.png`
