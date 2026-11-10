#include "Animador.h"

#include <Arduino.h>

#include "Pantalla.h"

namespace {

// Margenes dentro de los que se mueve, en pixeles de pantalla.
constexpr float kIzquierda = 2.0f;
constexpr float kDerecha   = Pantalla::kAnchoPantalla - arte::kAncho * Pantalla::kEscala - 2.0f;

constexpr float kVelocidadNormal = 54.0f;  // pixeles por segundo, corriendo
constexpr float kVelocidadFloja  = 22.0f;  // caminando

constexpr uint32_t kAullidoMs = 1500;

// Cuanto se queda quieto antes de salir corriendo, segun como se sienta.
constexpr uint32_t kEsperaAnimado = 2200;
constexpr uint32_t kEsperaDecaido = 9000;

bool quiereMoverse(bichito::Humor h) {
    switch (h) {
        case bichito::Humor::Contento:
        case bichito::Humor::Aburrido:
            return true;
        default:
            return false;  // enfermo, con hambre, con sueno o sucio: se queda
    }
}

}  // namespace

void Animador::inicializar(int xInicial) {
    x_ = destino_ = static_cast<float>(xInicial);
    esperaMs_ = kEsperaAnimado;
}

void Animador::cambiarA(arte::Clip clip) {
    if (clip_ == clip) return;
    clip_ = clip;
    cuadroDelClip_ = 0;
    enCuadroMs_ = 0;
}

void Animador::correrUnRato(uint32_t ms) {
    corriendoMs_ = ms;
    comiendoMs_ = 0;
    nuevoDestino();
}

void Animador::ponerseAComer(uint32_t ms) {
    comiendoMs_ = ms;
    corriendoMs_ = 0;
    destino_ = x_;
}

void Animador::nuevoDestino() {
    // Un destino al azar, pero nunca tan cerca que el trote no se note.
    const float ancho = kDerecha - kIzquierda;
    float nuevo = kIzquierda + static_cast<float>(random(0, static_cast<long>(ancho)));
    if (fabsf(nuevo - x_) < 40.0f) {
        nuevo = (x_ - kIzquierda > ancho * 0.5f) ? kIzquierda : kDerecha;
    }
    destino_ = nuevo;
    haciaIzquierda_ = (destino_ < x_);
}

void Animador::elegirClip(const bichito::Bicho& bicho, uint32_t deltaMs) {
    // El paquete de arte no trae poses de comer ni de dormir. En vez de
    // inventar uno que no calce con el estilo, esos estados usan el clip
    // quieto y se distinguen por lo que dibuja Pantalla: el plato y los zZz.
    if (!bicho.vivo() || bicho.durmiendo()) {
        cambiarA(arte::kQuieto);
        destino_ = x_;
        aullandoMs_ = 0;
        return;
    }
    if (comiendoMs_ > 0) {
        comiendoMs_ = (comiendoMs_ > deltaMs) ? comiendoMs_ - deltaMs : 0;
        cambiarA(arte::kQuieto);
        destino_ = x_;
        return;
    }
    if (bicho.etapa() == bichito::Etapa::Huevo) {
        cambiarA(arte::kHuevo);
        destino_ = x_;
        return;
    }
    if (aullandoMs_ > 0) {
        aullandoMs_ = (aullandoMs_ > deltaMs) ? aullandoMs_ - deltaMs : 0;
        cambiarA(arte::kAulla);
        destino_ = x_;
        return;
    }

    if (corriendoMs_ > 0) {
        corriendoMs_ = (corriendoMs_ > deltaMs) ? corriendoMs_ - deltaMs : 0;
        if (fabsf(destino_ - x_) < 2.0f) nuevoDestino();
    }

    const bool llego = fabsf(destino_ - x_) < 2.0f;
    if (llego) {
        cambiarA(arte::kQuieto);
        if (esperaMs_ > deltaMs) {
            esperaMs_ -= deltaMs;
        } else if (quiereMoverse(bicho.humor())) {
            // De cada tres impulsos, uno es para aullar en vez de moverse.
            if (random(0, 3) == 0) {
                aullandoMs_ = kAullidoMs;
            } else {
                nuevoDestino();
            }
            esperaMs_ = (bicho.humor() == bichito::Humor::Contento)
                            ? kEsperaAnimado : kEsperaDecaido;
        } else {
            esperaMs_ = kEsperaDecaido;
        }
    } else {
        // Corre solo si anda contento; si no, camina.
        cambiarA(bicho.humor() == bichito::Humor::Contento ? arte::kCorre
                                                           : arte::kCamina);
    }
}

void Animador::actualizar(const bichito::Bicho& bicho, uint32_t deltaMs) {
    elegirClip(bicho, deltaMs);

    if (clip_ == arte::kCorre || clip_ == arte::kCamina) {
        const float v = (clip_ == arte::kCorre) ? kVelocidadNormal
                                                : kVelocidadFloja;
        const float paso = v * static_cast<float>(deltaMs) / 1000.0f;
        if (destino_ > x_) {
            x_ = (x_ + paso > destino_) ? destino_ : x_ + paso;
            haciaIzquierda_ = false;
        } else {
            x_ = (x_ - paso < destino_) ? destino_ : x_ - paso;
            haciaIzquierda_ = true;
        }
        if (x_ < kIzquierda) x_ = kIzquierda;
        if (x_ > kDerecha) x_ = kDerecha;
    }

    // Avance de cuadros dentro del clip.
    const arte::Tramo& tramo = arte::kClips[clip_];
    enCuadroMs_ += deltaMs;
    uint16_t dura = arte::kDuraciones[tramo.primero + cuadroDelClip_];
    while (enCuadroMs_ >= dura) {
        enCuadroMs_ -= dura;
        cuadroDelClip_ = (cuadroDelClip_ + 1) % tramo.cantidad;
        dura = arte::kDuraciones[tramo.primero + cuadroDelClip_];
    }
}

uint8_t Animador::cuadro() const {
    return arte::kClips[clip_].primero + cuadroDelClip_;
}
