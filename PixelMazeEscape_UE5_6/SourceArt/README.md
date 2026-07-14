# SourceArt

PNG fuente del proyecto. Las texturas de juego tienen una resolución nativa de 32×32 y pueden editarse en Aseprite, Krita, Photoshop o cualquier editor que conserve nearest-neighbor y transparencia.

`tileset_4x1.png` contiene, de izquierda a derecha: suelo, pared, jugador y salida.

El script `Content/Python/setup_pixelmaze_assets.py` importa las texturas de suelo, pared, jugadores, enemigo, salida y niebla, y crea sus materiales Unlit. El icono y la pantalla de título se incluyen como material de interfaz y branding.

`enemy.png` es el aspecto pixel art predeterminado de `APMEPatrolEnemy`. Puede sustituirse conservando el nombre y volviendo a ejecutar el script con `force_rebuild=True`.
