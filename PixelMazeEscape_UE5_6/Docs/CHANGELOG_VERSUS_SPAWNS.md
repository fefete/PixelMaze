# Hotfix: spawns separados en Versus

- Player 1 y Player 2 ya no usan la misma celda en Versus.
- BFS desde la meta para escoger posiciones con igual distancia de recorrido.
- Selección determinista y reproducible por semilla.
- Máxima separación espacial posible entre los dos puntos.
- Fallback para laberintos muy pequeños.
- Co-Op y Single Player conservan el spawn original compartido.
- Parámetros de separación expuestos a Blueprint en `APMEMazeGenerator`.
