# Guía de ampliación del arte por IA

## Dirección visual

- Pixel art auténtico, resolución nativa baja.
- Vista cenital o frontal estricta según el asset.
- Paleta limitada: violeta oscuro, lavanda, verde menta y ámbar.
- Bordes duros, sin antialiasing, sin blur, sin degradados suaves.
- Fondo transparente para personajes, enemigos y objetos.
- Tamaño recomendado: 32×32 o 64×64 por frame.

## Tiles de mazmorra

> Seamless 32x32 pixel art dungeon tileset, top-down view, dark violet stone, lavender edge highlights, mint magical accents, strict limited palette, hard pixel edges, no antialiasing, no text, orthogonal grid, game-ready.

## Personaje

> 32x32 pixel art explorer character, top-down game sprite, amber hood and violet tunic, readable silhouette, four-direction movement frames, transparent background, strict sprite sheet grid, hard pixel edges, no antialiasing.

## Portal de salida

> 32x32 pixel art magical exit portal, square mint glow, dark violet frame, top-down game object, transparent background, four-frame looping animation, strict sprite sheet grid, hard pixel edges.

## Enemigo

> 32x32 pixel art maze guardian, purple stone creature with mint eyes, top-down game sprite, four-direction movement, limited palette, transparent background, strict sprite sheet grid, no antialiasing.

## Revisión manual necesaria

Las imágenes generativas pueden introducir tamaños de frame inconsistentes. Antes de importarlas:

1. Ajusta cada frame a una cuadrícula exacta.
2. Elimina píxeles semitransparentes.
3. Comprueba que el borde pueda repetirse sin cortes.
4. Mantén el pivote del personaje en el centro inferior o en el centro geométrico de todos los frames.
