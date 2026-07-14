# SourceAudio

WAV PCM 16-bit / 44.1 kHz utilizados por Pixel Maze Escape.

- `BGM_Menu_80sSoft_Loop.wav`: menú, loop de 60 s.
- `BGM_Maze_BreathingMaze_Loop.wav`: ambiente de laberinto seleccionado (Opción B), loop de 48 s.
- `SFX_Player_Step.wav`: paso del jugador.
- `SFX_Victory.wav`: final de ronda favorable.
- `SFX_Defeat.wav`: derrota local en Versus.
- `SFX_Enemy_Step.wav`: paso del enemigo.
- `SFX_Enemy_Vocal.wav`: vocalización periódica del enemigo.
- `SFX_Enemy_HitPlayer.wav`: contacto y reinicio del jugador.

El script `Content/Python/setup_pixelmaze_assets.py` importa estos archivos a `/Game/PixelMaze/Audio`.
