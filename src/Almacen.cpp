#include "Almacen.h"

#include <Preferences.h>

namespace almacen {
namespace {
constexpr const char* kEspacio = "bichito";
constexpr const char* kClave   = "estado";
}  // namespace

bool cargar(bichito::Guardado& destino) {
    Preferences p;
    if (!p.begin(kEspacio, /*readOnly=*/true)) return false;

    bichito::Guardado leido;
    const size_t bytes = p.getBytes(kClave, &leido, sizeof(leido));
    p.end();

    if (bytes != sizeof(leido)) return false;
    if (leido.version != bichito::kVersionGuardado) return false;

    destino = leido;
    return true;
}

void guardar(const bichito::Guardado& g) {
    Preferences p;
    if (!p.begin(kEspacio, /*readOnly=*/false)) return;
    p.putBytes(kClave, &g, sizeof(g));
    p.end();
}

void borrar() {
    Preferences p;
    if (!p.begin(kEspacio, /*readOnly=*/false)) return;
    p.clear();
    p.end();
}

}  // namespace almacen
