// El mundo real entrando al juego: BMI270 para los pasos y el agitón, y el
// micrófono para las palmadas.
//
// La lógica de detección no vive acá sino en lib/nucleo/Movimiento.h, que se
// prueba en el Mac. Esto solo lee el hardware y se lo pasa.
//
// El altavoz y el micrófono comparten el bus de audio, así que no pueden estar
// los dos encendidos: antes de hacer sonar un tono hay que llamar a
// soltarAudio(), y retomarAudio() cuando el tono termine.
#pragma once

#include <cstdint>

#include "Bichito.h"
#include "Movimiento.h"

// Poner en 0 si el micrófono da problemas: el resto sigue funcionando igual.
#ifndef BICHITO_CON_MICROFONO
#define BICHITO_CON_MICROFONO 1
#endif

class Sensores {
public:
    void inicializar();

    // Llamar en cada vuelta del bucle principal.
    void actualizar(uint32_t ahoraMs);

    // Devuelve lo acumulado desde la última vez y lo deja en cero.
    bichito::Estimulos tomar();

    void soltarAudio();
    void retomarAudio();

    bool     hayImu() const { return imu_; }
    bool     hayMicrofono() const { return microfono_; }
    uint32_t pasosTotales() const { return pasos_.total(); }

    // Para calibrar el umbral por el monitor serie. El pico es el que importa:
    // una palmada dura menos que el intervalo entre informes, así que el último
    // valor suelto casi nunca la pilla.
    uint16_t ultimoNivel() const { return nivel_; }
    uint16_t picoNivel() const { return pico_; }
    uint16_t fondoNivel() const { return fondo_ < 0 ? 0 : static_cast<uint16_t>(fondo_); }
    void     olvidarPico() { pico_ = 0; }

private:
    void escuchar(uint32_t ahoraMs);

    movimiento::ContadorPasos    pasos_;
    movimiento::DetectorSacudida sacudida_;

    bool     imu_ = false;
    bool     microfono_ = false;
    bool     audioSoltado_ = false;

    uint32_t pasosPendientes_ = 0;
    bool     cariciaPendiente_ = false;
    bool     ruidoPendiente_ = false;

    uint32_t proximaEscuchaMs_ = 0;
    uint16_t nivel_ = 0;
    uint16_t pico_ = 0;
    float    fondo_ = -1.0f;  // negativo = todavia no se midio nada
};
