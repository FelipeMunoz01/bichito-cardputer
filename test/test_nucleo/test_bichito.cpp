// Pruebas del nucleo, corren en el Mac: pio test -e pruebas
//
// No hace falta el Cardputer enchufado porque lib/nucleo no depende de Arduino.
#include <unity.h>

#include <cmath>

#include "Bichito.h"

using namespace bichito;

namespace {

// Un bicho recien salido del huevo, con las cuatro necesidades en 80.
Bicho reciennacido() {
    Bicho b;
    b.avanzar(tiempos::kEclosion);
    return b;
}

constexpr uint32_t kHora = 3600;

}  // namespace

void setUp() {}
void tearDown() {}

void test_el_huevo_no_acepta_acciones() {
    Bicho b;
    TEST_ASSERT_TRUE(b.etapa() == Etapa::Huevo);
    TEST_ASSERT_FALSE(b.aplicar(Accion::Alimentar));
    TEST_ASSERT_FALSE(b.aplicar(Accion::Jugar));

    b.avanzar(tiempos::kEclosion);
    TEST_ASSERT_TRUE(b.etapa() == Etapa::Cria);
    TEST_ASSERT_TRUE(b.aplicar(Accion::Jugar));
}

void test_el_huevo_no_pasa_hambre() {
    Bicho b;
    b.avanzar(tiempos::kEclosion - 1);
    TEST_ASSERT_EQUAL_FLOAT(80.0f, b.necesidades().saciedad);
}

void test_el_hambre_baja_con_el_tiempo() {
    Bicho b = reciennacido();
    b.avanzar(kHora);
    // 0,18 puntos por minuto por 60 minutos = 10,8
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 69.2f, b.necesidades().saciedad);
    TEST_ASSERT_TRUE(b.humor() == Humor::Contento);
}

void test_alimentar_llena_pero_no_desborda() {
    Bicho b = reciennacido();
    b.avanzar(kHora);
    TEST_ASSERT_TRUE(b.aplicar(Accion::Alimentar));
    TEST_ASSERT_TRUE(b.aplicar(Accion::Alimentar));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, b.necesidades().saciedad);
    // Lleno al tope, un plato mas no corresponde.
    TEST_ASSERT_FALSE(b.aplicar(Accion::Alimentar));
}

void test_jugar_necesita_energia() {
    Bicho b = reciennacido();
    b.avanzar(40000);  // mas de 11 horas despierto
    TEST_ASSERT_TRUE(b.necesidades().energia < 15.0f);
    TEST_ASSERT_FALSE(b.aplicar(Accion::Jugar));
}

void test_dormir_recupera_y_despierta_solo() {
    Bicho b = reciennacido();
    b.avanzar(kHora);
    TEST_ASSERT_TRUE(b.aplicar(Accion::Acostar));
    TEST_ASSERT_TRUE(b.durmiendo());
    TEST_ASSERT_TRUE(b.humor() == Humor::Durmiendo);

    // Venia con 74 de energia: a 0,6 por minuto necesita unos 43 min para
    // llegar a 100, y los minutos que sobran ya los pasa despierto.
    b.avanzar(3000);
    TEST_ASSERT_FALSE(b.durmiendo());
    TEST_ASSERT_TRUE(b.necesidades().energia > 95.0f);
}

void test_el_ruido_lo_despierta() {
    Bicho b = reciennacido();
    b.aplicar(Accion::Acostar);
    Estimulos e;
    e.ruido = true;
    b.avanzar(60, e);
    TEST_ASSERT_FALSE(b.durmiendo());
}

void test_nueve_horas_de_hambre_todavia_no_enferman() {
    Bicho b = reciennacido();
    // La saciedad nace en 80 y cae 0,18 por minuto: llega a 0 a las 7,4 h, asi
    // que a las 9 h lleva 1,6 h de descuido y el umbral son 3 h.
    b.avanzar(9 * kHora);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, b.necesidades().saciedad);
    TEST_ASSERT_FALSE(b.enfermo());
    TEST_ASSERT_TRUE(b.humor() == Humor::Hambriento);
}

void test_trece_horas_de_hambre_enferman_y_el_remedio_cura() {
    Bicho b = reciennacido();
    b.avanzar(13 * kHora);
    TEST_ASSERT_TRUE(b.enfermo());
    TEST_ASSERT_TRUE(b.vivo());
    TEST_ASSERT_TRUE(b.humor() == Humor::Enfermo);

    TEST_ASSERT_TRUE(b.aplicar(Accion::Medicar));
    TEST_ASSERT_FALSE(b.enfermo());
    // Curado y sano, el remedio ya no corresponde.
    TEST_ASSERT_FALSE(b.aplicar(Accion::Medicar));
}

void test_una_semana_de_abandono_lo_mata() {
    Bicho b = reciennacido();
    b.avanzar(7 * 24 * kHora);
    TEST_ASSERT_FALSE(b.vivo());
    TEST_ASSERT_TRUE(b.humor() == Humor::Muerto);
    TEST_ASSERT_FALSE(b.aplicar(Accion::Alimentar));

    b.reiniciar();
    TEST_ASSERT_TRUE(b.vivo());
    TEST_ASSERT_TRUE(b.etapa() == Etapa::Huevo);
}

void test_cobrar_de_golpe_es_igual_que_segundo_a_segundo() {
    Bicho golpe = reciennacido();
    Bicho lento = reciennacido();

    golpe.avanzar(kHora);
    for (int i = 0; i < 3600; ++i) lento.avanzar(1);

    TEST_ASSERT_FLOAT_WITHIN(0.5f, lento.necesidades().saciedad,
                             golpe.necesidades().saciedad);
    TEST_ASSERT_EQUAL_UINT32(lento.edadSegundos(), golpe.edadSegundos());
}

void test_el_guardado_da_la_vuelta_completa() {
    Bicho antes = reciennacido();
    antes.avanzar(5 * kHora);
    antes.aplicar(Accion::Acostar);

    const Guardado g = antes.exportar(1762700000);

    Bicho despues;
    TEST_ASSERT_TRUE(despues.importar(g));
    TEST_ASSERT_EQUAL_UINT32(antes.edadSegundos(), despues.edadSegundos());
    TEST_ASSERT_EQUAL_FLOAT(antes.necesidades().saciedad,
                            despues.necesidades().saciedad);
    TEST_ASSERT_TRUE(despues.durmiendo());

    // Un formato de otra version se rechaza en vez de leerse como basura.
    Guardado viejo = g;
    viejo.version = 99;
    Bicho otro;
    TEST_ASSERT_FALSE(otro.importar(viejo));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_el_huevo_no_acepta_acciones);
    RUN_TEST(test_el_huevo_no_pasa_hambre);
    RUN_TEST(test_el_hambre_baja_con_el_tiempo);
    RUN_TEST(test_alimentar_llena_pero_no_desborda);
    RUN_TEST(test_jugar_necesita_energia);
    RUN_TEST(test_dormir_recupera_y_despierta_solo);
    RUN_TEST(test_el_ruido_lo_despierta);
    RUN_TEST(test_nueve_horas_de_hambre_todavia_no_enferman);
    RUN_TEST(test_trece_horas_de_hambre_enferman_y_el_remedio_cura);
    RUN_TEST(test_una_semana_de_abandono_lo_mata);
    RUN_TEST(test_cobrar_de_golpe_es_igual_que_segundo_a_segundo);
    RUN_TEST(test_el_guardado_da_la_vuelta_completa);
    return UNITY_END();
}
