# Pruebas multijugador

## Prueba rápida en un solo PC

Usa dos procesos independientes. PIE con varios jugadores puede modificar el nombre del mapa y no representa exactamente una conexión IP real.

1. Abre **Play > Advanced Settings**.
2. Usa **Standalone Game**, o empaqueta Development.
3. Abre dos instancias.
4. Instancia A:
   - Multiplayer
   - Co-Op o Versus
   - Crear partida / Host
5. Instancia B:
   - Multiplayer
   - Selecciona cualquier modo; el modo real lo decide el servidor.
   - IP: `127.0.0.1:7777`
   - Conectar

## Prueba en LAN

En el host, ejecuta `ipconfig` y busca la IPv4 del adaptador activo. El cliente escribe, por ejemplo:

```text
192.168.1.25:7777
```

Permite la aplicación y UDP 7777 en el firewall del host.

## Prueba por Internet

- Redirige UDP 7777 en el router hacia la IPv4 privada del host.
- El cliente usa la IP pública del host.
- Como alternativa, usa una VPN LAN entre ambos equipos.

## Casos que deben pasar

### Límite

- El primer jugador crea la partida.
- El segundo entra correctamente.
- Una tercera conexión recibe `ServerFull`.

### Co-Op

- La ronda no empieza hasta que hay dos jugadores.
- Ambos aparecen en las mismas coordenadas.
- No se bloquean entre sí.
- El primero que llega queda inmovilizado.
- La ronda solo acaba cuando llega el segundo.

### Versus

- Ambos aparecen en las mismas coordenadas.
- La primera llegada bloquea la salida.
- Solo aumenta el marcador del ganador.
- La siguiente ronda conserva el marcador.

### Replicación

- Paredes y salida coinciden en host y cliente.
- Nivel, semilla y reloj aparecen en ambos HUD.
- Al reiniciar el host con `R`, ambos reciben el nuevo laberinto.
- `R` en el cliente no reinicia la ronda.

### Niebla

- Cada jugador ve únicamente el radio alrededor de su propio Pawn.
- Un jugador remoto desaparece al quedar fuera del radio.
- Cambiar `Set Fog Reveal Radius` en el cliente solo modifica su niebla local.

## Comandos útiles

Para mostrar estadísticas de red en consola:

```text
stat net
```

Para simular latencia en una instancia:

```text
Net PktLag=100
```

Para retirar la latencia simulada:

```text
Net PktLag=0
```
