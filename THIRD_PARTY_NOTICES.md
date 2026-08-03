# Componentes de terceros

Toribio Radio combina código MIT propio con bibliotecas libres instaladas mediante devkitPro Pacman.

| Componente usado en la compilación probada | Versión | Licencia |
| --- | ---: | --- |
| WUT | 1.9.1 | zlib |
| SDL2 para Wii U | 2.32.10 | zlib |
| curl para Wii U | 8.7.1 | curl |
| mpg123 para PowerPC | 1.33.4 | LGPL-2.1-only |
| libjpeg-turbo para PowerPC | 3.1.4.1 | IJG y BSD-3-Clause |
| ca-certificates de MSYS2 | 20250419-1 | MPL y GPL (metadatos del paquete) |

Los textos suministrados por esas instalaciones se conservan en `third_party/licenses/`.

## Certificados HTTPS

`apps/radio/content/ca-bundle.crt` procede de `usr/ssl/cert.pem` del paquete oficial `ca-certificates 20250419-1` de MSYS2. Su SHA-256 es `50d055ee716f373c901c34d80b275d7f599a582d36b6f1596bad92d61d61012d`. El archivo reúne certificados raíz públicos utilizados exclusivamente para verificar servidores HTTPS; consulta `third_party/licenses/ca-certificates-NOTICE.txt` para la procedencia y los enlaces del proyecto.

## mpg123 y redistribución del ejecutable

El paquete `.wuhb` enlaza estáticamente mpg123. Cada publicación binaria debe incluir:

- un aviso visible de que utiliza mpg123 bajo LGPL 2.1;
- el texto completo de esa licencia;
- el código fuente completo de Toribio Radio y sus scripts de compilación;
- acceso al código fuente exacto de mpg123 utilizado, incluyendo cualquier modificación;
- los archivos fuente u objeto necesarios para que una persona pueda recompilar y volver a enlazar la aplicación con una versión modificada compatible de mpg123.

La publicación no impide la modificación para uso propio ni la ingeniería inversa necesaria para depurar esas modificaciones.

Copyright © 1995-2020 Michael Hipp y otros. mpg123 es software libre bajo LGPL 2.1.

## libjpeg-turbo

This software is based in part on the work of the Independent JPEG Group.

No se utiliza el nombre del Independent JPEG Group, de libjpeg-turbo ni de sus colaboradores para promocionar o respaldar este proyecto.

## Datos de Radio Browser

Radio Browser declara sus datos acumulados —nombres, etiquetas, enlaces, idiomas y países— como dominio público y ofrece una API abierta para aplicaciones. Toribio Radio no incorpora el software del servidor de Radio Browser.

## Alcance

La licencia MIT del archivo `LICENSE` cubre solamente el código y los recursos originales del proyecto. No reemplaza ni amplía las licencias de estos componentes, de las marcas o de los contenidos emitidos.
