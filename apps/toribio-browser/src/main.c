#include <toribio/app.h>
int main(void) {
    ToribioApp app = {
        .title = "Toribio Browser", .subtitle = "Cliente web experimental",
        .items = {"Inicio", "Favoritos", "Escribir URL", "Configuracion"}, .item_count = 4,
        .footer = "Demo UI; sin motor HTML/JS en esta version"
    };
    return toribio_run(&app);
}
