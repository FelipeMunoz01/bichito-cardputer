#include "Bichito.h"

namespace bichito {
namespace {

float sujetar(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 100.0f) return 100.0f;
    return v;
}

uint32_t restar(uint32_t a, uint32_t b) { return (a > b) ? a - b : 0; }

}  // namespace

void Bicho::limitar() {
    n_.saciedad = sujetar(n_.saciedad);
    n_.animo    = sujetar(n_.animo);
    n_.energia  = sujetar(n_.energia);
    n_.limpieza = sujetar(n_.limpieza);
}

void Bicho::avanzar(uint32_t segundos, const Estimulos& estimulos) {
    if (!vivo_ || segundos == 0) return;

    // Si el equipo estuvo semanas apagado no tiene gracia encontrarse el bicho
    // muerto diez veces: el castigo se corta a los siete dias.
    if (segundos > tiempos::kAusenciaMaxima) segundos = tiempos::kAusenciaMaxima;

    // Los huecos largos se parten en tramos de un minuto. Asi cobrarle ocho
    // horas apagado da el mismo resultado que haberlas pasado encendido, en vez
    // de contar ocho horas de descuido por los ultimos diez minutos de hambre.
    Estimulos pendientes = estimulos;
    while (segundos > 0 && vivo_) {
        const uint32_t tramo = (segundos > 60) ? 60 : segundos;
        paso(tramo, pendientes);
        pendientes = Estimulos{};  // pasos y caricias se cobran una sola vez
        segundos -= tramo;
    }
}

void Bicho::paso(uint32_t segundos, const Estimulos& estimulos) {
    const float minutos = static_cast<float>(segundos) / 60.0f;

    // Un huevo no tiene hambre: solo cuenta el tiempo. La etapa se mira antes
    // de sumar la edad para que el tramo que lo abre no le cobre desgaste.
    const bool eraHuevo = (etapa() == Etapa::Huevo);
    edadSegundos_ += segundos;
    if (eraHuevo) return;

    if (durmiendo_) {
        n_.energia  += ritmo_.recuperaDurmiendo * minutos;
        n_.saciedad -= ritmo_.saciedad * 0.5f * minutos;  // dormido gasta menos
        n_.limpieza -= ritmo_.limpieza * minutos;
        if (n_.energia >= 100.0f) durmiendo_ = false;  // despierta descansado
    } else {
        n_.saciedad -= ritmo_.saciedad * minutos;
        n_.animo    -= ritmo_.animo * minutos;
        n_.energia  -= ritmo_.energia * minutos;
        n_.limpieza -= ritmo_.limpieza * minutos;
        n_.animo    += ritmo_.animoPorPaso * static_cast<float>(estimulos.pasos);
    }

    if (estimulos.caricia) n_.animo += 4.0f;
    if (estimulos.ruido) {
        n_.animo -= 3.0f;
        durmiendo_ = false;  // el ruido lo despierta, y de malas
    }

    limitar();

    // Descuido: pasar hambre o estar sucio acumula; atenderlo descuenta.
    const bool descuidado = (n_.saciedad <= 0.0f || n_.limpieza <= 0.0f);
    descuidoSegundos_ = descuidado ? descuidoSegundos_ + segundos
                                   : restar(descuidoSegundos_, segundos);
    if (descuidoSegundos_ >= tiempos::kDescuidoEnferma) enfermo_ = true;

    enfermoSegundos_ = enfermo_ ? enfermoSegundos_ + segundos : 0;
    if (enfermoSegundos_ >= tiempos::kEnfermoMuere) vivo_ = false;
}

bool Bicho::aplicar(Accion accion) {
    if (!vivo_ || accion == Accion::Ninguna) return false;
    if (etapa() == Etapa::Huevo) return false;  // todavia no hay con quien hablar

    switch (accion) {
        case Accion::Alimentar:
            if (durmiendo_) return false;
            if (n_.saciedad > 95.0f) return false;  // empachado
            n_.saciedad += 25.0f;
            n_.limpieza -= 3.0f;
            break;

        case Accion::Limpiar:
            if (n_.limpieza > 95.0f) return false;
            n_.limpieza = 100.0f;
            n_.animo += 2.0f;
            break;

        case Accion::Jugar:
            if (durmiendo_) return false;
            if (n_.energia < 15.0f) return false;  // esta muerto de sueno
            n_.animo    += 18.0f;
            n_.energia  -= 8.0f;
            n_.saciedad -= 4.0f;
            break;

        case Accion::Acostar:
            if (durmiendo_) return false;
            durmiendo_ = true;
            break;

        case Accion::Despertar:
            if (!durmiendo_) return false;
            durmiendo_ = false;
            if (n_.energia < 50.0f) n_.animo -= 5.0f;  // lo despertaste a medias
            break;

        case Accion::Medicar:
            if (!enfermo_) return false;
            enfermo_ = false;
            enfermoSegundos_ = 0;
            descuidoSegundos_ = 0;
            n_.animo -= 4.0f;  // el remedio es amargo
            n_.saciedad += 5.0f;
            break;

        default:
            return false;
    }

    limitar();
    return true;
}

Humor Bicho::humor() const {
    if (!vivo_) return Humor::Muerto;
    if (durmiendo_) return Humor::Durmiendo;
    if (enfermo_) return Humor::Enfermo;
    if (n_.saciedad < 25.0f) return Humor::Hambriento;
    if (n_.limpieza < 25.0f) return Humor::Sucio;
    if (n_.energia < 20.0f) return Humor::Cansado;
    if (n_.animo < 30.0f) return Humor::Aburrido;
    return Humor::Contento;
}

Etapa Bicho::etapa() const {
    if (edadSegundos_ < tiempos::kEclosion) return Etapa::Huevo;
    if (edadSegundos_ < tiempos::kFinCria) return Etapa::Cria;
    if (edadSegundos_ < tiempos::kFinJoven) return Etapa::Joven;
    return Etapa::Adulto;
}

void Bicho::reiniciar() {
    n_ = Necesidades{};
    edadSegundos_ = 0;
    descuidoSegundos_ = 0;
    enfermoSegundos_ = 0;
    durmiendo_ = false;
    enfermo_ = false;
    vivo_ = true;
}

Guardado Bicho::exportar(int64_t epoch) const {
    Guardado g;
    g.version          = kVersionGuardado;
    g.saciedad         = n_.saciedad;
    g.animo            = n_.animo;
    g.energia          = n_.energia;
    g.limpieza         = n_.limpieza;
    g.edadSegundos     = edadSegundos_;
    g.descuidoSegundos = descuidoSegundos_;
    g.enfermoSegundos  = enfermoSegundos_;
    g.durmiendo        = durmiendo_;
    g.enfermo          = enfermo_;
    g.vivo             = vivo_;
    g.ultimoEpoch      = epoch;
    return g;
}

bool Bicho::importar(const Guardado& g) {
    if (g.version != kVersionGuardado) return false;
    n_.saciedad       = g.saciedad;
    n_.animo          = g.animo;
    n_.energia        = g.energia;
    n_.limpieza       = g.limpieza;
    edadSegundos_     = g.edadSegundos;
    descuidoSegundos_ = g.descuidoSegundos;
    enfermoSegundos_  = g.enfermoSegundos;
    durmiendo_        = g.durmiendo;
    enfermo_          = g.enfermo;
    vivo_             = g.vivo;
    limitar();
    return true;
}

const char* nombre(Humor h) {
    switch (h) {
        case Humor::Contento:   return "contento";
        case Humor::Aburrido:   return "aburrido";
        case Humor::Hambriento: return "con hambre";
        case Humor::Sucio:      return "sucio";
        case Humor::Cansado:    return "con sueno";
        case Humor::Durmiendo:  return "durmiendo";
        case Humor::Enfermo:    return "enfermo";
        case Humor::Muerto:     return "se murio";
    }
    return "?";
}

const char* nombre(Etapa e) {
    switch (e) {
        case Etapa::Huevo:  return "huevo";
        case Etapa::Cria:   return "cria";
        case Etapa::Joven:  return "joven";
        case Etapa::Adulto: return "adulto";
    }
    return "?";
}

}  // namespace bichito
