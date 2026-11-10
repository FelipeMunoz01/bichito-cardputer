// Todo el dibujo en pantalla, sobre un lienzo de 240 x 135 que se vuelca de
// una sola vez para que no parpadee.
#pragma once

#include <M5Cardputer.h>

#include "Bichito.h"

class Animador;

class Pantalla {
public:
    static constexpr int kAnchoPantalla = 240;
    static constexpr int kAltoPantalla  = 135;
    static constexpr int kEscala        = 3;   // el bicho es de 32 x 20
    static constexpr int kSuelo         = 101; // donde apoya las patas

    void inicializar();

    // Aviso breve en la franja de abajo, pisa la ayuda de teclas por 1,5 s.
    void mensaje(const char* texto);

    void dibujar(const bichito::Bicho& bicho, const Animador& animador,
                 uint32_t ahoraMs);

private:
    void franjaSuperior(const bichito::Bicho& bicho);
    void franjaInferior(uint32_t ahoraMs);
    void habitacion(const bichito::Bicho& bicho, const Animador& animador);
    void bicho(const Animador& animador, bichito::Humor humor);
    void panelNecesidades(const bichito::Necesidades& n);
    void barra(int x, int y, float valor, const char* etiqueta);

    M5Canvas lienzo_{&M5Cardputer.Display};
    char     mensaje_[41] = {0};  // 40 caracteres es el ancho a 6 px por letra
    uint32_t mensajeHastaMs_ = 0;
};
