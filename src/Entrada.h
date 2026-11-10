// Lectura del teclado. En el ADV el teclado cuelga de un controlador I2C
// (TCA8418), no de la matriz GPIO del Cardputer original, pero la libreria
// M5Cardputer 1.1.0 o mayor detecta la placa sola y la API es la misma.
#pragma once

#include "Bichito.h"

namespace entrada {

struct Pulsacion {
    bichito::Accion accion = bichito::Accion::Ninguna;
    bool reiniciar = false;
    bool hubo = false;  // se apreto algo, aunque no sirviera de nada
};

// Llamar despues de M5Cardputer.update(). durmiendo decide si la tecla D
// acuesta o despierta.
Pulsacion leer(bool durmiendo);

}  // namespace entrada
