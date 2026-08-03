# Cambios de Wii U Radio

## 0.18.0-beta.1 — 2026-08-03

- Primera beta pública en GitHub.
- Se mantiene la reproducción al volver al selector de países.
- Se muestra la emisora activa y su país en TV y GamePad.
- Se valida HTTPS con un paquete de autoridades certificadoras incluido dentro del `.wuhb`.
- Se publican un paquete listo para SD, el código fuente, sumas SHA-256 y los materiales de licencia de mpg123.

## 0.18-reproduccion-persistente — 2026-08-03

- La emisora sigue reproduciendose al volver al selector de paises o al cambiar de catalogo.
- La esquina superior izquierda muestra la emisora activa y, debajo, su pais.
- La informacion permanece visible en TV y GamePad durante la navegacion.
- El boton X detiene el audio y limpia el indicador de emisora activa.

## 0.17-failover — 2026-08-03

- Se cambia el servidor principal al nombre actualmente publicado por Radio Browser.
- Si el servidor principal falla, el catalogo se reinicia y se consulta automaticamente en un servidor comunitario alternativo.
- Los errores de DNS, conexion y TLS se muestran con su causa real en vez de aparecer siempre como catalogo invalido.
- El cambio de servidor ocurre sin bloquear la interfaz ni requerir cambiar de pais manualmente.

## 0.16-catalogo — 2026-08-03

- Las respuestas HTTP de error, HTML o JSON ya no se interpretan como una emisora.
- Cada pagina del catalogo debe ser una lista M3U valida antes de modificar la lista visible.
- Una descarga incompleta se descarta y se reintenta dos veces con una conexion nueva y espera progresiva.
- Si Radio Browser no responde correctamente, la app muestra un error con A para reintentar y B para volver.
- Al cambiar de pais se crea una sesion de red nueva para evitar resultados pendientes del pais anterior.

## 0.15-buffer — 2026-08-03

- El audio espera cuatro segundos antes de comenzar para absorber variaciones del Wi-Fi.
- El búfer circular pasa de 512 KiB a 2 MiB y se reserva dinámicamente al abrir la aplicación.
- Si queda menos de un segundo de audio, la reproducción se pausa y recarga automáticamente.
- Las ráfagas de datos conservan el audio más reciente en vez de descartarlo silenciosamente.
- La descarga continúa actualizándose al volver desde las emisoras al selector FM/AM.
- Se amplían los bloques de red y la capacidad de vaciar datos pendientes del decodificador MP3.
- Se activa TCP keepalive para conexiones largas.

Esta versión reduce los microcortes, pero no puede ocultar una caída de Internet o del servidor que dure más que el búfer disponible.
