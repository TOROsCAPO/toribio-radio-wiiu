# Publicación del proyecto

## Modelo recomendado

- Aplicación gratuita y código abierto.
- Donaciones estrictamente voluntarias para desarrollo y pruebas.
- Ninguna función, emisora o actualización queda bloqueada detrás de una donación.
- Los textos de donación no deben sugerir propiedad o representación de las emisoras.

## GitHub

1. Utilizar el repositorio público `https://github.com/TOROsCAPO/toribio-radio-wiiu`.
2. Subir el código sin `build/`, `dist/`, claves ni rutas personales.
3. Activar Issues y utilizar los formularios incluidos.
4. Crear una versión beta marcada como pre-release y adjuntar:
   - el `.wuhb`;
   - un ZIP de todo el código fuente;
   - el código fuente exacto de mpg123 utilizado para enlazar el binario;
   - comprobaciones SHA-256;
   - notas de cambios y limitaciones.
5. Añadir el enlace de donación solamente después de que la plataforma apruebe la cuenta receptora.

## GitHub Sponsors

GitHub Sponsors admite desarrolladores residentes en Uruguay. La cuenta debe solicitar y completar su perfil de patrocinio, incluidos los datos de cobro y fiscales requeridos por GitHub.

Después de la aprobación se puede crear `.github/FUNDING.yml` con:

```yaml
github: [USUARIO_DE_GITHUB]
```

No publiques el texto de ejemplo sin reemplazarlo por el usuario real.

## Homebrew App Store

Después de disponer de un repositorio público y una versión estable, se podrá presentar la aplicación siguiendo el formulario de envío de Homebrew App Store. El ZIP solicitado deberá instalar únicamente archivos dentro de la estructura homebrew de la SD.

## Impuestos y contabilidad

La plataforma de donaciones puede solicitar identidad fiscal y emitir reportes. El tratamiento en Uruguay depende de la situación personal, la frecuencia y el carácter de los ingresos; debe confirmarse con un contador antes de habilitar cobros regulares.
