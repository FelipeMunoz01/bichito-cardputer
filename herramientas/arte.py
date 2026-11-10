#!/usr/bin/env python3
"""Convierte el arte del bicho en algo que el firmware pueda dibujar.

El formato de origen es arte/bicho.txt: texto plano, un caracter por pixel, con
los cuadros agrupados en clips. Se edita a mano sin herramientas.

    python3 herramientas/arte.py

Genera tres cosas:

    src/ArteBicho.h    los cuadros empaquetados a 4 bits para el Cardputer
    arte/previa.png    todos los cuadros en una hoja, para revisarlos
    arte/previa.gif    los clips animados, para ver el movimiento sin flashear

Y para traer arte de fuera, por ejemplo una hoja de sprites descargada:

    python3 herramientas/arte.py importar hoja.png 32x20 > arte/bicho.txt

Eso la traduce al mismo formato de texto, con un caracter por color. De ahi en
adelante el arte importado y el dibujado a mano son la misma cosa.
"""

import pathlib
import struct
import sys
import zlib

RAIZ = pathlib.Path(__file__).resolve().parent.parent
ORIGEN = RAIZ / "arte" / "bicho.txt"
CABECERA = RAIZ / "src" / "ArteBicho.h"
PNG = RAIZ / "arte" / "previa.png"
GIF = RAIZ / "arte" / "previa.gif"

ESCALA_PNG = 8
ESCALA_GIF = 5


# --------------------------------------------------------------------------
# Lectura del formato de texto
# --------------------------------------------------------------------------

class Arte:
    def __init__(self):
        self.ancho = 0
        self.alto = 0
        self.fondo = (16, 20, 24)
        self.colores = {}      # caracter -> (r, g, b), en orden de aparicion
        self.cuadros = []      # (clip, ms, [filas])

    @property
    def clips(self):
        """Nombre de clip -> (primer cuadro, cantidad), en orden de aparicion."""
        orden = {}
        for i, (clip, _, _) in enumerate(self.cuadros):
            if clip not in orden:
                orden[clip] = [i, 0]
            orden[clip][1] += 1
        return orden


def leer(ruta):
    a = Arte()
    lineas = ruta.read_text(encoding="utf-8").split("\n")
    i = 0
    while i < len(lineas):
        cruda = lineas[i]
        linea = cruda.strip()
        i += 1
        if not linea or linea.startswith("#"):
            continue

        if linea.startswith("tamano"):
            a.ancho, a.alto = (int(v) for v in linea.split(":", 1)[1].split())
        elif linea.startswith("fondo"):
            a.fondo = tuple(int(v) for v in linea.split(":", 1)[1].split())
        elif linea.startswith("color"):
            cabeza, resto = linea.split(":", 1)
            caracter = cabeza.split()[1]
            a.colores[caracter] = tuple(int(v) for v in resto.split())
        elif linea.startswith("cuadro"):
            partes = linea.split()
            if len(partes) != 3:
                raise SystemExit(f"linea {i}: se esperaba 'cuadro <clip> <ms>'")
            clip, ms = partes[1], int(partes[2])
            filas = [l.rstrip("\r") for l in lineas[i:i + a.alto]]
            i += a.alto
            if len(filas) != a.alto:
                raise SystemExit(f"cuadro {clip}: faltan filas al final del archivo")
            for n, fila in enumerate(filas):
                if len(fila) != a.ancho:
                    raise SystemExit(
                        f"cuadro {clip} que empieza en la linea {i - a.alto + n}: "
                        f"la fila {n} tiene {len(fila)} caracteres y se esperaban "
                        f"{a.ancho}\n  {fila!r}")
                desconocidos = set(fila) - set(a.colores) - {"."}
                if desconocidos:
                    raise SystemExit(
                        f"cuadro {clip}, fila {n}: color sin definir "
                        f"{sorted(desconocidos)}")
            a.cuadros.append((clip, ms, filas))
        else:
            raise SystemExit(f"linea {i}: no se entiende {linea!r}")

    if not a.cuadros:
        raise SystemExit("no hay ningun cuadro en el archivo")
    if len(a.colores) > 15:
        raise SystemExit(f"{len(a.colores)} colores: el maximo son 15 mas el fondo")
    return a


def indices(a, filas):
    """Convierte las filas de texto en indices de paleta. El 0 es transparente."""
    orden = {c: n + 1 for n, c in enumerate(a.colores)}
    return [orden.get(ch, 0) for fila in filas for ch in fila]


# --------------------------------------------------------------------------
# Salida para el firmware
# --------------------------------------------------------------------------

def escribir_cabecera(a):
    paleta = [(0, 0, 0)] + list(a.colores.values())
    paleta += [(0, 0, 0)] * (16 - len(paleta))

    partes = [
        "// GENERADO POR herramientas/arte.py, NO EDITAR A MANO.",
        "// El original es arte/bicho.txt. Para rehacerlo:",
        "//     python3 herramientas/arte.py",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace arte {",
        "",
        f"constexpr int kAncho = {a.ancho};",
        f"constexpr int kAlto  = {a.alto};",
        f"constexpr int kBytesPorCuadro = {a.ancho * a.alto // 2};",
        "",
        "// Indice 0 = transparente. El resto es RGB de 8 bits por canal.",
        "constexpr uint32_t kPaleta[16] = {",
    ]
    partes.append("    " + ", ".join(f"0x{r:02X}{g:02X}{b:02X}" for r, g, b in paleta))
    partes += ["};", ""]

    partes.append("// Dos pixeles por byte: primero el de la izquierda.")
    partes.append(f"constexpr uint8_t kCuadros[{len(a.cuadros)}][kBytesPorCuadro] = {{")
    for clip, _, filas in a.cuadros:
        px = indices(a, filas)
        bytes_ = [(px[n] << 4) | px[n + 1] for n in range(0, len(px), 2)]
        partes.append(f"    {{  // {clip}")
        for n in range(0, len(bytes_), a.ancho // 2):
            fila = bytes_[n:n + a.ancho // 2]
            partes.append("        " + ", ".join(f"0x{b:02X}" for b in fila) + ",")
        partes.append("    },")
    partes += ["};", ""]

    partes.append("// Cuanto dura cada cuadro en pantalla, en milisegundos.")
    partes.append(f"constexpr uint16_t kDuraciones[{len(a.cuadros)}] = {{")
    partes.append("    " + ", ".join(str(ms) for _, ms, _ in a.cuadros))
    partes += ["};", ""]

    clips = a.clips
    partes.append("// El orden de este enum es el orden en que aparecen en bicho.txt.")
    partes.append("enum Clip : uint8_t {")
    for nombre in clips:
        partes.append(f"    k{nombre.capitalize()},")
    partes.append("    kClipsTotal,")
    partes += ["};", ""]

    partes.append("struct Tramo {")
    partes.append("    uint8_t primero;")
    partes.append("    uint8_t cantidad;")
    partes.append("};")
    partes.append("constexpr Tramo kClips[kClipsTotal] = {")
    for nombre, (primero, cantidad) in clips.items():
        partes.append(f"    {{{primero}, {cantidad}}},  // {nombre}")
    partes += ["};", "", "}  // namespace arte", ""]

    CABECERA.write_text("\n".join(partes), encoding="utf-8")
    return len(a.cuadros) * (a.ancho * a.alto // 2)


# --------------------------------------------------------------------------
# PNG y GIF de vista previa
# --------------------------------------------------------------------------

def png(ruta, pixeles, ancho, alto):
    crudo = b"".join(b"\x00" + bytes(fila) for fila in pixeles)

    def trozo(tipo, datos):
        return (struct.pack(">I", len(datos)) + tipo + datos
                + struct.pack(">I", zlib.crc32(tipo + datos) & 0xFFFFFFFF))

    ruta.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + trozo(b"IHDR", struct.pack(">IIBBBBB", ancho, alto, 8, 2, 0, 0, 0))
        + trozo(b"IDAT", zlib.compress(crudo, 9))
        + trozo(b"IEND", b""))


def hoja_png(a):
    cols = min(6, len(a.cuadros))
    filas_hoja = (len(a.cuadros) + cols - 1) // cols
    margen = 4
    cw = a.ancho * ESCALA_PNG + margen
    ch = a.alto * ESCALA_PNG + margen
    ancho = cols * cw + margen
    alto = filas_hoja * ch + margen

    paleta = [a.fondo] + list(a.colores.values())
    lienzo = [list(a.fondo) * ancho for _ in range(alto)]

    for n, (_, _, filas) in enumerate(a.cuadros):
        px = indices(a, filas)
        ox = margen + (n % cols) * cw
        oy = margen + (n // cols) * ch
        for y in range(a.alto):
            for x in range(a.ancho):
                color = paleta[px[y * a.ancho + x]]
                for dy in range(ESCALA_PNG):
                    fila = lienzo[oy + y * ESCALA_PNG + dy]
                    for dx in range(ESCALA_PNG):
                        i = (ox + x * ESCALA_PNG + dx) * 3
                        fila[i:i + 3] = list(color)

    png(PNG, lienzo, ancho, alto)


def lzw(datos, mcs):
    limpiar = 1 << mcs
    fin = limpiar + 1
    dic = {bytes([i]): i for i in range(limpiar)}
    siguiente = fin + 1
    ancho_codigo = mcs + 1
    salida = bytearray()
    acum = bits = 0

    def emitir(codigo):
        nonlocal acum, bits
        acum |= codigo << bits
        bits += ancho_codigo
        while bits >= 8:
            salida.append(acum & 0xFF)
            acum >>= 8
            bits -= 8

    emitir(limpiar)
    actual = b""
    for byte in datos:
        nuevo = actual + bytes([byte])
        if nuevo in dic:
            actual = nuevo
            continue
        emitir(dic[actual])
        if siguiente < 4096:
            dic[nuevo] = siguiente
            siguiente += 1
            # Se ensancha un codigo mas tarde de lo que uno esperaria: el
            # decodificador agrega su entrada al leer el codigo siguiente, asi
            # que va una atrasado. Verificado contra sips.
            if siguiente > (1 << ancho_codigo) and ancho_codigo < 12:
                ancho_codigo += 1
        else:
            emitir(limpiar)
            dic = {bytes([i]): i for i in range(limpiar)}
            siguiente = fin + 1
            ancho_codigo = mcs + 1
        actual = nuevo[-1:]
    if actual:
        emitir(dic[actual])
    emitir(fin)
    if bits:
        salida.append(acum & 0xFF)
    return bytes(salida)


def bloques(datos):
    salida = bytearray()
    for n in range(0, len(datos), 255):
        trozo = datos[n:n + 255]
        salida.append(len(trozo))
        salida += trozo
    salida.append(0)
    return bytes(salida)


def gif(a):
    """Los clips uno tras otro, repitiendo en bucle."""
    paleta = [a.fondo] + list(a.colores.values())
    paleta += [(0, 0, 0)] * (16 - len(paleta))

    ancho = a.ancho * ESCALA_GIF
    alto = a.alto * ESCALA_GIF

    salida = bytearray(b"GIF89a")
    salida += struct.pack("<HHBBB", ancho, alto, 0x80 | 0x70 | 3, 0, 0)
    for r, g, b in paleta:
        salida += bytes((r, g, b))
    salida += b"\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00"

    for _, ms, filas in a.cuadros:
        px = indices(a, filas)
        ampliado = bytearray()
        for y in range(a.alto):
            fila = bytearray()
            for x in range(a.ancho):
                fila += bytes([px[y * a.ancho + x]]) * ESCALA_GIF
            ampliado += fila * ESCALA_GIF

        salida += b"\x21\xF9\x04\x04" + struct.pack("<H", max(2, round(ms / 10)))
        salida += b"\x00\x00"
        salida += b"\x2C" + struct.pack("<HHHHB", 0, 0, ancho, alto, 0)
        salida += bytes([4]) + bloques(lzw(bytes(ampliado), 4))

    salida += b"\x3B"
    GIF.write_bytes(bytes(salida))


# --------------------------------------------------------------------------
# Importar una hoja de sprites PNG al formato de texto
# --------------------------------------------------------------------------

def leer_png(ruta):
    datos = ruta.read_bytes()
    if datos[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"{ruta} no es un PNG")

    ancho = alto = profundidad = tipo = None
    paleta = []
    trans = []
    comprimido = bytearray()
    i = 8
    while i < len(datos):
        largo = struct.unpack(">I", datos[i:i + 4])[0]
        tipo_trozo = datos[i + 4:i + 8]
        cuerpo = datos[i + 8:i + 8 + largo]
        i += 12 + largo
        if tipo_trozo == b"IHDR":
            ancho, alto, profundidad, tipo, _, _, entrelazado = struct.unpack(
                ">IIBBBBB", cuerpo)
            if profundidad != 8 or entrelazado:
                raise SystemExit("solo PNG de 8 bits por canal y sin entrelazado")
        elif tipo_trozo == b"PLTE":
            paleta = [tuple(cuerpo[n:n + 3]) for n in range(0, len(cuerpo), 3)]
        elif tipo_trozo == b"tRNS":
            trans = list(cuerpo)
        elif tipo_trozo == b"IDAT":
            comprimido += cuerpo
        elif tipo_trozo == b"IEND":
            break

    canales = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(tipo)
    if canales is None:
        raise SystemExit(f"tipo de color {tipo} no soportado")

    crudo = zlib.decompress(bytes(comprimido))
    paso = ancho * canales
    anterior = bytearray(paso)
    pixeles = []
    pos = 0
    for _ in range(alto):
        filtro = crudo[pos]
        linea = bytearray(crudo[pos + 1:pos + 1 + paso])
        pos += 1 + paso
        for n in range(paso):
            izq = linea[n - canales] if n >= canales else 0
            arriba = anterior[n]
            diag = anterior[n - canales] if n >= canales else 0
            if filtro == 1:
                linea[n] = (linea[n] + izq) & 0xFF
            elif filtro == 2:
                linea[n] = (linea[n] + arriba) & 0xFF
            elif filtro == 3:
                linea[n] = (linea[n] + (izq + arriba) // 2) & 0xFF
            elif filtro == 4:
                p = izq + arriba - diag
                pa, pb, pc = abs(p - izq), abs(p - arriba), abs(p - diag)
                mejor = izq if (pa <= pb and pa <= pc) else (arriba if pb <= pc else diag)
                linea[n] = (linea[n] + mejor) & 0xFF
        anterior = linea

        fila = []
        for x in range(ancho):
            celda = linea[x * canales:(x + 1) * canales]
            if tipo == 3:
                idx = celda[0]
                alfa = trans[idx] if idx < len(trans) else 255
                fila.append(paleta[idx] + (alfa,))
            elif tipo == 2:
                fila.append(tuple(celda) + (255,))
            elif tipo == 6:
                fila.append(tuple(celda))
            elif tipo == 0:
                fila.append((celda[0],) * 3 + (255,))
            else:
                fila.append((celda[0],) * 3 + (celda[1],))
        pixeles.append(fila)
    return ancho, alto, pixeles


def importar(ruta, ancho_cuadro, alto_cuadro):
    ancho, alto, pixeles = leer_png(pathlib.Path(ruta))
    cols = ancho // ancho_cuadro
    filas = alto // alto_cuadro
    if cols == 0 or filas == 0:
        raise SystemExit(f"la hoja es de {ancho}x{alto}: no cabe un cuadro")

    vistos = {}
    letras = "oxbdenwmptgvyz"
    for fila in pixeles:
        for r, g, b, al in fila:
            if al < 128:
                continue
            vistos.setdefault((r, g, b), None)
    if len(vistos) > len(letras):
        raise SystemExit(
            f"la hoja usa {len(vistos)} colores y el maximo son {len(letras)}. "
            "Reduce la paleta en tu editor de pixel art antes de importarla.")
    for n, color in enumerate(vistos):
        vistos[color] = letras[n]

    salida = [
        f"# importado de {pathlib.Path(ruta).name}, cuadros de "
        f"{ancho_cuadro}x{alto_cuadro}",
        "# Pon los nombres de clip que correspondan en cada linea 'cuadro'.",
        f"tamano: {ancho_cuadro} {alto_cuadro}",
        "fondo: 16 20 24",
    ]
    for color, letra in vistos.items():
        salida.append(f"color {letra}: {color[0]} {color[1]} {color[2]}")
    salida.append("")

    for f in range(filas):
        for c in range(cols):
            salida.append("cuadro quieto 120")
            for y in range(alto_cuadro):
                linea = ""
                for x in range(ancho_cuadro):
                    r, g, b, al = pixeles[f * alto_cuadro + y][c * ancho_cuadro + x]
                    linea += "." if al < 128 else vistos[(r, g, b)]
                salida.append(linea)
            salida.append("")
    return "\n".join(salida)


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "importar":
        if len(sys.argv) != 4 or "x" not in sys.argv[3]:
            raise SystemExit("uso: arte.py importar <hoja.png> <ancho>x<alto>")
        ancho, alto = (int(v) for v in sys.argv[3].split("x"))
        print(importar(sys.argv[2], ancho, alto))
        return

    a = leer(ORIGEN)
    bytes_ = escribir_cabecera(a)
    hoja_png(a)
    gif(a)

    print(f"{len(a.cuadros)} cuadros de {a.ancho}x{a.alto}, "
          f"{len(a.colores)} colores, {bytes_} bytes de flash")
    for nombre, (primero, cantidad) in a.clips.items():
        total = sum(ms for _, ms, _ in a.cuadros[primero:primero + cantidad])
        print(f"  {nombre:10s} {cantidad} cuadros, {total} ms")
    print(f"escrito: {CABECERA.relative_to(RAIZ)}, "
          f"{PNG.relative_to(RAIZ)}, {GIF.relative_to(RAIZ)}")


if __name__ == "__main__":
    main()
