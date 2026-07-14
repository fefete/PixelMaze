# Arquitectura del gameplay loop GAS

## Autoridad y replicación

- El `GameMode` genera el laberinto, objetivos, pickups y enemigos exclusivamente en servidor.
- El `PlayerState` posee el ASC para conservar atributos, habilidades, inventario y mejoras durante respawns y cambios de nivel de la misma run.
- Los clientes reciben atributos, efectos, inventario, progreso, fase, puntuación y estadísticas por replicación.
- La niebla y el audio de resultado permanecen locales a cada cliente.

## Fases

1. `Exploring`: recoger objetos y activar sellos.
2. `Escape`: salida abierta y enemigos alertados.
3. `RoundResult`: resultado y sonido de victoria/derrota.
4. `UpgradeSelection`: selección individual de mejora.
5. `RunComplete`: resumen final.

## GAS

### Attributes

- Health / MaxHealth
- MoveSpeed
- FogRevealRadius
- NoiseMultiplier
- DetectionMultiplier
- SprintMultiplier

### Abilities

- Sprint: local predicted, termina al soltar input.
- Items: server initiated y consumen una carga validada por servidor.
- Revive: server initiated y solo funciona en Co-Op dentro del radio configurado.

### Gameplay Effects

- `PMEGE_DamageOne`: instantáneo, resta un corazón.
- `PMEGE_Sprint`: infinito mientras se mantiene la habilidad; aumenta ruido.
- `PMEGE_Torch`: duración configurable y aumento temporal de visión.

## Configuración Blueprint

Los valores globales se exponen en un Blueprint hijo de `PMEGameModeBase`. Las clases de habilidad se pueden sustituir desde un Blueprint hijo de `PMEPlayerState`. Las definiciones data-driven disponibles son `PMEDifficultyDefinition`, `PMEPickupDefinition` y `PMEUpgradeDefinition`.
