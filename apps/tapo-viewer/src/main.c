#include <toribio/app.h>
int main(void) {
    ToribioApp app = {
        .title = "Tapo Wii U Viewer", .subtitle = "Esqueleto RTSP/local",
        .items = {"Camara principal", "Camara secundaria", "Configuracion"}, .item_count = 3,
        .footer = "Demo UI; video y credenciales aun no implementados"
    };
    return toribio_run(&app);
}
