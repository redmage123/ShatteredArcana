"""
Generate a hand-painted battle scene for the Shattered Arcana main menu.
Style: Oil painting / classical battle painting in the style of Delacroix or Altdorfer.
Aurelith heroes and wizards (left) vs Abyssal denizens (right).
Uses heavy painterly post-processing to simulate brushwork.
"""

import os
import math
import random
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

BASE = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed"
OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Content\Textures\UI\T_MenuBackground.png"

WIDTH, HEIGHT = 1920, 1080

random.seed(42)  # Reproducible

# Warm, rich oil painting palette
SKY_TOP     = (140, 100, 60)     # Smoky amber sky (battle haze)
SKY_MID     = (200, 150, 80)     # Warm golden
SKY_LOW     = (180, 120, 70)     # Dusty horizon
GROUND_TOP  = (80, 90, 50)       # Churned battlefield green
GROUND_MID  = (65, 60, 40)       # Muddy earth
GROUND_BOT  = (50, 40, 30)       # Dark foreground earth
GOLD_LIGHT  = (255, 220, 140)    # Divine light source
FIRE_RED    = (200, 60, 30)      # Abyssal fire
BLOOD_RED   = (140, 25, 20)      # Battle crimson
STEEL_BLUE  = (120, 140, 170)    # Aurelith armor glint
HOLY_WHITE  = (240, 235, 220)    # Holy glow


def load_img(path, size=None):
    try:
        img = Image.open(path).convert("RGBA")
        if size:
            img = img.resize(size, Image.LANCZOS)
        return img
    except:
        return None


def oil_paint_effect(img, radius=4):
    """Simulate oil painting by applying median filter + slight blur."""
    img = img.filter(ImageFilter.MedianFilter(size=radius * 2 + 1 if radius < 3 else 5))
    img = img.filter(ImageFilter.SMOOTH_MORE)
    return img


def add_brush_strokes(canvas):
    """Overlay random semi-transparent brush stroke lines for painted texture."""
    overlay = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    for _ in range(3000):
        x = random.randint(0, WIDTH)
        y = random.randint(0, HEIGHT)
        length = random.randint(10, 40)
        angle = random.uniform(0, math.pi)
        dx = int(length * math.cos(angle))
        dy = int(length * math.sin(angle))

        # Sample color from canvas at this point
        try:
            px = canvas.getpixel((min(x, WIDTH - 1), min(y, HEIGHT - 1)))
            r, g, b = px[0], px[1], px[2]
            # Slight color variation
            r = max(0, min(255, r + random.randint(-20, 20)))
            g = max(0, min(255, g + random.randint(-20, 20)))
            b = max(0, min(255, b + random.randint(-20, 20)))
            alpha = random.randint(30, 80)
            draw.line([(x, y), (x + dx, y + dy)], fill=(r, g, b, alpha),
                      width=random.randint(2, 5))
        except:
            pass

    return Image.alpha_composite(canvas, overlay)


def paint_gradient_sky(draw):
    """Paint a dramatic battle sky with warm smoky tones."""
    for y in range(0, HEIGHT // 2 + 100):
        progress = y / (HEIGHT // 2 + 100)

        if progress < 0.4:
            # Top: smoky amber
            t = progress / 0.4
            r = int(SKY_TOP[0] * (1 - t) + SKY_MID[0] * t)
            g = int(SKY_TOP[1] * (1 - t) + SKY_MID[1] * t)
            b = int(SKY_TOP[2] * (1 - t) + SKY_MID[2] * t)
        else:
            # Lower: golden to dusty
            t = (progress - 0.4) / 0.6
            r = int(SKY_MID[0] * (1 - t) + SKY_LOW[0] * t)
            g = int(SKY_MID[1] * (1 - t) + SKY_LOW[1] * t)
            b = int(SKY_MID[2] * (1 - t) + SKY_LOW[2] * t)

        # Add slight noise for painterly texture
        r = max(0, min(255, r + random.randint(-8, 8)))
        g = max(0, min(255, g + random.randint(-8, 8)))
        b = max(0, min(255, b + random.randint(-8, 8)))

        draw.line([(0, y), (WIDTH, y)], fill=(r, g, b))


def paint_battlefield_ground(draw):
    """Paint churned battlefield ground."""
    ground_start = HEIGHT // 2 + 50
    for y in range(ground_start, HEIGHT):
        progress = (y - ground_start) / (HEIGHT - ground_start)

        if progress < 0.4:
            t = progress / 0.4
            r = int(GROUND_TOP[0] * (1 - t) + GROUND_MID[0] * t)
            g = int(GROUND_TOP[1] * (1 - t) + GROUND_MID[1] * t)
            b = int(GROUND_TOP[2] * (1 - t) + GROUND_MID[2] * t)
        else:
            t = (progress - 0.4) / 0.6
            r = int(GROUND_MID[0] * (1 - t) + GROUND_BOT[0] * t)
            g = int(GROUND_MID[1] * (1 - t) + GROUND_BOT[1] * t)
            b = int(GROUND_MID[2] * (1 - t) + GROUND_BOT[2] * t)

        r = max(0, min(255, r + random.randint(-10, 10)))
        g = max(0, min(255, g + random.randint(-10, 10)))
        b = max(0, min(255, b + random.randint(-10, 10)))

        draw.line([(0, y), (WIDTH, y)], fill=(r, g, b))


def paint_smoke_clouds(canvas):
    """Add billowing smoke/dust clouds across the battlefield."""
    overlay = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # Battle smoke clouds
    smoke_positions = [
        (WIDTH // 2, HEIGHT // 3, 250, (180, 160, 120, 40)),
        (WIDTH // 2 - 200, HEIGHT // 3 + 50, 180, (160, 140, 100, 35)),
        (WIDTH // 2 + 200, HEIGHT // 3 - 30, 200, (170, 150, 110, 38)),
        (300, HEIGHT // 4, 150, (150, 140, 120, 25)),
        (WIDTH - 300, HEIGHT // 4, 150, (160, 120, 100, 30)),
    ]

    for cx, cy, radius, color in smoke_positions:
        for r in range(radius, 0, -4):
            alpha = int(color[3] * (r / radius))
            draw.ellipse([cx - r, cy - r, cx + r, cy + r],
                         fill=(color[0], color[1], color[2], alpha))

    overlay = overlay.filter(ImageFilter.GaussianBlur(20))
    return Image.alpha_composite(canvas, overlay)


def paint_light_rays(canvas):
    """Add dramatic divine light from upper left (Aurelith side)."""
    overlay = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # Light source: upper left (divine Aurelith light)
    cx, cy = 200, 50
    for i in range(12):
        angle = math.radians(-20 + i * 8)
        x2 = cx + int(1800 * math.cos(angle))
        y2 = cy + int(1800 * math.sin(angle))
        for w in range(20, 0, -1):
            alpha = int(12 * (w / 20))
            draw.line([(cx, cy), (x2, y2)],
                      fill=(GOLD_LIGHT[0], GOLD_LIGHT[1], GOLD_LIGHT[2], alpha), width=w)

    # Fire glow from upper right (Abyssal side)
    fcx, fcy = WIDTH - 200, 80
    for i in range(8):
        angle = math.radians(160 + i * 8)
        x2 = fcx + int(1500 * math.cos(angle))
        y2 = fcy + int(1500 * math.sin(angle))
        for w in range(15, 0, -1):
            alpha = int(10 * (w / 15))
            draw.line([(fcx, fcy), (x2, y2)],
                      fill=(FIRE_RED[0], FIRE_RED[1], FIRE_RED[2], alpha), width=w)

    overlay = overlay.filter(ImageFilter.GaussianBlur(8))
    return Image.alpha_composite(canvas, overlay)


def blend_figure(canvas, sprite, pos, size, flip=False, tint=None):
    """Blend a figure into the painting with soft edges and color integration."""
    fig = load_img(sprite, (size, size))
    if fig is None:
        return canvas

    if flip:
        fig = fig.transpose(Image.FLIP_LEFT_RIGHT)

    # Boost saturation and warmth to match oil painting look
    fig = ImageEnhance.Color(fig).enhance(1.4)
    fig = ImageEnhance.Brightness(fig).enhance(1.2)

    if tint:
        tint_overlay = Image.new("RGBA", fig.size, tint)
        fig = Image.blend(fig, tint_overlay, 0.15)

    # Soften edges for painterly integration
    alpha = fig.split()[3]
    alpha = alpha.filter(ImageFilter.GaussianBlur(2))
    fig.putalpha(alpha)

    # Apply oil paint effect to individual figure
    rgb = fig.convert("RGB")
    rgb = rgb.filter(ImageFilter.SMOOTH_MORE)
    rgb = rgb.filter(ImageFilter.MedianFilter(3))
    fig = rgb.convert("RGBA")
    fig.putalpha(alpha)

    canvas.paste(fig, pos, fig)
    return canvas


def main():
    print("=== Generating Oil Painting Battle Scene ===")

    canvas = Image.new("RGBA", (WIDTH, HEIGHT), SKY_TOP)
    draw = ImageDraw.Draw(canvas)

    # --- 1. Paint sky and ground ---
    paint_gradient_sky(draw)
    paint_battlefield_ground(draw)

    # --- 2. Add smoke and atmosphere ---
    canvas = paint_smoke_clouds(canvas)

    # --- 3. Add dramatic light ---
    canvas = paint_light_rays(canvas)

    # --- 4. Place Aurelith forces (left side) — warm steel blue/gold tint ---
    print("  Placing Aurelith forces...")

    aurelith_tint = (100, 120, 180, 255)  # Steel blue

    # Heroes (large, foreground)
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/paladin.png"),
                          (250, 300), 350, tint=aurelith_tint)
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/warlock.png"),
                          (80, 380), 300, tint=aurelith_tint)
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/cleric.png"),
                          (450, 400), 280, tint=aurelith_tint)

    # Aurelith units — medium size, mid-ground
    aurelith_races = ["crystal_elves", "celestials", "high_elves", "erlking_guard", "dwarves"]
    aurelith_units = []
    for race in aurelith_races:
        d = os.path.join(BASE, "units", race)
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if f.endswith(".png"):
                    aurelith_units.append(os.path.join(d, f))

    unit_positions_left = [
        (50, 550, 180), (180, 600, 170), (350, 580, 175),
        (100, 700, 160), (260, 720, 155), (420, 660, 165),
        (30, 800, 140), (170, 830, 135), (330, 810, 145),
    ]

    for i, (x, y, sz) in enumerate(unit_positions_left):
        if i < len(aurelith_units):
            canvas = blend_figure(canvas, aurelith_units[i], (x, y), sz, tint=aurelith_tint)

    # --- 5. Place Abyssal forces (right side) — fiery red/dark tint ---
    print("  Placing Abyssal forces...")

    abyssal_tint = (200, 80, 60, 255)  # Fiery red

    # Abyssal heroes/leaders
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/necromancer.png"),
                          (WIDTH - 580, 300), 350, flip=True, tint=abyssal_tint)
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/assassin.png"),
                          (WIDTH - 380, 380), 300, flip=True, tint=abyssal_tint)
    canvas = blend_figure(canvas, os.path.join(BASE, "heroes/psyker.png"),
                          (WIDTH - 720, 400), 280, flip=True, tint=abyssal_tint)

    # Abyssal units
    abyssal_races = ["abyssal_terrors", "demons", "demonkin", "undead", "shadow_folk", "dark_elves"]
    abyssal_units = []
    for race in abyssal_races:
        d = os.path.join(BASE, "units", race)
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if f.endswith(".png"):
                    abyssal_units.append(os.path.join(d, f))

    unit_positions_right = [
        (WIDTH - 230, 550, 180), (WIDTH - 360, 600, 170), (WIDTH - 530, 580, 175),
        (WIDTH - 280, 700, 160), (WIDTH - 440, 720, 155), (WIDTH - 600, 660, 165),
        (WIDTH - 210, 800, 140), (WIDTH - 350, 830, 135), (WIDTH - 510, 810, 145),
    ]

    for i, (x, y, sz) in enumerate(unit_positions_right):
        if i < len(abyssal_units):
            canvas = blend_figure(canvas, abyssal_units[i], (x, y), sz,
                                  flip=True, tint=abyssal_tint)

    # --- 6. Central clash effects — spell explosions ---
    print("  Adding clash effects...")

    center_glow = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(center_glow)
    cx, cy = WIDTH // 2, HEIGHT // 2 + 80

    # Golden/white explosion at clash point
    for r in range(300, 0, -3):
        p = r / 300
        alpha = int(50 * p)
        cr = int(255 * (1 - p) + 200 * p)
        cg = int(220 * (1 - p) + 140 * p)
        cb = int(150 * (1 - p) + 60 * p)
        gdraw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(cr, cg, cb, alpha))

    center_glow = center_glow.filter(ImageFilter.GaussianBlur(15))
    canvas = Image.alpha_composite(canvas, center_glow)

    # Spell effects at clash point
    clash_spells = [
        "spells/chaos/armageddon.png",
        "spells/life/divine_order.png",
        "spells/sorcery/counter_magic.png",
    ]
    spell_positions = [
        (WIDTH // 2 - 130, HEIGHT // 2 - 60, 260),
        (WIDTH // 2 - 200, HEIGHT // 2 + 80, 220),
        (WIDTH // 2 + 30, HEIGHT // 2 + 40, 200),
    ]

    for sp_file, (sx, sy, ss) in zip(clash_spells, spell_positions):
        sp_path = os.path.join(BASE, sp_file)
        if os.path.exists(sp_path):
            sp = load_img(sp_path, (ss, ss))
            if sp:
                sp = ImageEnhance.Color(sp).enhance(1.6)
                sp = ImageEnhance.Brightness(sp).enhance(1.4)
                alpha = sp.split()[3]
                alpha = ImageEnhance.Brightness(alpha).enhance(0.5)
                sp.putalpha(alpha)
                sp_rgb = sp.convert("RGB").filter(ImageFilter.SMOOTH_MORE)
                sp = sp_rgb.convert("RGBA")
                sp.putalpha(alpha)
                canvas.paste(sp, (sx, sy), sp)

    # --- 7. Heavy oil painting post-processing ---
    print("  Applying oil painting effect...")

    # Convert to RGB for processing
    canvas_rgb = canvas.convert("RGB")

    # Oil paint simulation: median filter + detail enhancement
    painted = canvas_rgb.filter(ImageFilter.MedianFilter(5))
    painted = painted.filter(ImageFilter.SMOOTH)

    # Edge enhancement for painterly detail
    edges = canvas_rgb.filter(ImageFilter.FIND_EDGES)
    edges = ImageEnhance.Brightness(edges).enhance(0.15)
    painted = Image.blend(painted, edges, 0.08)

    # Boost warmth and saturation for oil painting richness
    painted = ImageEnhance.Color(painted).enhance(1.35)
    painted = ImageEnhance.Contrast(painted).enhance(1.15)
    painted = ImageEnhance.Brightness(painted).enhance(1.1)

    canvas = painted.convert("RGBA")

    # --- 8. Add brush stroke texture ---
    print("  Adding brush strokes...")
    canvas = add_brush_strokes(canvas)

    # --- 9. Canvas texture overlay (linen weave) ---
    texture = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    tdraw = ImageDraw.Draw(texture)
    for x in range(0, WIDTH, 3):
        for y in range(0, HEIGHT, 3):
            if (x + y) % 6 < 2:
                tdraw.point((x, y), fill=(0, 0, 0, 8))
            elif (x + y) % 6 == 3:
                tdraw.point((x, y), fill=(255, 240, 200, 6))
    canvas = Image.alpha_composite(canvas, texture)

    # --- 10. Subtle vignette (painting frame darkness) ---
    vignette = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    vdraw = ImageDraw.Draw(vignette)
    vcx, vcy = WIDTH // 2, HEIGHT // 2
    max_dist = (vcx ** 2 + vcy ** 2) ** 0.5
    for r in range(int(max_dist), int(max_dist * 0.55), -4):
        progress = (r - max_dist * 0.55) / (max_dist * 0.45)
        a = int(70 * progress)
        vdraw.ellipse([vcx - r, vcy - r, vcx + r, vcy + r], fill=(30, 20, 10, a))
    canvas = Image.alpha_composite(canvas, vignette)

    # --- 11. Final adjustments ---
    canvas = canvas.convert("RGB")
    canvas = ImageEnhance.Brightness(canvas).enhance(1.05)
    canvas = ImageEnhance.Color(canvas).enhance(1.1)

    # --- Save ---
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    canvas.save(OUTPUT, "PNG", optimize=True)
    print(f"  Saved: {OUTPUT} ({canvas.size})")
    print("=== Done ===")


if __name__ == "__main__":
    main()
