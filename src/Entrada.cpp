#include "Entrada.h"

#include <M5Cardputer.h>

namespace entrada {

Pulsacion leer(bool durmiendo) {
    Pulsacion p;

    // isChange evita que una tecla mantenida apretada cuente mil veces.
    if (!M5Cardputer.Keyboard.isChange()) return p;
    if (!M5Cardputer.Keyboard.isPressed()) return p;

    const Keyboard_Class::KeysState estado = M5Cardputer.Keyboard.keysState();
    for (const char c : estado.word) {
        p.hubo = true;
        switch (c) {
            case 'a': p.accion = bichito::Accion::Alimentar; break;
            case 'l': p.accion = bichito::Accion::Limpiar;   break;
            case 'j': p.accion = bichito::Accion::Jugar;     break;
            case 'm': p.accion = bichito::Accion::Medicar;   break;
            case 'd':
                p.accion = durmiendo ? bichito::Accion::Despertar
                                     : bichito::Accion::Acostar;
                break;
            case 'r': p.reiniciar = true; break;
            default: break;
        }
        if (p.accion != bichito::Accion::Ninguna || p.reiniciar) break;
    }
    return p;
}

}  // namespace entrada
