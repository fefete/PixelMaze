# Audio integration

## Música

- Menú: `BGM_Menu_80sSoft_Loop.wav`.
- Nivel: `BGM_Maze_BreathingMaze_Loop.wav`, correspondiente a la Opción B elegida.
- Las dos pistas se importan con `Looping` activado.
- La música se reproduce localmente desde los PlayerController y se destruye con el cambio de mapa.

## SFX

- Pasos del jugador por distancia recorrida.
- Pasos del enemigo por distancia recorrida sobre su transform replicado.
- Vocalización enemiga aleatoria cada 4–9 segundos.
- Contacto enemigo/jugador mediante Client RPC solo para el jugador afectado.
- Victoria para Single/Co-Op y ganador de Versus.
- Derrota para el jugador que pierde la ronda de Versus.

## Blueprints

Todas las referencias, volúmenes, intervalos y distancias relevantes están expuestas en Class Defaults de los controladores, personaje y enemigo.
