"""Génère les images des animations plein écran du pomodoro.

L'écran SPI est trop lent pour redessiner 480 x 320 pixels à chaque image, donc
chaque scène = un fond fixe dessiné une fois + une petite zone animée :
  - pause   : CHAT_FOND (chat qui dort) + CHAT_ZZZ (les « z » qui s'envolent, en boucle) ;
  - travail : TRAVAIL_FOND (collines) ; les nuages sont dessinés par le sketch
              dans la bande du haut, qui ne contient que du fond.

Produit :
  - pomodoro/animations.h : les GIF sous forme de tableaux C, inclus par le sketch
  - outils/apercu_chat.gif, outils/apercu_travail.png : aperçus pour le Mac

Usage : python genere_animations.py   (nécessite Pillow)
"""
import io
import math
from pathlib import Path

from PIL import Image, ImageDraw

W, H = 480, 320
SS = 4                 # suréchantillonnage pour des bords lisses

# Palette (doit correspondre aux COL_* du sketch) — dark mode, base #0f0f0f / #dcd7d0 / #4c63d2
BG = (0x0f, 0x0f, 0x0f)
TEXT = (0xdc, 0xd7, 0xd0)
WORK = (0x4c, 0x63, 0xd2)
WORK_LIGHT = (0x7e, 0x8c, 0xd1)
PAUSE = (0x37, 0x46, 0x8e)
CAT = (0x8d, 0x97, 0xd1)
CUSHION = (0x38, 0x37, 0x36)
CUSHION_SHADE = (0x28, 0x27, 0x26)

# Zone animée des « z », à l'écran (x0, y0, x1, y1)
ZZZ_BOX = (290, 30, 410, 160)
ZZZ_FRAMES = 24
ZZZ_MS = 120

ROOT = Path(__file__).resolve().parent.parent


def canvas():
    img = Image.new("RGB", (W * SS, H * SS), BG)
    return img, ImageDraw.Draw(img)


def s(*v):
    return [int(round(a * SS)) for a in v]


def finish(img):
    return img.resize((W, H), Image.LANCZOS)


def blend(c1, c2, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))


# ---------- Scène de pause : le chat qui dort ----------

def draw_cat(d):
    # Coussin
    d.ellipse(s(40, 262, 330, 318), fill=CUSHION_SHADE)
    d.ellipse(s(40, 255, 330, 308), fill=CUSHION)
    # Queue enroulée devant le corps
    d.arc(s(55, 200, 315, 312), 70, 168, fill=CAT, width=s(18)[0])
    # Corps en boule, avec des rayures sur le dos
    d.ellipse(s(65, 175, 275, 295), fill=CAT)
    for k in range(4):
        x = 105 + k * 32
        d.arc(s(x, 182, x + 34, 222), 200, 300, fill=PAUSE, width=s(6)[0])
    # Bout de la queue
    d.ellipse(s(68, 262, 96, 288), fill=PAUSE)
    # Tête posée sur le côté droit
    d.polygon(s(208, 190, 214, 140, 244, 170), fill=CAT)      # oreille gauche
    d.polygon(s(262, 168, 294, 142, 296, 192), fill=CAT)      # oreille droite
    d.polygon(s(215, 182, 218, 154, 236, 172), fill=PAUSE)
    d.polygon(s(268, 170, 289, 153, 290, 184), fill=PAUSE)
    d.ellipse(s(200, 160, 305, 250), fill=CAT)
    # Yeux fermés, nez, moustaches
    d.arc(s(218, 196, 242, 214), 20, 160, fill=TEXT, width=s(3)[0])
    d.arc(s(262, 196, 286, 214), 20, 160, fill=TEXT, width=s(3)[0])
    d.polygon(s(247, 218, 257, 218, 252, 224), fill=PAUSE)
    for dy in (-3, 4):
        d.line(s(232, 226 + dy, 206, 224 + dy * 2), fill=PAUSE, width=s(2)[0])
        d.line(s(272, 226 + dy, 298, 224 + dy * 2), fill=PAUSE, width=s(2)[0])


def draw_z(d, cx, cy, size, color):
    """Un « z » dessiné en 3 traits, centré sur (cx, cy)."""
    h = size / 2
    pts = [(cx - h, cy - h), (cx + h, cy - h), (cx - h, cy + h), (cx + h, cy + h)]
    d.line(s(*[c for p in pts for c in p]), fill=color, width=max(1, int(size * SS / 5)), joint="curve")


def draw_zzz(d, frame):
    # 3 « z » qui montent en diagonale depuis la tête, décalés d'un tiers de cycle
    for k in range(3):
        t = ((frame / ZZZ_FRAMES) + k / 3) % 1.0
        x = 305 + t * 75
        y = 145 - t * 95 + 6 * math.sin(t * 2 * math.pi)
        size = 10 + t * 18
        fade = min(1.0, t / 0.15, (1 - t) / 0.25)   # apparition puis disparition douces
        draw_z(d, x, y, size, blend(BG, TEXT, max(0.0, fade)))


def scene_chat(frame=None):
    img, d = canvas()
    draw_cat(d)
    if frame is not None:
        draw_zzz(d, frame)
    return finish(img)


# ---------- Scène de travail : les collines (les nuages sont dessinés par le sketch) ----------

def scene_travail():
    img, d = canvas()
    d.ellipse(s(-200, 245, 380, 560), fill=WORK_LIGHT)
    d.ellipse(s(150, 262, 720, 600), fill=WORK)
    return finish(img)


# ---------- Encodage ----------

def to_paletted(frames, colors):
    """Palette commune à toutes les images, dans laquelle la couleur du fond reste exacte."""
    w, h = frames[0].size
    sheet = Image.new("RGB", (w, h * len(frames)))
    for i, f in enumerate(frames):
        sheet.paste(f, (0, i * h))
    pal = sheet.quantize(colors=colors - 1, dither=Image.Dither.NONE)
    palette = pal.getpalette()[: (colors - 1) * 3] + list(BG)
    pal_img = Image.new("P", (1, 1))
    pal_img.putpalette(palette + [0] * (768 - len(palette)))
    return [f.quantize(palette=pal_img, dither=Image.Dither.NONE) for f in frames]


def gif_bytes(frames, **kw):
    buf = io.BytesIO()
    frames[0].save(buf, format="GIF", save_all=len(frames) > 1, append_images=frames[1:],
                   optimize=False, **kw)
    return buf.getvalue()


def c_array(name, data):
    lines = [", ".join(f"0x{b:02x}" for b in data[i:i + 16]) for i in range(0, len(data), 16)]
    return f"const uint8_t {name}[] PROGMEM = {{\n  " + ",\n  ".join(lines) + "\n};\n"


def main():
    chat_fond = gif_bytes(to_paletted([scene_chat()], 64))
    travail_fond = gif_bytes(to_paletted([scene_travail()], 32))

    full = [scene_chat(i) for i in range(ZZZ_FRAMES)]
    zzz = gif_bytes(to_paletted([f.crop(ZZZ_BOX) for f in full], 32),
                    duration=ZZZ_MS, loop=0, disposal=1)

    out = ROOT / "outils"
    (out / "apercu_chat.gif").write_bytes(
        gif_bytes(to_paletted(full, 64), duration=ZZZ_MS, loop=0, disposal=1))
    scene_travail().save(out / "apercu_travail.png")

    x0, y0, x1, y1 = ZZZ_BOX
    header = (
        "// Généré par outils/genere_animations.py — ne pas modifier à la main\n"
        "#pragma once\n#include <Arduino.h>\n\n"
        f"// Zone animée des « z » à l'écran\n"
        f"#define ZZZ_X {x0}\n#define ZZZ_Y {y0}\n#define ZZZ_W {x1 - x0}\n#define ZZZ_H {y1 - y0}\n\n"
        + c_array("CHAT_FOND", chat_fond) + "\n"
        + c_array("CHAT_ZZZ", zzz) + "\n"
        + c_array("TRAVAIL_FOND", travail_fond)
    )
    (ROOT / "pomodoro" / "animations.h").write_text(header)
    print(f"chat : {len(chat_fond)} o, zzz : {len(zzz)} o, travail : {len(travail_fond)} o")


if __name__ == "__main__":
    main()
