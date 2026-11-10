#!/usr/bin/env python3
"""Dibuja los sprites del bicho en un PNG, para verlos sin flashear nada.

Los cuadros se leen de src/Sprites.h, asi que el arte no se puede desincronizar:
si editas el arte ASCII, esto muestra lo editado. Las paletas si son una copia
de paletaPara() en Pantalla.cpp, solo para la vista previa.

    python3 herramientas/vista_previa.py [salida.png]
"""

import pathlib
import re
import struct
import sys
import zlib

RAIZ = pathlib.Path(__file__).resolve().parent.parent
SPRITES = RAIZ / "src" / "Sprites.h"

ESCALA = 5
FONDO = (16, 20, 24)
MARGEN = 10

# cuerpo, contorno, brillo, ojo, boca
PALETAS = {
    "huevo":      ((232, 227, 212), (92, 86, 74), (255, 255, 255), (60, 56, 48), (60, 56, 48)),
    "contento":   ((92, 202, 120), (20, 48, 38), (176, 238, 192), (18, 28, 24), (198, 84, 92)),
    "aburrido":   ((124, 170, 162), (20, 48, 38), (176, 212, 206), (18, 28, 24), (198, 84, 92)),
    "con hambre": ((214, 190, 108), (74, 60, 24), (242, 226, 170), (18, 28, 24), (198, 84, 92)),
    "sucio":      ((150, 124, 84), (58, 46, 30), (190, 166, 126), (18, 28, 24), (198, 84, 92)),
    "con sueno":  ((120, 142, 192), (32, 42, 72), (176, 192, 230), (18, 28, 24), (198, 84, 92)),
    "durmiendo":  ((96, 120, 176), (28, 38, 66), (150, 172, 216), (18, 28, 24), (198, 84, 92)),
    "enfermo":    ((162, 182, 118), (56, 68, 34), (200, 216, 164), (18, 28, 24), (120, 150, 90)),
    "muerto":     ((110, 110, 116), (44, 44, 48), (150, 150, 156), (18, 28, 24), (90, 90, 94)),
}

CANAL = {"o": 0, "x": 1, "b": 2, "e": 3, "m": 4}


def leer_cuadros(ruta):
    """Saca cada constexpr const char* kNombre[kAlto] = { ... } del encabezado."""
    texto = ruta.read_text(encoding="utf-8")
    patron = re.compile(
        r"constexpr const char\* (k\w+)\[kAlto\] = \{(.*?)\};", re.DOTALL
    )
    cuadros = {}
    for nombre, cuerpo in patron.findall(texto):
        filas = re.findall(r'"([^"]*)"', cuerpo)
        if len(filas) != 16 or any(len(f) != 16 for f in filas):
            raise SystemExit(f"{nombre}: se esperaban 16 filas de 16 caracteres")
        cuadros[nombre] = filas
    return cuadros


class Lienzo:
    def __init__(self, ancho, alto, fondo):
        self.ancho, self.alto = ancho, alto
        self.px = [list(fondo) * ancho for _ in range(alto)]

    def rect(self, x, y, w, h, color):
        for fy in range(y, min(y + h, self.alto)):
            fila = self.px[fy]
            for fx in range(x, min(x + w, self.ancho)):
                fila[fx * 3 : fx * 3 + 3] = list(color)

    def cuadro(self, arte, x0, y0, paleta):
        for f, linea in enumerate(arte):
            for c, ch in enumerate(linea):
                if ch not in CANAL:
                    continue
                self.rect(x0 + c * ESCALA, y0 + f * ESCALA, ESCALA, ESCALA,
                          paleta[CANAL[ch]])

    def png(self, ruta):
        crudo = b"".join(b"\x00" + bytes(fila) for fila in self.px)

        def trozo(tipo, datos):
            return (struct.pack(">I", len(datos)) + tipo + datos
                    + struct.pack(">I", zlib.crc32(tipo + datos) & 0xFFFFFFFF))

        ruta.write_bytes(
            b"\x89PNG\r\n\x1a\n"
            + trozo(b"IHDR", struct.pack(">IIBBBBB", self.ancho, self.alto, 8, 2, 0, 0, 0))
            + trozo(b"IDAT", zlib.compress(crudo, 9))
            + trozo(b"IEND", b"")
        )


def main():
    salida = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else RAIZ / "vista_previa.png"
    cuadros = leer_cuadros(SPRITES)

    lado = 16 * ESCALA
    paso = lado + MARGEN

    # Fila 1: los cuadros de animacion. Fila 2: el mismo cuerpo en cada humor.
    fila1 = ["kHuevo", "kQuieto", "kRespira", "kParpadeo", "kDormido"]
    fila2 = [h for h in PALETAS if h != "huevo"]

    ancho = MARGEN + max(len(fila1), len(fila2)) * paso
    alto = MARGEN + 2 * paso
    lienzo = Lienzo(ancho, alto, FONDO)

    for i, nombre in enumerate(fila1):
        paleta = PALETAS["huevo" if nombre == "kHuevo" else "contento"]
        lienzo.cuadro(cuadros[nombre], MARGEN + i * paso, MARGEN, paleta)

    for i, humor in enumerate(fila2):
        arte = cuadros["kDormido" if humor == "durmiendo" else "kQuieto"]
        lienzo.cuadro(arte, MARGEN + i * paso, MARGEN + paso, PALETAS[humor])

    lienzo.png(salida)
    print(f"arriba:  {'  '.join(fila1)}")
    print(f"abajo:   {'  '.join(fila2)}")
    print(f"escrito: {salida}")


if __name__ == "__main__":
    main()
