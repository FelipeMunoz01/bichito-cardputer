// Los cuadros del bicho, escritos como arte ASCII de 16 x 16.
//
// Se dibujan a escala 5, o sea 80 x 80 pixeles en pantalla. Editarlos a mano
// es la idea: cambia un caracter y cambia el dibujo, sin herramientas ni SD.
// Cuando el bicho ya te guste, el paso siguiente es leer PNG desde la microSD
// y dejar esto solo como respaldo.
//
//   .  transparente     o  cuerpo        e  ojo
//   x  contorno         b  brillo        m  boca
#pragma once

#include <cstdint>

namespace sprites {

constexpr int kAncho = 16;
constexpr int kAlto  = 16;

constexpr const char* kHuevo[kAlto] = {
    "......xxxx......",
    "....xxooooxx....",
    "...xoooooooox...",
    "..xoooobooooox..",
    ".xoooooooooooox.",
    ".xoooooooooooox.",
    "xoooboooooooboox",
    "xoooooooooooooox",
    "xoooooooooooooox",
    "xoooooooooooooox",
    ".xoobooooooooox.",
    ".xoooooooooooox.",
    "..xoooooooooox..",
    "...xoooooooox...",
    "....xxxxxxxx....",
    "................",
};

constexpr const char* kQuieto[kAlto] = {
    ".....xxxxxx.....",
    "...xoooooooox...",
    "..xoobooooooox..",
    ".xoobooooooooox.",
    ".xooeeooooeeoox.",
    ".xooeeooooeeoox.",
    "xoooooooooooooox",
    "xoooooooooooooox",
    "xooooommmmooooox",
    "xoooooooooooooox",
    ".xoooooooooooox.",
    ".xoooooooooooox.",
    "..xoooooooooox..",
    "...xoooooooox...",
    "....xxxxxxxx....",
    "..xxx......xxx..",
};

// Un pixel mas abajo y una fila mas de panza: alternado con kQuieto da la
// sensacion de que respira.
constexpr const char* kRespira[kAlto] = {
    "................",
    ".....xxxxxx.....",
    "...xoooooooox...",
    "..xoobooooooox..",
    ".xooeeooooeeoox.",
    ".xooeeooooeeoox.",
    "xoooooooooooooox",
    "xoooooooooooooox",
    "xooooommmmooooox",
    "xoooooooooooooox",
    "xoooooooooooooox",
    ".xoooooooooooox.",
    "..xoooooooooox..",
    "...xoooooooox...",
    "....xxxxxxxx....",
    "..xxx......xxx..",
};

constexpr const char* kParpadeo[kAlto] = {
    ".....xxxxxx.....",
    "...xoooooooox...",
    "..xoobooooooox..",
    ".xoobooooooooox.",
    ".xoooooooooooox.",
    ".xooxxooooxxoox.",
    "xoooooooooooooox",
    "xoooooooooooooox",
    "xooooommmmooooox",
    "xoooooooooooooox",
    ".xoooooooooooox.",
    ".xoooooooooooox.",
    "..xoooooooooox..",
    "...xoooooooox...",
    "....xxxxxxxx....",
    "..xxx......xxx..",
};

constexpr const char* kDormido[kAlto] = {
    "................",
    ".....xxxxxx.....",
    "...xoooooooox...",
    "..xoobooooooox..",
    ".xoooooooooooox.",
    ".xooxxooooxxoox.",
    "xoooooooooooooox",
    "xoooooooooooooox",
    "xooooooommooooox",
    "xoooooooooooooox",
    "xoooooooooooooox",
    ".xoooooooooooox.",
    "..xoooooooooox..",
    "...xoooooooox...",
    "....xxxxxxxx....",
    "..xxx......xxx..",
};

// Color en RGB de 8 bits por canal. La conversion a RGB565 la hace Pantalla.
struct Color {
    uint8_t r, g, b;
};

struct Paleta {
    Color cuerpo;
    Color contorno;
    Color brillo;
    Color ojo;
    Color boca;
};

}  // namespace sprites
