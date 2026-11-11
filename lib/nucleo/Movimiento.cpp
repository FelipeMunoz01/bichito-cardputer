#include "Movimiento.h"

#include <cmath>

namespace movimiento {

void ContadorPasos::reiniciar() {
    iniciado_ = false;
    arriba_ = false;
    hubooPaso_ = false;
    racha_ = 0;
    total_ = 0;
}

uint8_t ContadorPasos::muestra(float magnitud, uint32_t ms) {
    if (!iniciado_) {
        rapido_ = lento_ = magnitud;
        iniciado_ = true;
        return 0;
    }

    rapido_ += a_.alfaRapido * (magnitud - rapido_);
    lento_  += a_.alfaLento * (magnitud - lento_);

    // Histéresis: para volver a armar el detector hay que bajar bastante más
    // de lo que costó subir. Si no, el ruido alrededor del umbral cuenta diez
    // pasos por cada uno real.
    const float arriba = lento_ + a_.umbral;
    const float abajo  = lento_ - a_.umbral * 0.5f;

    if (arriba_) {
        if (rapido_ < abajo) arriba_ = false;
        return 0;
    }
    if (rapido_ <= arriba) return 0;

    arriba_ = true;
    if (hubooPaso_ && ms - ultimoPicoMs_ < a_.refractarioMs) return 0;

    const bool sigueElRitmo = hubooPaso_ && (ms - ultimoPicoMs_ <= a_.maxIntervaloMs);
    racha_ = sigueElRitmo ? static_cast<uint8_t>(racha_ + 1) : 1;
    ultimoPicoMs_ = ms;
    hubooPaso_ = true;

    if (racha_ < a_.rachaMinima) return 0;
    if (racha_ == a_.rachaMinima) {
        // Se confirmó que camina: se cobran también los picos que venían
        // guardados sin contar.
        total_ += a_.rachaMinima;
        return a_.rachaMinima;
    }
    ++total_;
    return 1;
}

bool DetectorSacudida::muestra(float magnitud, uint32_t ms) {
    const float desvio = magnitud - 1.0f;
    if (std::fabs(desvio) < a_.umbral) return false;

    if (cruces_ == 0 || ms - inicioVentanaMs_ > a_.ventanaMs) {
        inicioVentanaMs_ = ms;
        cruces_ = 0;
        signoPrevio_ = 0;
    }

    const int8_t signo = (desvio > 0) ? 1 : -1;
    if (signo != signoPrevio_) {
        signoPrevio_ = signo;
        ++cruces_;
    }

    if (cruces_ < a_.crucesMinimos) return false;
    if (hubooSacudida_ && ms - ultimaMs_ < a_.enfriamientoMs) return false;

    ultimaMs_ = ms;
    hubooSacudida_ = true;
    cruces_ = 0;
    signoPrevio_ = 0;
    return true;
}

}  // namespace movimiento
