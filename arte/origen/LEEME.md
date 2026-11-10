# Hojas originales del lobo

Autor: **ScratchiO**, del paquete *Animated Wild Animals* publicado en
OpenGameArt: https://opengameart.org/content/animated-wild-animals

Licencia: **CC0 1.0, dominio público**. No exige atribución, pero se deja
igual porque corresponde.

Las tres hojas vienen tal como se descargaron, con cuadros de 64 x 40:

| Archivo | Cuadros | Qué es |
|---|---|---|
| `Wolf_Walk.png` | 8 | ciclo de caminata |
| `Wolf_Run.png` | 6 | ciclo de carrera |
| `Wolf_Howl.png` | 10 | parado, levanta la cabeza y aúlla |

Para regenerar `arte/bicho.txt` desde cero:

```bash
python3 herramientas/arte.py importar 64x40 \
  quieto=arte/origen/Wolf_Howl.png:0-1 \
  camina=arte/origen/Wolf_Walk.png \
  corre=arte/origen/Wolf_Run.png \
  aulla=arte/origen/Wolf_Howl.png > arte/bicho.txt
```

Eso recorta el margen vacío común y deja cuadros de 54 x 35. Ojo: hay que
volver a agregar a mano el clip `huevo` y ajustar las duraciones, porque eso
no sale de las hojas.
