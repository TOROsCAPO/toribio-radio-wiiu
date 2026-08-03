# Toribio Radio para Wii U

Reproductor de radio por Internet para Wii U, creado con devkitPro, devkitPPC y WUT y empaquetado como canal `.wuhb` para Aroma.

Versión publicada: **0.18.0-beta.2**.

> Proyecto homebrew comunitario y no oficial. No está afiliado, autorizado ni patrocinado por Nintendo, las emisoras, Radio Browser o los proyectos de las bibliotecas utilizadas.

## Estado

La aplicación está en **beta pública candidata**. Fue probada en una Wii U real con Aroma y en Cemu, pero todavía necesita pruebas prolongadas antes de una versión 1.0.

Funciones disponibles:

- Interfaz independiente en la televisión y el GamePad.
- Emisoras de Uruguay, Argentina, Brasil, Perú, Colombia, Chile, Ecuador, Bolivia, Paraguay, Venezuela, España y México.
- Catálogo dinámico obtenido de Radio Browser.
- Eliminación de entradas duplicadas y filtros Todas, FM, AM y Otras.
- Reproducción de streams MP3 compatibles.
- Búfer preventivo de cuatro segundos y recarga automática ante variaciones de red.
- Controles con GamePad: A reproducir, Y pausa, X detener, L/R cambiar y B volver.
- Fondo e identidad visual originales de Toribio Tecnologic.

## Limitaciones conocidas

- Sólo se decodifica MP3. AAC, HLS, YouTube y formatos protegidos no están soportados.
- Algunas emisoras cambian su enlace, restringen países o dejan de emitir temporalmente.
- El filtro FM/AM depende del nombre publicado por cada emisora y puede clasificar algunas como Otras.
- La beta actual no solicita cuentas ni contraseñas. No debe utilizarse con streams privados o credenciales.
- Debido a una incompatibilidad de libcurl/mbedTLS en Wii U, esta beta cifra HTTPS pero no valida temporalmente el certificado del servidor. No introduzcas credenciales ni utilices streams privados.

## Instalación en Aroma

1. Descarga `Toribio-Radio-0.18.0-beta.2-SD.zip` de [Releases](https://github.com/TOROsCAPO/toribio-radio-wiiu/releases), o el archivo `.wuhb` si prefieres instalarlo manualmente.
2. Elimina cualquier copia anterior para evitar iconos duplicados.
3. Copia el archivo con esta ruta:

   ```text
   sd:/wiiu/apps/toribio-radio.wuhb
   ```

4. Inserta la SD, inicia Aroma y abre **Toribio Radio** desde el menú de Wii U.

La aplicación no escribe en la NAND ni instala títulos del sistema. Aroma ejecuta el paquete directamente desde la tarjeta SD.

## Compilación en Windows

Instala el entorno gráfico de devkitPro, abre **devkitPro MSYS2** y ejecuta:

```bash
pacman -Syu
pacman -S --needed wiiu-dev wiiu-curl ppc-mpg123 wiiu-sdl2 ppc-libjpeg-turbo
```

Desde la raíz del repositorio:

```bash
chmod +x scripts/*.sh
./scripts/build.sh
./scripts/package.sh
```

Los archivos RPX se generan en `build/` y los paquetes instalables en `dist/`.

## Datos y privacidad

La aplicación consulta Radio Browser para obtener nombres y enlaces, y luego se conecta directamente al servidor público de la emisora seleccionada. No incorpora analítica, cuentas ni publicidad. Consulta [PRIVACY.md](PRIVACY.md) para conocer qué información técnica pueden recibir esos servidores.

Los datos del catálogo de Radio Browser se publican en dominio público. Los programas y contenidos emitidos siguen perteneciendo a sus respectivos titulares.

## Avisos, bajas y soporte

Los errores técnicos y las solicitudes relacionadas con una emisora deben abrirse mediante los formularios de Issues de GitHub. Una solicitud válida puede provocar el bloqueo o retiro de una entrada en una versión posterior.

Las donaciones, cuando se habiliten, apoyarán únicamente el desarrollo, las pruebas y la documentación de la aplicación. No compran acceso a emisoras ni derechos sobre sus contenidos.

## Apoyar el proyecto

Si Toribio Radio te resulta útil, puedes [apoyar voluntariamente el desarrollo mediante PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=diego_del_mar%40hotmail.com&item_name=Desarrollo+de+Toribio+Radio+para+Wii+U&currency_code=USD).

La aplicación seguirá siendo gratuita y de código abierto. Una contribución no compra emisoras, contenidos, funciones exclusivas, prioridad de soporte ni derechos sobre el proyecto. PayPal procesa el pago directamente y puede aplicar sus propias comisiones o conversión de moneda.

## Licencias

El código propio está disponible bajo licencia MIT. El ejecutable también utiliza componentes de terceros con sus propias condiciones. Lee [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) y la carpeta `third_party/licenses/` antes de redistribuir una compilación.

Wii U y Nintendo son marcas de Nintendo. Aroma, WUT, Radio Browser y las bibliotecas mencionadas pertenecen a sus respectivos proyectos y autores.
