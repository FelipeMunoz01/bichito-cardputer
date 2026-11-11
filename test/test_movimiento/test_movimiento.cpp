// Pruebas del contador de pasos y del detector de sacudidas.
//
// No hace falta el BMI270: se le dan señales sintéticas a 100 Hz, que es más o
// menos la frecuencia a la que el sensor entrega datos.
#include <unity.h>

#include <cmath>

#include "Movimiento.h"

using namespace movimiento;

namespace {

constexpr uint32_t kHz = 100;
constexpr uint32_t kMsPorMuestra = 1000 / kHz;

// Alimenta el contador con una sinusoide de la frecuencia y amplitud dadas.
uint32_t caminar(ContadorPasos& c, float hz, float amplitud, float segundos,
                 uint32_t desdeMs = 0) {
    const uint32_t muestras = static_cast<uint32_t>(segundos * kHz);
    for (uint32_t n = 0; n < muestras; ++n) {
        const float t = static_cast<float>(n) / kHz;
        const float mag = 1.0f + amplitud * std::sin(2.0f * 3.14159265f * hz * t);
        c.muestra(mag, desdeMs + n * kMsPorMuestra);
    }
    return c.total();
}

// Ruido chico y determinista, para no depender del generador aleatorio.
float temblor(uint32_t n) {
    return 0.008f * std::sin(static_cast<float>(n) * 1.7f);
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_quieto_no_cuenta_pasos() {
    ContadorPasos c;
    for (uint32_t n = 0; n < 500; ++n) {
        c.muestra(1.0f + temblor(n), n * kMsPorMuestra);
    }
    TEST_ASSERT_EQUAL_UINT32(0, c.total());
}

void test_caminar_a_dos_hz_cuenta_un_paso_por_ciclo() {
    ContadorPasos c;
    // 5 segundos a 2 pasos por segundo son 10 picos.
    const uint32_t pasos = caminar(c, 2.0f, 0.22f, 5.0f);
    TEST_ASSERT_UINT32_WITHIN(2, 10, pasos);
}

void test_caminar_mas_rapido_cuenta_mas() {
    ContadorPasos lento, rapido;
    const uint32_t a = caminar(lento, 1.6f, 0.22f, 5.0f);
    const uint32_t b = caminar(rapido, 2.6f, 0.22f, 5.0f);
    TEST_ASSERT_TRUE(b > a);
}

void test_un_golpe_suelto_no_es_un_paso() {
    ContadorPasos c;
    for (uint32_t n = 0; n < 300; ++n) {
        // Un solo pico de 0,6 g en medio de la calma.
        const float mag = (n >= 150 && n < 160) ? 1.6f : 1.0f;
        c.muestra(mag, n * kMsPorMuestra);
    }
    TEST_ASSERT_EQUAL_UINT32(0, c.total());
}

void test_picos_demasiado_lentos_no_son_caminar() {
    ContadorPasos c;
    // Un pico cada 2 segundos: más lento que maxIntervaloMs, la racha nunca
    // llega a confirmarse.
    TEST_ASSERT_EQUAL_UINT32(0, caminar(c, 0.5f, 0.30f, 10.0f));
}

void test_la_inclinacion_constante_no_cuenta_pasos() {
    ContadorPasos c;
    for (uint32_t n = 0; n < 200; ++n) c.muestra(1.0f, n * kMsPorMuestra);
    // El equipo queda apoyado de canto: la gravedad proyecta distinto y la
    // magnitud cambia de nivel, pero sigue sin moverse.
    for (uint32_t n = 200; n < 800; ++n) c.muestra(0.72f, n * kMsPorMuestra);
    TEST_ASSERT_EQUAL_UINT32(0, c.total());
}

void test_el_agiton_se_detecta() {
    DetectorSacudida d;
    bool hubo = false;
    for (uint32_t n = 0; n < 100; ++n) {  // un segundo de vaivén a 8 Hz
        const float t = static_cast<float>(n) / kHz;
        const float mag = 1.0f + 1.0f * std::sin(2.0f * 3.14159265f * 8.0f * t);
        if (d.muestra(mag, n * kMsPorMuestra)) hubo = true;
    }
    TEST_ASSERT_TRUE(hubo);
}

void test_caminar_no_es_un_agiton() {
    DetectorSacudida d;
    bool hubo = false;
    for (uint32_t n = 0; n < 500; ++n) {
        const float t = static_cast<float>(n) / kHz;
        const float mag = 1.0f + 0.22f * std::sin(2.0f * 3.14159265f * 2.0f * t);
        if (d.muestra(mag, n * kMsPorMuestra)) hubo = true;
    }
    TEST_ASSERT_FALSE(hubo);
}

void test_un_tiron_suelto_no_es_un_agiton() {
    DetectorSacudida d;
    bool hubo = false;
    for (uint32_t n = 0; n < 300; ++n) {
        const float mag = (n >= 150 && n < 162) ? 2.0f : 1.0f;
        if (d.muestra(mag, n * kMsPorMuestra)) hubo = true;
    }
    TEST_ASSERT_FALSE(hubo);
}

void test_el_enfriamiento_evita_contar_el_mismo_agiton() {
    DetectorSacudida d;
    int veces = 0;
    for (uint32_t n = 0; n < 200; ++n) {  // dos segundos agitando sin parar
        const float t = static_cast<float>(n) / kHz;
        const float mag = 1.0f + 1.0f * std::sin(2.0f * 3.14159265f * 8.0f * t);
        if (d.muestra(mag, n * kMsPorMuestra)) ++veces;
    }
    // Con 1,2 s de enfriamiento, dos segundos dan una o dos, nunca una lluvia.
    TEST_ASSERT_TRUE(veces >= 1);
    TEST_ASSERT_TRUE(veces <= 2);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_quieto_no_cuenta_pasos);
    RUN_TEST(test_caminar_a_dos_hz_cuenta_un_paso_por_ciclo);
    RUN_TEST(test_caminar_mas_rapido_cuenta_mas);
    RUN_TEST(test_un_golpe_suelto_no_es_un_paso);
    RUN_TEST(test_picos_demasiado_lentos_no_son_caminar);
    RUN_TEST(test_la_inclinacion_constante_no_cuenta_pasos);
    RUN_TEST(test_el_agiton_se_detecta);
    RUN_TEST(test_caminar_no_es_un_agiton);
    RUN_TEST(test_un_tiron_suelto_no_es_un_agiton);
    RUN_TEST(test_el_enfriamiento_evita_contar_el_mismo_agiton);
    return UNITY_END();
}
