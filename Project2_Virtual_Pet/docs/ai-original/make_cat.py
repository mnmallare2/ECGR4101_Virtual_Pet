"""Cat virtual-pet sprite generator (64x64 pixel art, RGB565 export).
Outputs into cat_out/:
  frames/   64x64 PNGs on the solid background (feed these to the starter converter)
  master/   transparent masters + 512px master (for the docs folder)
  preview/  4x animated GIFs + contact sheet
  c/        RGB565 const uint16_t arrays (backup / reference)
"""
from PIL import Image, ImageDraw
import math, os

W = H = 64
BG = (255, 255, 255)
CX = 32.5
OUT = "cat_out"

OUTLINE = (59, 38, 30, 255)
FUR     = (245, 166, 91, 255)
STRIPE  = (214, 120, 52, 255)
CREAM   = (255, 241, 222, 255)
EAR_IN  = (247, 168, 196, 255)
CHEEK   = (244, 143, 177, 255)
EYE     = (43, 27, 27, 255)
HILITE  = (255, 255, 255, 255)
NOSE    = (229, 115, 154, 255)
WHISK   = (110, 80, 70, 255)
TEAR    = (100, 175, 240, 255)
ZCOL    = (106, 123, 181, 255)
HEART   = (239, 83, 120, 255)
GOLD    = (255, 202, 40, 255)
DUST    = (214, 204, 194, 255)
YARN    = (186, 104, 200, 255)
YARN_D  = (123, 60, 140, 255)
BOWL_F  = (229, 115, 115, 255)
BOWL_W  = (100, 181, 246, 255)
KIBBLE  = (141, 90, 59, 255)
KIBBLE2 = (186, 128, 80, 255)
WATER   = (170, 220, 250, 255)
CASE    = (141, 110, 99, 255)
CASE_D  = (93, 64, 55, 255)
CASE_S  = (215, 190, 160, 255)


# ---------- helpers ----------
def rot(pts, pivot, ang):
    if not ang: return pts
    px_, py_ = pivot; a = math.radians(ang); c, s = math.cos(a), math.sin(a)
    return [(px_ + (x - px_) * c - (y - py_) * s, py_ + (x - px_) * s + (y - py_) * c) for x, y in pts]

def ell_pts(cx, cy, rx, ry, n=48):
    return [(cx + rx * math.cos(2 * math.pi * i / n), cy + ry * math.sin(2 * math.pi * i / n)) for i in range(n)]

def mask_poly(pts):
    m = Image.new("L", (W, H), 0); ImageDraw.Draw(m).polygon(pts, fill=255); return m

def mask_ell(cx, cy, rx, ry):
    return mask_poly(ell_pts(cx, cy, rx, ry))

def stamp(img, mask, fill, outline=OUTLINE):
    mp, ip = mask.load(), img.load()
    for y in range(H):
        for x in range(W):
            if mp[x, y]: continue
            for ax, ay in ((x+1, y), (x-1, y), (x, y+1), (x, y-1)):
                if 0 <= ax < W and 0 <= ay < H and mp[ax, ay]:
                    ip[x, y] = outline; break
    for y in range(H):
        for x in range(W):
            if mp[x, y]: ip[x, y] = fill

def paint(img, mask, col):
    mp, ip = mask.load(), img.load()
    for y in range(H):
        for x in range(W):
            if mp[x, y]: ip[x, y] = col

def px(img, pts, col):
    p = img.load()
    for x, y in pts:
        x, y = int(x), int(y)
        if 0 <= x < W and 0 <= y < H: p[x, y] = col

def rect(img, x0, y0, x1, y1, col):
    px(img, [(x, y) for x in range(int(x0), int(x1) + 1) for y in range(int(y0), int(y1) + 1)], col)

def line(img, a, b, col):
    ImageDraw.Draw(img).line([a, b], fill=col, width=1)

def side_stripes(img, mask, rows):
    mp = mask.load()
    for y in rows:
        if not 0 <= y < H: continue
        xs = [x for x in range(W) if mp[x, y]]
        if len(xs) < 10: continue
        rect(img, xs[0], y, xs[0] + 2, y, STRIPE); rect(img, xs[-1] - 2, y, xs[-1], y, STRIPE)

def tail(img, base, ctrl, tip, outline=True):
    m = Image.new("L", (W, H), 0); d = ImageDraw.Draw(m)
    pts = []
    for i in range(25):
        t = i / 24
        x = (1-t)**2*base[0] + 2*(1-t)*t*ctrl[0] + t*t*tip[0]
        y = (1-t)**2*base[1] + 2*(1-t)*t*ctrl[1] + t*t*tip[1]
        r = 2.7 - 0.5 * t
        d.ellipse([x - r, y - r, x + r, y + r], fill=255); pts.append((x, y))
    stamp(img, m, FUR)
    mp = m.load()
    for y in range(H):                      # striped tip
        for x in range(W):
            if mp[x, y] and math.dist((x, y), tip) < 3.2: px(img, [(x, y)], STRIPE)
    mid = pts[12]
    for y in range(H):
        for x in range(W):
            if mp[x, y] and abs(math.dist((x, y), mid) - 0) < 1.2: px(img, [(x, y)], STRIPE)


# ---------- face ----------
def face(img, ix, hy, eyes="open", mouth="smile", cheeks=True, whisk_droop=0, tears=0):
    L, R = 26 + ix, 37 + ix                       # left x of each 3px eye
    for ex in (L, R):
        if eyes in ("open", "sad"):
            rect(img, ex, hy - 1, ex + 2, hy + 1, EYE); px(img, [(ex, hy - 1)], HILITE)
        elif eyes == "closed":
            px(img, [(ex-1, hy), (ex, hy+1), (ex+1, hy+1), (ex+2, hy+1), (ex+3, hy)], EYE)
        elif eyes == "happy":
            px(img, [(ex-1, hy+1), (ex, hy), (ex+1, hy), (ex+2, hy), (ex+3, hy+1)], EYE)
        elif eyes == "blink":
            rect(img, ex - 1, hy + 1, ex + 3, hy + 1, EYE)
        elif eyes == "dizzy":
            rect(img, ex - 1, hy, ex + 3, hy, EYE); px(img, [(ex - 1, hy - 1), (ex + 3, hy + 1)], EYE)
        elif eyes == "x":
            line(img, (ex - 1, hy - 2), (ex + 3, hy + 2), EYE); line(img, (ex + 3, hy - 2), (ex - 1, hy + 2), EYE)
    if eyes == "sad":
        px(img, [(L - 1, hy - 3), (L, hy - 4), (L + 1, hy - 4), (R + 1, hy - 4), (R + 2, hy - 4), (R + 3, hy - 3)], EYE)
    if tears:
        for ex in (L, R + 2):
            px(img, [(ex, hy + 2 + k) for k in range(tears)] + [(ex - 1 if ex == L else ex + 1, hy + 2 + tears)], TEAR)

    # nose + mouth (":3")
    px(img, [(32 + ix, hy + 4), (33 + ix, hy + 4)], NOSE)
    a, b = hy + 6, hy + 7
    if mouth == "smile":
        px(img, [(30+ix, a), (31+ix, b), (32+ix, a), (33+ix, a), (34+ix, b), (35+ix, a)], EYE)
    elif mouth == "open":
        rect(img, 31 + ix, hy + 6, 34 + ix, hy + 8, EYE); px(img, [(32 + ix, hy + 8), (33 + ix, hy + 8)], NOSE)
    elif mouth == "chew":
        px(img, [(31+ix, a), (32+ix, b), (33+ix, b), (34+ix, a)], EYE)
    elif mouth == "frown":
        px(img, [(30+ix, b), (31+ix, a), (32+ix, a), (33+ix, a), (34+ix, a), (35+ix, b)], EYE)
    elif mouth == "flat":
        rect(img, 31 + ix, b, 34 + ix, b, EYE)
    elif mouth == "lick":
        px(img, [(31+ix, a), (32+ix, a), (33+ix, a), (34+ix, a)], EYE); px(img, [(32+ix, b), (33+ix, b)], NOSE)

    if cheeks:
        rect(img, 23 + ix, hy + 3, 24 + ix, hy + 3, CHEEK); rect(img, 41 + ix, hy + 3, 42 + ix, hy + 3, CHEEK)

    # whiskers (droop>0 bends them down)
    for k, dyw in enumerate((-1, 1)):
        y0 = hy + 4 + k
        line(img, (22 + ix, y0), (14 + ix, y0 + dyw + whisk_droop * (k + 1)), WHISK)
        line(img, (43 + ix, y0), (51 + ix, y0 + dyw + whisk_droop * (k + 1)), WHISK)


# ---------- the cat (sitting, front or back) ----------
def cat(dx=0, dy=0, hdx=0, hdy=0, bw=0, bh=0, ear_droop=0, eyes="open", mouth="smile", cheeks=True,
        whisk_droop=0, tears=0, back=False, tail_side=1, swish=0, paw=None):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    cx = CX + dx
    bcy, brx, bry = 49 + dy - bh / 2, 12 + bw, 11 + bh
    hx, hy = cx + hdx, 28 + dy + hdy

    if not back:   # tail behind the body
        s = tail_side
        tail(img, (cx + s * 9, bcy + 7), (cx + s * 22, bcy + 7), (cx + s * (18 + swish), bcy - 9))

    # ears
    for s, ang in ((-1, -ear_droop), (1, ear_droop)):
        A, B, T = (hx + s * 12.5, hy - 3), (hx + s * 2.5, hy - 9.5), (hx + s * 11.5, hy - 18)
        piv = ((A[0] + B[0]) / 2, (A[1] + B[1]) / 2)
        tri = rot([A, B, T], piv, ang)
        stamp(img, mask_poly(tri), FUR)
        if not back:
            c = (sum(p[0] for p in tri) / 3, sum(p[1] for p in tri) / 3)
            inner = [(c[0] + (p[0] - c[0]) * 0.5, c[1] + (p[1] - c[1]) * 0.5 - 0.5) for p in tri]
            paint(img, mask_poly(inner), EAR_IN)

    # body
    body = mask_ell(cx, bcy, brx, bry)
    stamp(img, body, FUR)
    side_stripes(img, body, [int(bcy - 3), int(bcy + 1), int(bcy + 5)])
    if not back:
        paint(img, mask_ell(cx, bcy + 2, brx * 0.5, bry * 0.65), CREAM)
        for s in (-1, 1):
            if paw and s == 1: continue
            stamp(img, mask_ell(cx + s * 4.5, min(59 + dy, 60.5), 3.5, 2.5), CREAM)
        if paw:
            stamp(img, mask_ell(paw[0], paw[1], 3, 2.5), CREAM)
    else:
        for s in (-1, 1):
            stamp(img, mask_ell(cx + s * 5.5, min(59.5 + dy, 60.5), 3.5, 2.2), FUR)

    # head
    stamp(img, mask_ell(hx, hy, 13, 10.5), FUR)
    ix, ihy = int(round(hx - CX)), int(round(hy))
    for xx, ln in ((29, 2), (32, 3), (33, 3), (36, 2)):          # tabby forehead
        rect(img, xx + ix, ihy - 10, xx + ix, ihy - 10 + ln, STRIPE)
    if back:
        tail(img, (cx, bcy + 8), (cx + 9, bcy + 3), (cx + 4 + swish, bcy - 9))
    else:
        paint(img, mask_ell(hx, hy + 5.5, 5.5, 3.2), CREAM)
        face(img, ix, ihy, eyes, mouth, cheeks, whisk_droop, tears)
    return img


# ---------- the cat lying down (sleep) ----------
def cat_lying(breath=0, dy=0, eyes="closed", mouth="flat", ear_ang=12, whisk=1, dead=False):
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    bcy = 53 + dy - breath / 2
    body = mask_ell(37, bcy, 20, 7 + breath)
    stamp(img, body, FUR)
    mp = body.load()
    for xx in (34, 40, 46):                                   # back stripes
        ys = [y for y in range(H) if mp[xx, y]]
        if ys: rect(img, xx, ys[0], xx, ys[0] + 2, STRIPE)
    if dead: tail(img, (54, bcy + 2), (59, bcy + 5), (62, 60))      # limp tail
    else: tail(img, (54, bcy + 3), (58, 62), (36, 60.5))       # tail wrapped in front
    for x in (12, 19):
        stamp(img, mask_ell(x, 59.5, 3.5, 2), CREAM)          # front paws
    hx, hy = 19.5, 46 + dy
    for s in (-1, 1):
        A, B, T = (hx + s * 12.5, hy - 3), (hx + s * 2.5, hy - 9.5), (hx + s * 11.5, hy - 17)
        piv = ((A[0] + B[0]) / 2, (A[1] + B[1]) / 2)
        tri = rot([A, B, T], piv, s * ear_ang)
        stamp(img, mask_poly(tri), FUR)
        c = (sum(p[0] for p in tri) / 3, sum(p[1] for p in tri) / 3)
        paint(img, mask_poly([(c[0] + (p[0]-c[0]) * .5, c[1] + (p[1]-c[1]) * .5 - .5) for p in tri]), EAR_IN)
    stamp(img, mask_ell(hx, hy, 13, 10.5), FUR)
    ix = int(round(hx - CX))
    for xx, ln in ((29, 2), (32, 3), (33, 3), (36, 2)):
        rect(img, xx + ix, hy - 10, xx + ix, hy - 10 + ln, STRIPE)
    paint(img, mask_ell(hx, hy + 5.5, 5.5, 3.2), CREAM)
    face(img, ix, hy, eyes=eyes, mouth=mouth, cheeks=not dead, whisk_droop=whisk)
    return img


# ---------- props ----------
FONT = {
    "M": ["10001", "11011", "10101", "10001", "10001"],
    "E": ["111", "100", "110", "100", "111"],
    "O": ["111", "101", "101", "101", "111"],
    "W": ["10001", "10001", "10101", "11011", "10001"],
    "P": ["111", "101", "111", "100", "100"],
    "R": ["110", "101", "110", "101", "101"],
}
def text(img, s, x, y, col):
    for ch in s:
        g = FONT[ch]
        px(img, [(x + c, y + r) for r, row in enumerate(g) for c, v in enumerate(row) if v == "1"], col)
        x += len(g[0]) + 1

HEART_PAT = ["01010", "11111", "11111", "01110", "00100"]
def heart(img, x, y):
    px(img, [(x + c, y + r) for r, row in enumerate(HEART_PAT) for c, v in enumerate(row) if v == "1"], HEART)

def sparkle(img, x, y, big=False):
    pts = [(x, y), (x-1, y), (x+1, y), (x, y-1), (x, y+1)]
    if big: pts += [(x-2, y), (x+2, y), (x, y-2), (x, y+2)]
    px(img, pts, GOLD)

Z_SMALL = ["111", "001", "010", "100", "111"]
Z_BIG = ["1111", "0001", "0010", "0100", "1000", "1111"]
def zee(img, x, y, big=False):
    pat = Z_BIG if big else Z_SMALL
    px(img, [(x + c, y + r) for r, row in enumerate(pat) for c, v in enumerate(row) if v == "1"], ZCOL)

def purr_marks(img, phase):
    o = 1 if phase else 0
    for x0, s in ((9 - o, -1), (56 + o, 1)):
        px(img, [(x0, 25), (x0 + s, 26), (x0 + s, 27), (x0, 28)], ZCOL)
        px(img, [(x0 + 3 * s, 24), (x0 + 4 * s, 25), (x0 + 4 * s, 28), (x0 + 3 * s, 29), (x0 + 4 * s, 26), (x0 + 4 * s, 27)], ZCOL)

def puff(img, x, y, r=3):
    stamp(img, mask_ell(x, y, r, r * 0.8), DUST, (170, 160, 150, 255))

def bowl(img, cx, kind, level=1.0):
    top = 55
    shape = [(cx - 7, top), (cx + 7, top), (cx + 5, 61), (cx - 5, 61)]
    stamp(img, mask_poly(shape), BOWL_F if kind == "food" else BOWL_W)
    rect(img, cx - 6, top + 2, cx + 6, top + 2, (255, 255, 255, 255))     # rim stripe
    if kind == "food":
        n = int(9 * level)
        spots = [(-4, -1), (-2, -2), (0, -1), (2, -2), (4, -1), (-3, -3), (1, -3), (-1, -2), (3, -3)]
        for i, (ox, oy) in enumerate(spots[:n]):
            px(img, [(cx + ox, top + oy), (cx + ox + 1, top + oy)], KIBBLE if i % 2 else KIBBLE2)
    else:
        if level > 0:
            rect(img, cx - 6, top, cx + 6, top, WATER)
            if level > 0.6: rect(img, cx - 5, top - 1, cx + 5, top - 1, WATER)

def yarn(img, x, y, r=5, spin=0, string_to=None):
    if string_to:
        line(img, (x - r + 1, y + 2), string_to, YARN_D)
    stamp(img, mask_ell(x, y, r, r), YARN, YARN_D)
    d = ImageDraw.Draw(img)
    for k in range(2):
        a = spin + k * 70
        d.arc([x - r + 1, y - r + 2 + k, x + r - 1, y + r - 1 + k], a, a + 140, fill=YARN_D)
    px(img, [(x - 2, y - 2)], (225, 170, 235, 255))

def suitcase(img, x, y):
    """x,y = top-left of a 14x10 case."""
    stamp(img, mask_poly([(x, y), (x + 13, y), (x + 13, y + 9), (x, y + 9)]), CASE, CASE_D)
    rect(img, x + 4, y - 3, x + 9, y - 3, CASE_D); px(img, [(x + 4, y - 2), (x + 4, y - 1), (x + 9, y - 2), (x + 9, y - 1)], CASE_D)
    rect(img, x + 3, y + 1, x + 3, y + 8, CASE_S); rect(img, x + 10, y + 1, x + 10, y + 8, CASE_S)
    heart(img, x + 5, y + 3)

def halo(img, x, y):
    ImageDraw.Draw(img).ellipse([x - 6, y - 2, x + 6, y + 2], outline=GOLD)

def tilt(sprite, angle, ground=61):
    r = sprite.rotate(angle, resample=Image.NEAREST, expand=True)
    r = r.crop(r.getbbox())
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    out.alpha_composite(r, (max((W - r.width) // 2, 0), max(ground - r.height, 0)))
    return out

def ghost(scale=0.45, alpha=190):
    """Angel version of the cat: wings, pale tint, see-through."""
    g = cat(eyes="closed", mouth="smile", tail_side=-1, swish=2)
    for s in (-1, 1):                                    # little wings
        wing = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        stamp(wing, mask_poly(rot(ell_pts(CX + s * 18, 40, 8, 5), (CX + s * 11, 44), s * -30)), (255, 255, 255, 255))
        wing.alpha_composite(g); g = wing
    size = int(W * scale)
    g = g.resize((size, size), Image.NEAREST)
    p = g.load()
    for y in range(size):
        for x in range(size):
            r, gg, b, a = p[x, y]
            if a:
                p[x, y] = (int(r * .55 + 215 * .45), int(gg * .55 + 235 * .45), int(b * .55 + 255 * .45), alpha)
    return g


# ---------- animations ----------
def anim_idle():
    a = cat()
    b = cat(eyes="blink", swish=4)                                           # blink + tail swish
    c = cat(eyes="happy", mouth="open", hdy=-1, swish=2); text(c, "MEOW", 42, 1, HEART)   # meow
    d = cat(eyes="happy", mouth="open", hdy=-1, swish=0); text(d, "MEOW", 42, 2, HEART)
    e = cat(eyes="closed", mouth="smile", swish=2); text(e, "PRR", 46, 1, ZCOL); purr_marks(e, 0)   # purr
    f = cat(eyes="closed", mouth="smile", dy=1, bh=-1, swish=3); text(f, "PRR", 46, 2, ZCOL); purr_marks(f, 1)
    return [a, b, c, d, e, f]

def anim_feed():
    a = cat(swish=2); bowl(a, 8, "food", 1); bowl(a, 56, "water", 1)
    b = cat(dx=-3, dy=1, hdx=-7, hdy=12, eyes="closed", mouth="chew", swish=4)    # eating
    bowl(b, 12, "food", 0.55); px(b, [(10, 50), (15, 48)], KIBBLE2)
    c = cat(dx=3, dy=1, hdx=7, hdy=12, eyes="closed", mouth="lick", tail_side=-1)  # drinking
    bowl(c, 53, "water", 0.5); bowl(c, 8, "food", 0.55)
    px(c, [(48, 52), (49, 51), (57, 52), (58, 51)], WATER)
    d = cat(eyes="happy", cheeks=True, swish=3); bowl(d, 8, "food", 0.3); bowl(d, 56, "water", 0.5)
    heart(d, 4, 10); heart(d, 54, 8)
    return [a, b, c, d]

def anim_play():
    a = cat(dy=3, bw=2, bh=-2, tail_side=-1, swish=5); yarn(a, 55, 56, string_to=(46, 61))
    b = cat(dy=-4, eyes="happy", mouth="open", tail_side=-1, paw=(46, 42)); yarn(b, 55, 56, spin=40, string_to=(47, 61))
    sparkle(b, 8, 22, True)
    c = cat(dx=3, dy=1, tail_side=-1, swish=3, paw=(47, 50), mouth="open"); yarn(c, 51, 56, spin=90, string_to=(40, 61))
    d = cat(eyes="happy", tail_side=-1, swish=5); yarn(d, 58, 57, spin=160, string_to=(38, 61))
    sparkle(d, 50, 40, True); sparkle(d, 8, 30); heart(d, 46, 6)
    return [a, b, c, d]

def anim_sleep():
    a = cat_lying(0); zee(a, 44, 26)
    b = cat_lying(1); zee(b, 46, 21); zee(b, 52, 12, True)
    c = cat_lying(0); zee(c, 48, 15, True); zee(c, 44, 26)
    d = cat_lying(1); zee(d, 53, 6, True); zee(d, 47, 20)
    return [a, b, c, d]

def anim_death():
    droop = dict(ear_droop=55, whisk_droop=2, cheeks=False)
    a = cat(eyes="dizzy", mouth="frown", dy=1, **droop)
    b = cat(eyes="x", mouth="flat", dy=3, bw=1, bh=-2, hdy=1, **droop)
    fallen = cat_lying(dy=2, eyes="x", mouth="flat", ear_ang=45, whisk=3, dead=True)
    c = fallen.copy()
    g = ghost()
    d = fallen.copy(); d.alpha_composite(g, ((W - g.width) // 2 + 4, 10)); halo(d, 36, 11)   # rising out
    e = fallen.copy(); e.alpha_composite(g, ((W - g.width) // 2 + 4, 3)); halo(e, 36, 4)     # floating above
    return [a, b, c, d, e]

def anim_runaway():
    sad = dict(ear_droop=40, whisk_droop=2, eyes="sad", mouth="frown", cheeks=False)
    a = cat(tears=3, **sad)
    b = cat(dx=-6, tears=4, tail_side=-1, **sad); suitcase(b, 46, 51)
    c = cat(back=True, dx=4, ear_droop=25, swish=-2); suitcase(c, 47, 51)
    px(c, [(18, 30), (17, 32), (47, 29), (48, 31)], TEAR)
    d = cat(back=True, dx=26, dy=-2, ear_droop=25); suitcase(d, 54, 49)
    puff(d, 22, 59); puff(d, 12, 57, 2)
    return [a, b, c, d]

ANIMS = [("idle", anim_idle), ("feed", anim_feed), ("play", anim_play),
         ("sleep", anim_sleep), ("death", anim_death), ("runaway", anim_runaway)]
DURATION = {"idle": 300, "feed": 350, "play": 220, "sleep": 500, "death": 450, "runaway": 400}


# ---------- export ----------
def flat(img):
    out = Image.new("RGB", (W, H), BG); out.paste(img, mask=img.split()[3]); return out

def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def main():
    for d in ("frames", "master", "preview", "c"):
        os.makedirs(f"{OUT}/{d}", exist_ok=True)
    allf = {}
    for name, fn in ANIMS:
        frames = fn(); allf[name] = frames
        gif = []
        for i, f in enumerate(frames):
            flat(f).save(f"{OUT}/frames/cat_{name}_{i}.png")
            f.save(f"{OUT}/master/cat_{name}_{i}_transparent.png")
            gif.append(flat(f).resize((W * 4, H * 4), Image.NEAREST))
        gif[0].save(f"{OUT}/preview/cat_{name}.gif", save_all=True, append_images=gif[1:],
                    duration=DURATION[name], loop=0)
    allf["idle"][0].resize((512, 512), Image.NEAREST).save(f"{OUT}/master/cat_master_512.png")

    S, pad, lab = 3, 10, 70
    cols = max(len(v) for v in allf.values())
    sheet = Image.new("RGB", (lab + cols * (W * S + pad) + pad, len(ANIMS) * (H * S + pad) + pad), (245, 240, 245))
    dr = ImageDraw.Draw(sheet)
    for r, (name, _) in enumerate(ANIMS):
        y = pad + r * (H * S + pad)
        dr.text((8, y + H * S // 2 - 5), name, fill=(60, 40, 60))
        for c, f in enumerate(allf[name]):
            sheet.paste(flat(f).resize((W * S, H * S), Image.NEAREST), (lab + pad + c * (W * S + pad), y))
    sheet.save(f"{OUT}/preview/cat_contact_sheet.png")

    h = ["#ifndef CAT_SPRITES_H", "#define CAT_SPRITES_H", "", "#include <stdint.h>", "",
         "#define CAT_W 64", "#define CAT_H 64", "#define CAT_BG_COLOR 0x%04X  /* solid background */" % rgb565(*BG), ""]
    c = ['#include "cat_sprites.h"', "", "/* RGB565, row-major, top-left first. 64x64 = 8192 bytes per frame. */", ""]
    for name, frames in allf.items():
        n = len(frames)
        h.append(f"#define CAT_{name.upper()}_FRAMES {n}")
        for i, f in enumerate(frames):
            vals = [rgb565(*p) for p in list(flat(f).getdata())]
            h.append(f"extern const uint16_t cat_{name}_{i}[CAT_W * CAT_H];")
            c.append(f"const uint16_t cat_{name}_{i}[CAT_W * CAT_H] = {{")
            for k in range(0, len(vals), 16):
                c.append("    " + ", ".join("0x%04X" % v for v in vals[k:k + 16]) + ",")
            c.append("};\n")
        h.append(f"extern const uint16_t * const cat_{name}_anim[CAT_{name.upper()}_FRAMES];")
        c.append(f"const uint16_t * const cat_{name}_anim[CAT_{name.upper()}_FRAMES] = {{ "
                 + ", ".join(f"cat_{name}_{i}" for i in range(n)) + " };\n")
        h.append("")
    h += ["#endif /* CAT_SPRITES_H */", ""]
    open(f"{OUT}/c/cat_sprites.h", "w").write("\n".join(h))
    open(f"{OUT}/c/cat_sprites.c", "w").write("\n".join(c))

if __name__ == "__main__":
    main()
