#include "Pantalla.h"

#include <cstdio>
#include <cstring>

#include "Animador.h"
#include "ArteBicho.h"

using bichito::Bicho;
using bichito::Etapa;
using bichito::Humor;
using bichito::Necesidades;

namespace {

struct Color {
    uint8_t r, g, b;
};

const Color kFondo  = {16, 20, 24};
const Color kTenue  = {110, 120, 130};
const Color kClaro  = {225, 230, 235};
const Color kSombra = {30, 36, 42};
const Color kPiso   = {44, 52, 60};

uint16_t c565(const Color& c) { return M5Cardputer.Display.color565(c.r, c.g, c.b); }

// Cada humor tine el pelaje hacia un color. Es la via mas corta para que se
// note de una mirada sin dibujar un juego de cuadros por estado.
struct Tinte {
    Color color;
    float fuerza;  // 0 = el pelaje original, 1 = el color del tinte
};

Tinte tintePara(Humor h) {
    switch (h) {
        case Humor::Contento:   return {{255, 255, 255}, 0.00f};
        case Humor::Aburrido:   return {{120, 140, 150}, 0.18f};
        case Humor::Hambriento: return {{222, 186, 96},  0.26f};
        case Humor::Sucio:      return {{120, 94, 56},   0.34f};
        case Humor::Cansado:    return {{96, 118, 180},  0.22f};
        case Humor::Durmiendo:  return {{72, 96, 160},   0.26f};
        case Humor::Enfermo:    return {{150, 180, 90},  0.34f};
        case Humor::Muerto:     return {{90, 90, 96},    0.62f};
    }
    return {{255, 255, 255}, 0.0f};
}

uint8_t mezclar(uint8_t origen, uint8_t destino, float fuerza) {
    const float v = origen + (static_cast<float>(destino) - origen) * fuerza;
    return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

void textoCorto(M5Canvas& l, int x, int y, const Color& c, const char* t) {
    l.setTextColor(c565(c));
    l.setCursor(x, y);
    l.print(t);
}

}  // namespace

void Pantalla::inicializar() {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(90);
    lienzo_.setColorDepth(16);
    lienzo_.createSprite(kAnchoPantalla, kAltoPantalla);
    lienzo_.setFont(&fonts::Font0);
    lienzo_.setTextSize(1);
}

void Pantalla::mensaje(const char* texto) {
    snprintf(mensaje_, sizeof(mensaje_), "%s", texto);
    mensajeHastaMs_ = millis() + 1500;
}

void Pantalla::bicho(const Animador& animador, Humor humor) {
    // La paleta tenida se arma una vez por cuadro: son 16 colores contra los
    // 640 pixeles que vienen despues.
    const Tinte tinte = tintePara(humor);
    uint16_t paleta[16];
    for (int i = 0; i < 16; ++i) {
        const uint32_t c = arte::kPaleta[i];
        paleta[i] = M5Cardputer.Display.color565(
            mezclar((c >> 16) & 0xFF, tinte.color.r, tinte.fuerza),
            mezclar((c >> 8) & 0xFF, tinte.color.g, tinte.fuerza),
            mezclar(c & 0xFF, tinte.color.b, tinte.fuerza));
    }

    const uint8_t* datos = arte::kCuadros[animador.cuadro()];
    const int  x0 = animador.x();
    const int  y0 = kSuelo - arte::kAlto * kEscala;
    const bool volteado = animador.mirandoIzquierda();

    for (int f = 0; f < arte::kAlto; ++f) {
        for (int c = 0; c < arte::kAncho; ++c) {
            const int     n = f * arte::kAncho + c;
            const uint8_t par = datos[n / 2];
            const uint8_t indice = (n & 1) ? (par & 0x0F) : (par >> 4);
            if (indice == 0) continue;  // transparente
            const int cx = volteado ? (arte::kAncho - 1 - c) : c;
            lienzo_.fillRect(x0 + cx * kEscala, y0 + f * kEscala, kEscala, kEscala,
                             paleta[indice]);
        }
    }
}

void Pantalla::habitacion(const Bicho& b, const Animador& animador) {
    lienzo_.drawFastHLine(0, kSuelo + 1, kAnchoPantalla, c565(kPiso));

    // Sombra bajo las patas, para que no quede flotando.
    const int centro = animador.x() + arte::kAncho * kEscala / 2;
    lienzo_.fillEllipse(centro, kSuelo + 1, 30, 3, c565(kSombra));

    bicho(animador, b.humor());

    if (b.durmiendo() && b.vivo()) {
        textoCorto(lienzo_, centro + 34, 40, kTenue, "z");
        textoCorto(lienzo_, centro + 41, 30, kTenue, "Z");
    }
}

void Pantalla::franjaSuperior(const Bicho& b) {
    lienzo_.fillRect(0, 0, kAnchoPantalla, 12, c565(kSombra));
    textoCorto(lienzo_, 3, 2, kClaro, "BICHITO");

    char medio[40];
    const uint32_t horas = b.edadSegundos() / 3600;
    const uint32_t mins  = (b.edadSegundos() % 3600) / 60;
    snprintf(medio, sizeof(medio), "%s  %luh%02lu", bichito::nombre(b.etapa()),
             static_cast<unsigned long>(horas), static_cast<unsigned long>(mins));
    textoCorto(lienzo_, 56, 2, kTenue, medio);

    if (b.etapa() != Etapa::Huevo) {
        char estado[20];
        snprintf(estado, sizeof(estado), "%s", bichito::nombre(b.humor()));
        textoCorto(lienzo_, 148, 2, kClaro, estado);
    }

    char bat[8];
    snprintf(bat, sizeof(bat), "%d%%", M5Cardputer.Power.getBatteryLevel());
    textoCorto(lienzo_, kAnchoPantalla - 4 - static_cast<int>(strlen(bat)) * 6, 2,
               kTenue, bat);
}

void Pantalla::barra(int x, int y, float valor, const char* etiqueta) {
    constexpr int kAnchoBarra = 36;
    constexpr int kAltoBarra  = 7;

    textoCorto(lienzo_, x, y + 1, kTenue, etiqueta);

    const int x0 = x + 19;
    lienzo_.drawRect(x0, y, kAnchoBarra, kAltoBarra, c565(kTenue));

    const int lleno = static_cast<int>((kAnchoBarra - 2) * valor / 100.0f);
    Color color = {92, 202, 120};
    if (valor < 30.0f) {
        color = {214, 92, 92};
    } else if (valor < 60.0f) {
        color = {222, 182, 92};
    }
    if (lleno > 0) {
        lienzo_.fillRect(x0 + 1, y + 1, lleno, kAltoBarra - 2, c565(color));
    }
}

void Pantalla::panelNecesidades(const Necesidades& n) {
    constexpr int y = 108;
    barra(4, y, n.saciedad, "COM");
    barra(63, y, n.animo, "ANI");
    barra(122, y, n.energia, "ENE");
    barra(181, y, n.limpieza, "LIM");
}

void Pantalla::franjaInferior(uint32_t ahoraMs) {
    lienzo_.fillRect(0, kAltoPantalla - 13, kAnchoPantalla, 13, c565(kSombra));

    if (mensaje_[0] != '\0' && ahoraMs < mensajeHastaMs_) {
        textoCorto(lienzo_, 3, kAltoPantalla - 10, kClaro, mensaje_);
        return;
    }
    mensaje_[0] = '\0';
    textoCorto(lienzo_, 0, kAltoPantalla - 10, kTenue,
               "A:comer L:lavar J:jugar D:dormir M:curar");
}

void Pantalla::dibujar(const Bicho& b, const Animador& animador, uint32_t ahoraMs) {
    lienzo_.fillSprite(c565(kFondo));

    habitacion(b, animador);
    franjaSuperior(b);

    if (b.etapa() == Etapa::Huevo) {
        const uint32_t faltan = bichito::tiempos::kEclosion - b.edadSegundos();
        char aviso[40];
        snprintf(aviso, sizeof(aviso), "nace en %lu s",
                 static_cast<unsigned long>(faltan));
        textoCorto(lienzo_, 4, 110, kClaro, aviso);
    } else {
        panelNecesidades(b.necesidades());
        if (!b.vivo()) {
            textoCorto(lienzo_, 4, 16, kTenue, "R: de nuevo");
        }
    }

    franjaInferior(ahoraMs);
    lienzo_.pushSprite(0, 0);
}
