// Guardado del estado en NVS, la particion de preferencias de la flash.
// Sobrevive reinicios y cortes de bateria.
#pragma once

#include "Bichito.h"

namespace almacen {

// false si no habia nada guardado o si la version del formato cambio.
bool cargar(bichito::Guardado& destino);

void guardar(const bichito::Guardado& g);

void borrar();

}  // namespace almacen
