// Decide que esta haciendo el bicho en pantalla: que clip toca, en que cuadro
// va, donde esta parado y hacia que lado mira.
//
// El nucleo del juego no sabe nada de esto. Aca solo vive la puesta en escena.
#pragma once

#include <cstdint>

#include "ArteBicho.h"
#include "Bichito.h"

class Animador {
public:
    void inicializar(int xInicial);

    // Llamar una vez por cuadro, con el tiempo transcurrido desde la anterior.
    void actualizar(const bichito::Bicho& bicho, uint32_t deltaMs);

    // Interrupciones puntuales que pide main cuando el usuario hace algo.
    void correrUnRato(uint32_t ms);
    void ponerseAComer(uint32_t ms);

    uint8_t cuadro() const;  // indice absoluto dentro de arte::kCuadros
    int     x() const { return static_cast<int>(x_); }

    // El lobo del paquete esta dibujado mirando a la izquierda, asi que hay que
    // espejarlo justo cuando camina hacia la derecha.
    bool espejado() const { return !haciaIzquierda_; }

    bool comiendo() const { return comiendoMs_ > 0; }

private:
    void elegirClip(const bichito::Bicho& bicho, uint32_t deltaMs);
    void cambiarA(arte::Clip clip);
    void nuevoDestino();

    arte::Clip clip_ = arte::kQuieto;
    uint8_t    cuadroDelClip_ = 0;
    uint32_t   enCuadroMs_ = 0;

    float    x_ = 0;
    float    destino_ = 0;
    bool     haciaIzquierda_ = false;
    uint32_t esperaMs_ = 0;   // cuanto falta para que se le ocurra moverse
    uint32_t comiendoMs_ = 0;
    uint32_t corriendoMs_ = 0;
    uint32_t aullandoMs_ = 0;
};
