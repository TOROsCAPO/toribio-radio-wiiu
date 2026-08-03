# Toribio Radio 0.18.0-beta.2

Corrección de compatibilidad de la primera beta pública, probada en Cemu.

## Corrección principal

- Soluciona el error `e77` producido al importar certificados con libcurl/mbedTLS.
- Fuerza IPv4 para evitar esperas de red de Cemu con direcciones IPv6.
- Usa `de1` como servidor principal de Radio Browser y `de2` como respaldo.
- Mantiene HTTPS cifrado en modo compatible, pero no valida temporalmente el certificado.
- Añade códigos de error de red visibles para facilitar el diagnóstico.

## Limitación de seguridad

No introduzcas credenciales ni utilices streams privados. La aplicación sólo consulta un catálogo público y reproduce audio público; la validación del certificado se restaurará cuando haya una solución compatible con el port de libcurl/mbedTLS para Wii U.

## Instalación

Descomprime `Toribio-Radio-0.18.0-beta.2-SD.zip` en la raíz de la tarjeta SD, o copia el WUHB en:

```text
sd:/wiiu/apps/toribio-radio.wuhb
```

Reemplaza la beta anterior; no conserves ambos archivos para evitar iconos duplicados.
