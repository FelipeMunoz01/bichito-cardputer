// Bichito: mascota virtual para M5Stack Cardputer ADV.
//
// Teclas:  A alimentar   L limpiar   J jugar   D dormir o despertar
//          M remedio     R volver al huevo
#include <Arduino.h>
#include <M5Cardputer.h>

#include "Almacen.h"
#include "Animador.h"
#include "Bichito.h"
#include "Entrada.h"
#include "Pantalla.h"
#include "Reloj.h"

namespace {

bichito::Bicho bicho;
Pantalla       pantalla;
Animador       animador;

uint32_t ultimoTickMs     = 0;
uint32_t ultimoGuardadoMs = 0;
uint32_t ultimoDibujoMs   = 0;

constexpr uint32_t kTickMs      = 1000;   // el nucleo avanza de segundo en segundo
constexpr uint32_t kGuardadoMs  = 30000;  // y se guarda cada medio minuto
constexpr uint32_t kDibujoMs    = 50;     // 20 cuadros por segundo

void sonar(bool bien) {
    // El ADV usa el codec ES8311. M5Unified 0.2.8 o mayor ya lo configura solo.
    M5Cardputer.Speaker.tone(bien ? 1200 : 300, bien ? 70 : 120);
}

// Aqui entra el sprint 2. El BMI270 vive en M5Cardputer.Imu y el microfono en
// M5Cardputer.Mic: pasos para subir el animo, agiton para caricia, palmada
// para ruido. Mientras devuelva vacio, el juego corre solo con el reloj.
bichito::Estimulos estimulosDelMundo() {
    return bichito::Estimulos{};
}

const char* textoDe(bichito::Accion a) {
    switch (a) {
        case bichito::Accion::Alimentar: return "comio y quedo contento";
        case bichito::Accion::Limpiar:   return "quedo limpio";
        case bichito::Accion::Jugar:     return "jugaron un rato";
        case bichito::Accion::Acostar:   return "se fue a dormir";
        case bichito::Accion::Despertar: return "se desperto";
        case bichito::Accion::Medicar:   return "le dio el remedio";
        default:                         return "";
    }
}

void guardarAhora() {
    almacen::guardar(bicho.exportar(reloj::epoch()));
    ultimoGuardadoMs = millis();
}

// Le cobra al bicho el tiempo que el equipo estuvo apagado. Solo se puede si
// hay hora real: sin NTP no hay forma de saber cuanto paso.
void cobrarAusencia(int64_t ultimoEpoch) {
    if (ultimoEpoch <= 0 || !reloj::horaConocida()) return;

    const int64_t ahora = reloj::epoch();
    if (ahora <= ultimoEpoch) return;

    const int64_t  hueco = ahora - ultimoEpoch;
    const uint32_t segundos =
        (hueco > static_cast<int64_t>(bichito::tiempos::kAusenciaMaxima))
            ? bichito::tiempos::kAusenciaMaxima
            : static_cast<uint32_t>(hueco);

    bicho.avanzar(segundos);

    char aviso[41];
    snprintf(aviso, sizeof(aviso), "estuvo solo %luh%02lum",
             static_cast<unsigned long>(segundos / 3600),
             static_cast<unsigned long>((segundos % 3600) / 60));
    pantalla.mensaje(aviso);
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, /*enableKeyboard=*/true);

    Serial.begin(115200);
    pantalla.inicializar();

    // Primero la hora, porque de ella depende saber cuanto estuvo apagado.
    reloj::inicializar();

    bichito::Guardado g;
    if (almacen::cargar(g) && bicho.importar(g)) {
        Serial.printf("[bichito] estado recuperado, edad %lus\n",
                      static_cast<unsigned long>(bicho.edadSegundos()));
        cobrarAusencia(g.ultimoEpoch);
    } else {
        Serial.println("[bichito] empieza un huevo nuevo");
        pantalla.mensaje("un huevo nuevo");
    }

    if (!reloj::horaConocida()) {
        Serial.println("[bichito] sin hora real: solo cuenta el tiempo encendido");
    }

    // La semilla sale del ruido del ADC, asi el bicho no corre siempre igual.
    randomSeed(esp_random());
    animador.inicializar(70);

    guardarAhora();
    ultimoTickMs = ultimoDibujoMs = millis();
    animador.actualizar(bicho, 0);
    pantalla.dibujar(bicho, animador, millis());
}

void loop() {
    M5Cardputer.update();
    const uint32_t ahora = millis();

    const entrada::Pulsacion p = entrada::leer(bicho.durmiendo());
    if (p.reiniciar) {
        bicho.reiniciar();
        almacen::borrar();
        guardarAhora();
        pantalla.mensaje("de vuelta al huevo");
        sonar(true);
    } else if (p.accion != bichito::Accion::Ninguna) {
        if (bicho.aplicar(p.accion)) {
            pantalla.mensaje(textoDe(p.accion));
            if (p.accion == bichito::Accion::Alimentar) {
                animador.ponerseAComer(1600);
            } else if (p.accion == bichito::Accion::Jugar) {
                animador.correrUnRato(3000);
            }
            sonar(true);
            guardarAhora();
        } else {
            pantalla.mensaje("ahora no corresponde");
            sonar(false);
        }
    }

    if (ahora - ultimoTickMs >= kTickMs) {
        const uint32_t segundos = (ahora - ultimoTickMs) / kTickMs;
        ultimoTickMs += segundos * kTickMs;
        bicho.avanzar(segundos, estimulosDelMundo());
    }

    if (ahora - ultimoGuardadoMs >= kGuardadoMs) {
        guardarAhora();
    }

    if (ahora - ultimoDibujoMs >= kDibujoMs) {
        const uint32_t delta = ahora - ultimoDibujoMs;
        ultimoDibujoMs = ahora;
        animador.actualizar(bicho, delta);
        pantalla.dibujar(bicho, animador, ahora);
    }

    delay(5);
}
