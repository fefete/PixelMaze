# Enemigos con visión, persecución y regreso

## Comportamiento añadido

`APMEPatrolEnemy` utiliza ahora tres estados:

- `Patrol`: movimiento ping-pong por la ruta determinista.
- `Chase`: persecución del jugador visible con una distancia máxima medida en tiles recorridos.
- `ReturnToPatrol`: regreso al punto exacto donde empezó la persecución y reanudación de la ruta previa.

## Detección

- Comprobación autoritativa únicamente en servidor.
- Radio configurable en tiles.
- Visión de 360 grados.
- `LineTraceSingleByChannel` sobre `ECC_Visibility` para que las paredes bloqueen la detección.
- Selección del jugador visible más cercano en Co-Op y Versus.
- Los jugadores que ya han alcanzado la meta no son objetivos válidos.

## Persecución

- El presupuesto se descuenta con la distancia física realmente recorrida.
- El camino hasta la celda actual del jugador se calcula con BFS sobre las casillas transitables.
- La ruta se actualiza a un intervalo configurable para seguir a un objetivo móvil.
- El enemigo no atraviesa paredes ni toma atajos diagonales al empezar entre dos centros de tile.
- Si toca al jugador durante la persecución, lo reinicia y comienza el regreso inmediatamente.

## Regreso

- Se conserva la localización exacta, waypoint e dirección de patrulla del momento de detección.
- El enemigo vuelve por un camino válido del laberinto.
- Al llegar restaura el waypoint anterior y continúa patrullando.
- Durante el regreso no puede iniciar una nueva persecución.

## Variables Blueprint en `PMEGameModeBase`

- `Enemy Sight Radius In Tiles` (7 por defecto).
- `Enemy Chase Distance In Tiles` (8 por defecto).
- `Enemy Chase Movement Speed` (245 cm/s por defecto).
- `Enemy Return Movement Speed` (195 cm/s por defecto).
- `Enemy Sight Check Interval` (0,10 s por defecto).
- `Enemy Chase Path Refresh Interval` (0,15 s por defecto).

## Blueprint de enemigo

`PMEPatrolEnemy` expone además:

- `Sight Radius In Tiles`.
- `Chase Distance In Tiles`.
- `Chase Movement Speed`.
- `Return Movement Speed`.
- `Sight Check Interval`.
- `Chase Path Refresh Interval`.
- `Draw Sight Debug`.
- `Get Behaviour State`.
- `Get Remaining Chase Distance In Tiles`.
