#include "Pantalla.h"

#include <cstdio>
#include <cstring>

using bichito::Bicho;
using bichito::Etapa;
using bichito::Humor;
using bichito::Necesidades;
using sprites::Color;
using sprites::Paleta;

namespace {

constexpr int kAnchoPantalla = 240;
constexpr int kAltoPantalla  = 135;
constexpr int kEscala        = 5;  // 16 x 16 a escala 5 = 80 x 80

const Color kFondo  = {16, 20, 24};
const Color kTenue  = {110, 120, 130};
const Color kClaro  = {225, 230, 235};
const Color kSombra = {30, 36, 42};

uint16_t c565(const Color& c) { return M5Cardputer.Display.color565(c.r, c.g, c.b); }

// Cada humor tiene su propio color de cuerpo. Es la via mas corta para que se
// note de una mirada, sin dibujar un cuadro distinto para cada estado.
Paleta paletaPara(Humor h, Etapa etapa) {
    if (etapa == Etapa::Huevo) {
        return {{232, 227, 212}, {92, 86, 74}, {255, 255, 255}, {60, 56, 48}, {60, 56, 48}};
    }
    const Color contorno = {20, 48, 38};
    const Color ojo      = {18, 28, 24};
    const Color boca     = {198, 84, 92};
    switch (h) {
        case Humor::Contento:   return {{92, 202, 120}, contorno, {176, 238, 192}, ojo, boca};
        case Humor::Aburrido:   return {{124, 170, 162}, contorno, {176, 212, 206}, ojo, boca};
        case Humor::Hambriento: return {{214, 190, 108}, {74, 60, 24}, {242, 226, 170}, ojo, boca};
        case Humor::Sucio:      return {{150, 124, 84}, {58, 46, 30}, {190, 166, 126}, ojo, boca};
        case Humor::Cansado:    return {{120, 142, 192}, {32, 42, 72}, {176, 192, 230}, ojo, boca};
        case Humor::Durmiendo:  return {{96, 120, 176}, {28, 38, 66}, {150, 172, 216}, ojo, boca};
        case Humor::Enfermo:    return {{162, 182, 118}, {56, 68, 34}, {200, 216, 164}, ojo, {120, 150, 90}};
        case Humor::Muerto:     return {{110, 110, 116}, {44, 44, 48}, {150, 150, 156}, ojo, {90, 90, 94}};
    }
    return {{92, 202, 120}, contorno, {176, 238, 192}, ojo, boca};
}

// Elige el cuadro segun el estado y el reloj de animacion.
const char* const* cuadroPara(const Bicho& bicho, uint32_t ms) {
    if (bicho.etapa() == Etapa::Huevo) return sprites::kHuevo;
    if (!bicho.vivo()) return sprites::kParpadeo;
    if (bicho.durmiendo()) return sprites::kDormido;
    // Un parpadeo de 120 ms cada 4 s rompe la repeticion.
    if (ms % 4000 < 120) return sprites::kParpadeo;
    return (ms % 1200 < 600) ? sprites::kQuieto : sprites::kRespira;
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
    lienzo_.setTextFont(&fonts::Font0);
    lienzo_.setTextSize(1);
}

void Pantalla::mensaje(const char* texto) {
    snprintf(mensaje_, sizeof(mensaje_), "%s", texto);
    mensajeHastaMs_ = millis() + 1500;
}

void Pantalla::cuadro(const char* const* arte, int x0, int y0, int escala,
                      const Paleta& paleta) {
    for (int fila = 0; fila < sprites::kAlto; ++fila) {
        const char* linea = arte[fila];
        for (int col = 0; col < sprites::kAncho; ++col) {
            Color c;
            switch (linea[col]) {
                case 'o': c = paleta.cuerpo;   break;
                case 'x': c = paleta.contorno; break;
                case 'b': c = paleta.brillo;   break;
                case 'e': c = paleta.ojo;      break;
                case 'm': c = paleta.boca;     break;
                default: continue;  // '.' no se dibuja
            }
            lienzo_.fillRect(x0 + col * escala, y0 + fila * escala, escala, escala,
                             c565(c));
        }
    }
}

void Pantalla::franjaSuperior(const Bicho& bicho) {
    lienzo_.fillRect(0, 0, kAnchoPantalla, 13, c565(kSombra));

    textoCorto(lienzo_, 3, 3, kClaro, "BICHITO");

    char medio[32];
    const uint32_t horas = bicho.edadSegundos() / 3600;
    const uint32_t mins  = (bicho.edadSegundos() % 3600) / 60;
    snprintf(medio, sizeof(medio), "%s %luh%02lu", bichito::nombre(bicho.etapa()),
             static_cast<unsigned long>(horas), static_cast<unsigned long>(mins));
    textoCorto(lienzo_, 62, 3, kTenue, medio);

    char bat[8];
    snprintf(bat, sizeof(bat), "%d%%", M5Cardputer.Power.getBatteryLevel());
    textoCorto(lienzo_, kAnchoPantalla - 4 - static_cast<int>(strlen(bat)) * 6, 3,
               kTenue, bat);
}

void Pantalla::barra(int x, int y, float valor, const char* etiqueta) {
    constexpr int kAnchoBarra = 76;
    constexpr int kAltoBarra  = 7;

    textoCorto(lienzo_, x, y, kTenue, etiqueta);

    const int x0 = x + 24;
    lienzo_.drawRect(x0, y - 1, kAnchoBarra, kAltoBarra, c565(kTenue));

    const int lleno = static_cast<int>((kAnchoBarra - 2) * valor / 100.0f);
    Color color = {92, 202, 120};
    if (valor < 30.0f) {
        color = {214, 92, 92};
    } else if (valor < 60.0f) {
        color = {222, 182, 92};
    }
    if (lleno > 0) {
        lienzo_.fillRect(x0 + 1, y, lleno, kAltoBarra - 2, c565(color));
    }
}

void Pantalla::panelNecesidades(const Necesidades& n) {
    constexpr int x = 122;
    barra(x, 26, n.saciedad, "COM");
    barra(x, 44, n.animo,    "ANI");
    barra(x, 62, n.energia,  "ENE");
    barra(x, 80, n.limpieza, "LIM");
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

void Pantalla::dibujar(const Bicho& bicho, uint32_t ahoraMs) {
    lienzo_.fillSprite(c565(kFondo));

    const Humor  humor  = bicho.humor();
    const Paleta paleta = paletaPara(humor, bicho.etapa());

    // Sombra en el piso, para que el bicho no quede flotando en el vacio.
    lienzo_.fillEllipse(58, 114, 30, 5, c565(kSombra));

    // El huevo se bambolea; el bicho contento da saltitos.
    int desplazamientoY = 0;
    if (bicho.etapa() == Etapa::Huevo) {
        desplazamientoY = (ahoraMs % 1000 < 500) ? 0 : 2;
    } else if (humor == Humor::Contento && ahoraMs % 1200 >= 600) {
        desplazamientoY = -3;
    }

    cuadro(cuadroPara(bicho, ahoraMs), 18, 30 + desplazamientoY, kEscala, paleta);

    if (bicho.durmiendo()) {
        textoCorto(lienzo_, 92, 32, kTenue, "z");
        textoCorto(lienzo_, 98, 24, kTenue, "Z");
    }

    franjaSuperior(bicho);

    if (bicho.etapa() == Etapa::Huevo) {
        const uint32_t faltan = bichito::tiempos::kEclosion - bicho.edadSegundos();
        char aviso[32];
        snprintf(aviso, sizeof(aviso), "nace en %lus",
                 static_cast<unsigned long>(faltan));
        textoCorto(lienzo_, 122, 44, kClaro, aviso);
        textoCorto(lienzo_, 122, 60, kTenue, "esperalo");
    } else {
        panelNecesidades(bicho.necesidades());
        char estado[32];
        snprintf(estado, sizeof(estado), "esta %s", bichito::nombre(humor));
        textoCorto(lienzo_, 122, 100, kClaro, estado);
        if (!bicho.vivo()) {
            textoCorto(lienzo_, 122, 110, kTenue, "R: de nuevo");
        }
    }

    franjaInferior(ahoraMs);
    lienzo_.pushSprite(0, 0);
}
