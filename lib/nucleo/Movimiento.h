// Detección de pasos y de sacudidas a partir del acelerómetro.
//
// Igual que Bichito.h, acá no entra nada de Arduino: estas dos clases reciben
// la magnitud del vector de aceleración en g y una marca de tiempo en ms, y se
// prueban en el Mac con señales sintéticas. El BMI270 vive en src/Sensores.cpp.
#pragma once

#include <cstdint>

namespace movimiento {

// Cuenta pasos buscando picos rítmicos en la magnitud de la aceleración.
//
// Un pico suelto no cuenta: hay que encadenar varios con un ritmo creíble para
// que empiecen a sumar. Es lo que separa caminar de dejar el equipo encima de
// la mesa y darle un golpe.
class ContadorPasos {
public:
    struct Ajustes {
        float    alfaRapido   = 0.35f;  // suaviza el ruido de la muestra
        float    alfaLento    = 0.02f;  // sigue la gravedad y la inclinación
        float    umbral       = 0.11f;  // cuánto hay que pasarse de la base, en g
        uint32_t refractarioMs = 260;   // nadie da dos pasos más rápido que esto
        uint32_t maxIntervaloMs = 1600; // más lento que esto ya no es caminar
        uint8_t  rachaMinima  = 3;      // picos seguidos antes de empezar a sumar
    };

    // Dos constructores en vez de un argumento por defecto: Ajustes{} no se
    // puede evaluar dentro de la propia clase, que todavia no esta completa.
    ContadorPasos() = default;
    explicit ContadorPasos(const Ajustes& ajustes) : a_(ajustes) {}

    // Devuelve cuántos pasos sumó esta muestra: 0, 1, o la racha completa la
    // primera vez que se confirma que de verdad va caminando.
    uint8_t muestra(float magnitud, uint32_t ms);

    uint32_t total() const { return total_; }
    void     reiniciar();

private:
    Ajustes  a_;
    float    rapido_ = 0;
    float    lento_ = 0;
    bool     iniciado_ = false;
    bool     arriba_ = false;
    bool     hubooPaso_ = false;
    uint32_t ultimoPicoMs_ = 0;
    uint8_t  racha_ = 0;
    uint32_t total_ = 0;
};

// Detecta el agitón: el equipo moviéndose fuerte y cambiando de sentido varias
// veces seguidas. Una sola sacudida fuerte no basta, tiene que ser un vaivén.
class DetectorSacudida {
public:
    struct Ajustes {
        float    umbral         = 0.55f;  // desvío respecto de 1 g
        uint8_t  crucesMinimos  = 4;      // cambios de sentido en la ventana
        uint32_t ventanaMs      = 700;
        uint32_t enfriamientoMs = 1200;   // para no contar el mismo agitón diez veces
    };

    DetectorSacudida() = default;
    explicit DetectorSacudida(const Ajustes& ajustes) : a_(ajustes) {}

    bool muestra(float magnitud, uint32_t ms);

private:
    Ajustes  a_;
    int8_t   signoPrevio_ = 0;
    uint8_t  cruces_ = 0;
    uint32_t inicioVentanaMs_ = 0;
    uint32_t ultimaMs_ = 0;
    bool     hubooSacudida_ = false;
};

}  // namespace movimiento
