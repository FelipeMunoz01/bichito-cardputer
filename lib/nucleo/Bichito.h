// Nucleo del juego: necesidades, humor, etapas y envejecimiento.
//
// Este archivo no incluye nada de Arduino a proposito. Toda la logica se puede
// compilar y probar en el Mac con "pio test -e pruebas", sin el equipo enchufado.
#pragma once

#include <cstdint>

namespace bichito {

// Las cuatro necesidades van de 0 a 100, donde 100 es "perfecto".
struct Necesidades {
    float saciedad = 80.0f;  // lo contrario del hambre
    float animo    = 80.0f;
    float energia  = 80.0f;
    float limpieza = 80.0f;
};

// Cuanto se pierde por minuto. Subir un valor hace el juego mas exigente.
struct Ritmo {
    float saciedad          = 0.18f;  // de 100 a 0 en algo mas de 9 horas
    float animo             = 0.12f;
    float limpieza          = 0.08f;
    float energia           = 0.10f;  // gasto estando despierto
    float recuperaDurmiendo = 0.60f;  // recuperacion por minuto de sueno
    float animoPorPaso      = 0.01f;  // cada paso del IMU sube un poco el animo
};

// Lo que el mundo real le pasa al bicho entre dos actualizaciones.
// El sprint 1 lo deja todo en cero; el sprint 2 lo llena con IMU y microfono.
struct Estimulos {
    uint32_t pasos   = 0;
    bool     caricia = false;  // agiton suave
    bool     ruido   = false;  // palmada o grito cerca del microfono
};

enum class Etapa : uint8_t { Huevo, Cria, Joven, Adulto };

enum class Humor : uint8_t {
    Contento,
    Aburrido,
    Hambriento,
    Sucio,
    Cansado,
    Durmiendo,
    Enfermo,
    Muerto,
};

enum class Accion : uint8_t {
    Ninguna,
    Alimentar,
    Limpiar,
    Jugar,
    Acostar,
    Despertar,
    Medicar,
};

// Umbrales de tiempo, todos en segundos. Son constantes sueltas para que
// ajustar el ritmo del juego sea cambiar un numero y recompilar.
namespace tiempos {
constexpr uint32_t kEclosion       = 3 * 60;        // el huevo se abre a los 3 min
constexpr uint32_t kFinCria        = 6 * 3600;      // cria hasta las 6 horas
constexpr uint32_t kFinJoven       = 24 * 3600;     // joven hasta el primer dia
constexpr uint32_t kDescuidoEnferma = 3 * 3600;     // 3 h sin comer o sucio enferma
constexpr uint32_t kEnfermoMuere   = 48 * 3600;     // 48 h enfermo sin remedio
constexpr uint32_t kAusenciaMaxima = 7 * 24 * 3600; // tope al tiempo apagado
}  // namespace tiempos

// Version del formato guardado. Subirla invalida los guardados viejos en vez
// de leer basura, que es lo que uno quiere mientras el formato esta cambiando.
constexpr uint16_t kVersionGuardado = 1;

// Lo que se escribe en NVS. POD puro, sin punteros, para volcarlo como blob.
struct Guardado {
    uint16_t version = kVersionGuardado;
    float    saciedad = 0;
    float    animo = 0;
    float    energia = 0;
    float    limpieza = 0;
    uint32_t edadSegundos = 0;
    uint32_t descuidoSegundos = 0;
    uint32_t enfermoSegundos = 0;
    bool     durmiendo = false;
    bool     enfermo = false;
    bool     vivo = true;
    int64_t  ultimoEpoch = 0;  // 0 cuando nunca se supo la hora real
};

class Bicho {
public:
    // Hace correr el reloj. Llamar con el tiempo transcurrido de verdad, venga
    // del bucle principal o del hueco que dejo el equipo apagado.
    void avanzar(uint32_t segundos, const Estimulos& estimulos = Estimulos{});

    // Devuelve false si la accion no corresponde ahora: darle de comer dormido,
    // jugar sin energia, curar a un bicho sano. El llamador usa eso para avisar.
    bool aplicar(Accion accion);

    Humor humor() const;
    Etapa etapa() const;

    const Necesidades& necesidades() const { return n_; }
    Ritmo&             ritmo() { return ritmo_; }

    uint32_t edadSegundos() const { return edadSegundos_; }
    bool     durmiendo() const { return durmiendo_; }
    bool     enfermo() const { return enfermo_; }
    bool     vivo() const { return vivo_; }

    // Vuelve al huevo. Lo pide la pantalla de "se murio" y tambien el reinicio
    // manual con la tecla R.
    void reiniciar();

    Guardado exportar(int64_t epoch) const;
    bool     importar(const Guardado& g);  // false si la version no cuadra

private:
    void limitar();
    void paso(uint32_t segundos, const Estimulos& estimulos);

    Necesidades n_;
    Ritmo       ritmo_;
    uint32_t    edadSegundos_ = 0;
    uint32_t    descuidoSegundos_ = 0;
    uint32_t    enfermoSegundos_ = 0;
    bool        durmiendo_ = false;
    bool        enfermo_ = false;
    bool        vivo_ = true;
};

// Nombres en castellano para mostrar en pantalla y en el monitor serie.
const char* nombre(Humor h);
const char* nombre(Etapa e);

}  // namespace bichito
