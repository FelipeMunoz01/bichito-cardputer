// El Cardputer ADV no trae RTC, asi que la hora real solo se consigue por NTP.
//
// Sin red el juego funciona igual, pero el bicho deja de envejecer mientras el
// equipo esta apagado: solo cuenta el tiempo encendido. Con red, al prender se
// calcula el hueco y se le cobra de una vez.
#pragma once

#include <cstdint>

namespace reloj {

// Intenta conectarse y sincronizar. No bloquea mas de unos segundos.
void inicializar();

bool horaConocida();

// Segundos desde 1970, o 0 si nunca se supo la hora.
int64_t epoch();

}  // namespace reloj
