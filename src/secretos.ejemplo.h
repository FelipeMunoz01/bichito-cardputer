// Copia este archivo como src/secretos.h y pon los datos de tu red.
// secretos.h esta en .gitignore, no se sube al repo.
//
// Sirve solo para sincronizar la hora por NTP al prender, que es lo que
// permite que el bicho envejezca mientras el equipo esta apagado.
#pragma once

constexpr const char* kWifiRed   = "nombre de tu red";
constexpr const char* kWifiClave = "clave de tu red";
