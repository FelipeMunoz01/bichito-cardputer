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
#include "Sensores.h"

namespace {

bichito::Bicho bicho;
Pantalla       pantalla;
Animador       animador;
Sensores       sensores;

// El altavoz y el microfono no pueden estar encendidos a la vez. Cuando suena
// un tono se suelta el microfono y se vuelve a tomar pasado este momento.
uint32_t audioDeVueltaMs = 0;

uint32_t ultimoTickMs     = 0;
uint32_t ultimoGuardadoMs = 0;
uint32_t ultimoDibujoMs   = 0;
uint32_t ultimoInformeMs  = 0;

constexpr uint32_t kTickMs      = 1000;   // el nucleo avanza de segundo en segundo
constexpr uint32_t kGuardadoMs  = 30000;  // y se guarda cada medio minuto
constexpr uint32_t kDibujoMs    = 50;     // 20 cuadros por segundo
constexpr uint32_t kInformeMs   = 3000;   // nivel de ruido al monitor serie

void sonar(bool bien) {
    // El ADV usa el codec ES8311. M5Unified 0.2.8 o mayor ya lo configura solo.
    sensores.soltarAudio();
    M5Cardputer.Speaker.tone(bien ? 1200 : 300, bien ? 70 : 120);
    audioDeVueltaMs = millis() + 300;
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
    sensores.inicializar();

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
    pantalla.dibujar(bicho, animador, sensores.pasosTotales(), millis());
}

void loop() {
    M5Cardputer.update();
    const uint32_t ahora = millis();

    sensores.actualizar(ahora);
    if (audioDeVueltaMs != 0 && ahora >= audioDeVueltaMs) {
        sensores.retomarAudio();
        audioDeVueltaMs = 0;
    }

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

        const bichito::Estimulos mundo = sensores.tomar();
        bicho.avanzar(segundos, mundo);

        if (mundo.caricia && bicho.vivo() && !bicho.durmiendo()) {
            pantalla.mensaje("le gusto que lo movieras");
            animador.correrUnRato(2500);
        } else if (mundo.ruido && bicho.vivo()) {
            pantalla.mensaje("se asusto con el ruido");
        }
    }

    if (ahora - ultimoGuardadoMs >= kGuardadoMs) {
        guardarAhora();
    }

    // El umbral de palmada esta puesto a ojo porque no se pudo medir en el
    // equipo. Esta linea imprime el nivel real: dejar el monitor abierto,
    // aplaudir, y ajustar kUmbralRuido en Sensores.cpp con lo que se vea.
    if (ahora - ultimoInformeMs >= kInformeMs) {
        ultimoInformeMs = ahora;
        Serial.printf("[sensores] pasos %lu  ruido ahora %u  pico %u\n",
                      static_cast<unsigned long>(sensores.pasosTotales()),
                      sensores.ultimoNivel(), sensores.picoNivel());
        sensores.olvidarPico();
    }

    if (ahora - ultimoDibujoMs >= kDibujoMs) {
        const uint32_t delta = ahora - ultimoDibujoMs;
        ultimoDibujoMs = ahora;
        animador.actualizar(bicho, delta);
        pantalla.dibujar(bicho, animador, sensores.pasosTotales(), ahora);
    }

    delay(5);
}
