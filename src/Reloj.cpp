#include "Reloj.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Copia secretos.ejemplo.h como secretos.h y pon tu red. El archivo real esta
// en .gitignore. Si no existe, esto compila igual y se salta la sincronizacion.
#if __has_include("secretos.h")
#include "secretos.h"
#define BICHITO_TIENE_SECRETOS 1
#endif

namespace reloj {
namespace {

bool sincronizado = false;

constexpr uint32_t kEsperaWifiMs = 8000;
constexpr uint32_t kEsperaNtpMs  = 6000;
constexpr int64_t  kEpochMinimo  = 1700000000;  // nov 2023, para descartar basura

}  // namespace

void inicializar() {
#ifdef BICHITO_TIENE_SECRETOS
    WiFi.mode(WIFI_STA);
    WiFi.begin(kWifiRed, kWifiClave);

    const uint32_t limite = millis() + kEsperaWifiMs;
    while (WiFi.status() != WL_CONNECTED && millis() < limite) {
        delay(100);
    }
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_OFF);
        return;
    }

    // Se guarda epoch en UTC: la zona horaria solo importaria para mostrar la
    // hora en pantalla, y aqui no se muestra.
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");

    const uint32_t limiteNtp = millis() + kEsperaNtpMs;
    while (time(nullptr) < kEpochMinimo && millis() < limiteNtp) {
        delay(100);
    }
    sincronizado = time(nullptr) >= kEpochMinimo;

    // La radio se apaga: con 1750 mAh conviene, y la hora ya quedo en el reloj
    // interno del chip hasta el proximo apagon.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
#endif
}

bool horaConocida() { return sincronizado; }

int64_t epoch() {
    if (!sincronizado) return 0;
    return static_cast<int64_t>(time(nullptr));
}

}  // namespace reloj
