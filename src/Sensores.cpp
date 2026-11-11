#include "Sensores.h"

#include <M5Cardputer.h>

#include <cmath>

namespace {

// Cada cuánto se toma una muestra de audio. Escuchar todo el rato gastaría
// batería sin ganar nada: una palmada dura bastante más que esto.
constexpr uint32_t kIntervaloEscuchaMs = 350;

// Muestras por medición: a 16 kHz son unos 16 ms de audio.
constexpr size_t   kMuestrasAudio = 256;
constexpr uint32_t kFrecuenciaAudio = 16000;

// Umbral relativo en vez de un numero fijo. Medido en el equipo, el fondo de
// una pieza tranquila da un RMS de 7 a 16 y una palmada pasa de 150. Pero ese
// "7 a 16" es de esa pieza: en la calle o con musica el fondo sube y un umbral
// fijo se dispararia solo. Asi que se sigue el nivel de fondo y se dispara
// cuando algo lo supera por mucho.
constexpr float kFactorRuido = 5.0f;   // cuantas veces el fondo
constexpr float kMargenRuido = 80.0f;  // y ademas este salto minimo

// El fondo sube despacio y baja rapido: un ruido corto casi no lo mueve, pero
// si el ambiente se pone ruidoso de verdad, el detector se acostumbra.
constexpr float kSubeFondo = 0.05f;
constexpr float kBajaFondo = 0.20f;

int16_t audio[kMuestrasAudio];

uint16_t rms(const int16_t* muestras, size_t n) {
    uint64_t suma = 0;
    for (size_t i = 0; i < n; ++i) {
        const int32_t v = muestras[i];
        suma += static_cast<uint64_t>(v * v);
    }
    return static_cast<uint16_t>(std::sqrt(static_cast<double>(suma) / n));
}

}  // namespace

void Sensores::inicializar() {
    // Ojo: M5Cardputer solo toma referencias de Display, Power, Speaker y Mic.
    // El IMU se llega por el M5 global, que M5Cardputer.begin() ya dejo listo.
    imu_ = M5.Imu.isEnabled();
    Serial.printf("[sensores] IMU %s\n", imu_ ? "lista" : "no disponible");

#if BICHITO_CON_MICROFONO
    // El altavoz arranca encendido: hay que soltarlo antes de pedir el micrófono.
    if (M5Cardputer.Speaker.isEnabled()) M5Cardputer.Speaker.end();
    microfono_ = M5Cardputer.Mic.begin();
    Serial.printf("[sensores] microfono %s\n", microfono_ ? "listo" : "no disponible");
    if (!microfono_) {
        M5Cardputer.Speaker.begin();  // si no sirve, al menos que suene
    }
#endif
}

void Sensores::escuchar(uint32_t ahoraMs) {
#if BICHITO_CON_MICROFONO
    if (!microfono_ || audioSoltado_) return;
    if (ahoraMs < proximaEscuchaMs_) return;
    if (M5Cardputer.Mic.isRecording()) return;

    // La grabación anterior ya terminó: se mide y se pide la siguiente.
    nivel_ = rms(audio, kMuestrasAudio);
    if (nivel_ > pico_) pico_ = nivel_;

    if (fondo_ < 0.0f) fondo_ = nivel_;  // primera medicion
    const float umbral = fondo_ * kFactorRuido + kMargenRuido;
    if (nivel_ > umbral) ruidoPendiente_ = true;
    fondo_ += (nivel_ > fondo_ ? kSubeFondo : kBajaFondo) * (nivel_ - fondo_);

    proximaEscuchaMs_ = ahoraMs + kIntervaloEscuchaMs;
    M5Cardputer.Mic.record(audio, kMuestrasAudio, kFrecuenciaAudio);
#else
    (void)ahoraMs;
#endif
}

void Sensores::actualizar(uint32_t ahoraMs) {
    float ax, ay, az;
    if (imu_ && M5.Imu.update() && M5.Imu.getAccel(&ax, &ay, &az)) {
        const float magnitud = std::sqrt(ax * ax + ay * ay + az * az);
        pasosPendientes_ += pasos_.muestra(magnitud, ahoraMs);
        if (sacudida_.muestra(magnitud, ahoraMs)) cariciaPendiente_ = true;
    }
    escuchar(ahoraMs);
}

bichito::Estimulos Sensores::tomar() {
    bichito::Estimulos e;
    e.pasos = pasosPendientes_;
    e.caricia = cariciaPendiente_;
    e.ruido = ruidoPendiente_;

    pasosPendientes_ = 0;
    cariciaPendiente_ = false;
    ruidoPendiente_ = false;
    return e;
}

void Sensores::soltarAudio() {
#if BICHITO_CON_MICROFONO
    if (!microfono_ || audioSoltado_) return;
    M5Cardputer.Mic.end();
    M5Cardputer.Speaker.begin();
    audioSoltado_ = true;
#endif
}

void Sensores::retomarAudio() {
#if BICHITO_CON_MICROFONO
    if (!microfono_ || !audioSoltado_) return;
    M5Cardputer.Speaker.end();
    M5Cardputer.Mic.begin();
    audioSoltado_ = false;
    proximaEscuchaMs_ = 0;
#endif
}
