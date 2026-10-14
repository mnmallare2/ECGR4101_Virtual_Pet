"""Rebuild previews, master art and RGB565 C arrays from hand-edited frame PNGs.

Usage:  python3 rebuild_from_pngs.py <frames_folder> [output_folder]

Expects files named  cat_<animation>_<index>.png  (e.g. cat_idle_0.png), each 64x64.
Frame counts are detected automatically, so adding or removing frames is fine.
Writes:
  <out>/preview/cat_<anim>.gif, cat_contact_sheet.png
  <out>/master/cat_master_512.png
  <out>/c/sprites.c, sprites.h   (copy these into App/Src and App/Inc)
"""
import os, re, sys
from collections import defaultdict
from PIL import Image, ImageDraw

W = H = 64
BG = None  # detected from the frames' top-left pixel
ORDER = ["idle", "feed", "play", "sleep", "death", "runaway"]
DURATION = {"idle": 300, "feed": 350, "play": 220, "sleep": 500, "death": 450, "runaway": 400}
PATTERN = re.compile(r"^cat_([a-z]+)_(\d+)\.png$", re.I)


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def load(path):
    img = Image.open(path).convert("RGBA")
    if img.size != (W, H):
        sys.exit(f"ERROR: {os.path.basename(path)} is {img.size[0]}x{img.size[1]}, expected {W}x{H}. "
                 "Re-export from Piskel at scale 1.0x.")
    flat = Image.new("RGB", (W, H), BG)  # transparent pixels become the background color
    flat.paste(img, mask=img.split()[3])        # any transparent pixels become the white background
    return flat


def transparent(img):
    """Make every background-colored pixel transparent. The art never uses the exact
    background color (outline is dark brown, death bow is dark gray), so gaps enclosed
    by the tail, halo or letters become transparent too."""
    rgba = img.convert("RGBA"); p = rgba.load()
    for y in range(H):
        for x in range(W):
            if p[x, y][:3] == BG:
                p[x, y] = (0, 0, 0, 0)
    return rgba


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else "cat_rebuilt"

    groups = defaultdict(dict)
    for f in os.listdir(src):
        m = PATTERN.match(f)
        if m:
            groups[m.group(1).lower()][int(m.group(2))] = os.path.join(src, f)
    if not groups:
        sys.exit("ERROR: no files named like cat_idle_0.png found in " + src)

    global BG
    first_file = next(iter(groups[sorted(groups)[0]].values()))
    BG = Image.open(first_file).convert("RGB").getpixel((0, 0))
    print(f"  background color {BG} -> RGB565 0x{rgb565(*BG):04X}")

    anims = {}
    for name in sorted(groups, key=lambda n: (ORDER.index(n) if n in ORDER else 99, n)):
        idx = sorted(groups[name])
        if idx != list(range(len(idx))):
            sys.exit(f"ERROR: '{name}' frames are numbered {idx}; they must run 0,1,2,... with no gaps.")
        anims[name] = [load(groups[name][i]) for i in idx]
        print(f"  {name:8s} {len(idx)} frames")

    for d in ("preview", "master", "c"):
        os.makedirs(os.path.join(out, d), exist_ok=True)

    # GIF previews (4x)
    for name, frames in anims.items():
        big = [f.resize((W * 4, H * 4), Image.NEAREST) for f in frames]
        big[0].save(f"{out}/preview/cat_{name}.gif", save_all=True, append_images=big[1:],
                    duration=DURATION.get(name, 300), loop=0)

    # transparent masters (one per frame) + 8x master of idle frame 0
    for name, frames in anims.items():
        for i, f in enumerate(frames):
            transparent(f).save(f"{out}/master/cat_{name}_{i}_transparent.png")
    first_t = transparent(anims.get("idle", next(iter(anims.values())))[0])
    first_t.resize((512, 512), Image.NEAREST).save(f"{out}/master/cat_master_512_transparent.png")

    # master (idle frame 0, 8x)
    first = anims.get("idle", next(iter(anims.values())))[0]
    first.resize((512, 512), Image.NEAREST).save(f"{out}/master/cat_master_512.png")

    # contact sheet
    S, pad, lab = 3, 10, 70
    cols = max(len(v) for v in anims.values())
    sheet = Image.new("RGB", (lab + cols * (W * S + pad) + pad, len(anims) * (H * S + pad) + pad), (245, 240, 245))
    dr = ImageDraw.Draw(sheet)
    for r, (name, frames) in enumerate(anims.items()):
        y = pad + r * (H * S + pad)
        dr.text((8, y + H * S // 2 - 5), name, fill=(60, 40, 60))
        for c, f in enumerate(frames):
            sheet.paste(f.resize((W * S, H * S), Image.NEAREST), (lab + pad + c * (W * S + pad), y))
    sheet.save(f"{out}/preview/cat_contact_sheet.png")

    # C arrays
    h = ["/**",
         " * @file    sprites.h",
         " * @brief   Cat pet sprites: 64x64 RGB565 frames (row-major, top-left first).",
         " *",
         " * Art by Grecia: hand-edited in Piskel on top of AI-generated base poses.",
         " * Generated from the frame PNGs by tools/rebuild_from_pngs.py - do not edit",
         " * the arrays by hand; edit the PNGs and re-run the script instead.",
         " *",
         " * Draw a frame with ILI9341_DrawBitmap(x, y, CAT_W, CAT_H, cat_idle_anim[i]).",
         " * Each animation has a frame table cat_<name>_anim[] and a count CAT_<NAME>_FRAMES.",
         " * Background color is CAT_BG_COLOR (black), so frames need no clearing.",
         " */",
         "#ifndef SPRITES_H", "#define SPRITES_H", "", "#include <stdint.h>", "",
         "#define CAT_W 64", "#define CAT_H 64",
         "#define CAT_BG_COLOR 0x%04X  /* solid background */" % rgb565(*BG), ""]
    c = ["/**", " * @file    sprites.c",
         " * @brief   Cat pet sprite data (see sprites.h). Generated by tools/rebuild_from_pngs.py.",
         " */", '#include "sprites.h"', "",
         "/* RGB565, row-major, top-left first. 64x64 = 8192 bytes per frame. */", ""]
    total = 0
    for name, frames in anims.items():
        n, N = len(frames), name.upper()
        h.append(f"#define CAT_{N}_FRAMES {n}")
        for i, f in enumerate(frames):
            vals = [rgb565(*p) for p in list(f.getdata())]
            total += len(vals) * 2
            h.append(f"extern const uint16_t cat_{name}_{i}[CAT_W * CAT_H];")
            c.append(f"const uint16_t cat_{name}_{i}[CAT_W * CAT_H] = {{")
            for k in range(0, len(vals), 16):
                c.append("    " + ", ".join("0x%04X" % v for v in vals[k:k + 16]) + ",")
            c.append("};\n")
        h.append(f"extern const uint16_t * const cat_{name}_anim[CAT_{N}_FRAMES];")
        c.append(f"const uint16_t * const cat_{name}_anim[CAT_{N}_FRAMES] = {{ "
                 + ", ".join(f"cat_{name}_{i}" for i in range(n)) + " };\n")
        h.append("")
    h += ["#endif /* SPRITES_H */", ""]
    open(f"{out}/c/sprites.h", "w").write("\n".join(h))
    open(f"{out}/c/sprites.c", "w").write("\n".join(c))
    print(f"Done -> {out}/   ({total // 1024} KB of sprite data)")


if __name__ == "__main__":
    import warnings; warnings.filterwarnings("ignore")
    main()
