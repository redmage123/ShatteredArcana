#!/usr/bin/env python3
"""
Generate UI sprite assets for Shattered Arcana.
Dark fantasy theme with gold accents.
All assets are PNG with alpha transparency.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFilter, ImageFont

# === Color Palette ===
DARK_NAVY = (10, 10, 26)
DEEP_PURPLE = (15, 52, 96)
GOLD = (218, 165, 32)
RED_ACCENT = (233, 69, 96)
DARK_BG = (22, 33, 62)
LIGHT_BG = (30, 45, 80)
MID_BG = (18, 25, 50)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
GRAY = (100, 100, 120)
DARK_GRAY = (50, 50, 65)
MANA_BLUE = (50, 120, 220)
MANA_DARK = (20, 60, 140)
XP_GOLD_LIGHT = (255, 215, 80)
GREEN = (40, 180, 60)
GREEN_DARK = (20, 100, 30)
YELLOW = (220, 200, 40)
YELLOW_DARK = (140, 120, 20)
HEALTH_RED = (200, 30, 30)
HEALTH_RED_LIGHT = (240, 80, 60)

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "processed", "ui")
os.makedirs(OUTPUT_DIR, exist_ok=True)

file_count = 0


def save(img, name):
    global file_count
    path = os.path.join(OUTPUT_DIR, name)
    img.save(path, "PNG")
    file_count += 1


def rgba(color, alpha=255):
    return color + (alpha,)


def gradient_v(draw, x0, y0, x1, y1, color_top, color_bot):
    """Vertical gradient fill."""
    h = y1 - y0
    if h <= 0:
        return
    for y in range(y0, y1):
        t = (y - y0) / max(h - 1, 1)
        r = int(color_top[0] + (color_bot[0] - color_top[0]) * t)
        g = int(color_top[1] + (color_bot[1] - color_top[1]) * t)
        b = int(color_top[2] + (color_bot[2] - color_top[2]) * t)
        a_top = color_top[3] if len(color_top) > 3 else 255
        a_bot = color_bot[3] if len(color_bot) > 3 else 255
        a = int(a_top + (a_bot - a_top) * t)
        draw.line([(x0, y), (x1, y)], fill=(r, g, b, a))


def gradient_h(draw, x0, y0, x1, y1, color_left, color_right):
    """Horizontal gradient fill."""
    w = x1 - x0
    if w <= 0:
        return
    for x in range(x0, x1):
        t = (x - x0) / max(w - 1, 1)
        r = int(color_left[0] + (color_right[0] - color_left[0]) * t)
        g = int(color_left[1] + (color_right[1] - color_left[1]) * t)
        b = int(color_left[2] + (color_right[2] - color_left[2]) * t)
        a_l = color_left[3] if len(color_left) > 3 else 255
        a_r = color_right[3] if len(color_right) > 3 else 255
        a = int(a_l + (a_r - a_l) * t)
        draw.line([(x, y0), (x, y1)], fill=(r, g, b, a))


def rounded_rect(draw, bbox, radius, fill=None, outline=None, width=1):
    """Draw a rounded rectangle with given radius."""
    x0, y0, x1, y1 = bbox
    if fill:
        draw.rounded_rectangle(bbox, radius=radius, fill=fill)
    if outline:
        draw.rounded_rectangle(bbox, radius=radius, outline=outline, width=width)


def make_panel(w, h, border_color=GOLD, bg_top=DARK_NAVY, bg_bot=DARK_BG,
               border_w=2, alpha=220, radius=8):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Background gradient
    gradient_v(draw, 0, 0, w, h, rgba(bg_top, alpha), rgba(bg_bot, alpha))
    # Round off corners with mask
    mask = Image.new("L", (w, h), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([0, 0, w - 1, h - 1], radius=radius, fill=255)
    img.putalpha(mask)
    # Border
    draw2 = ImageDraw.Draw(img)
    draw2.rounded_rectangle([0, 0, w - 1, h - 1], radius=radius,
                            outline=rgba(border_color, 200), width=border_w)
    # Subtle inner highlight at top
    for i in range(3):
        a = 30 - i * 10
        draw2.line([(radius, border_w + i), (w - radius, border_w + i)],
                   fill=rgba(WHITE, max(a, 0)))
    return img


def make_button(w, h, bg_top, bg_bot, border_color, border_w=1, radius=6,
                pressed=False, disabled=False):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    if pressed:
        bg_top, bg_bot = bg_bot, bg_top
    gradient_v(draw, 0, 0, w, h, rgba(bg_top, 255), rgba(bg_bot, 255))
    mask = Image.new("L", (w, h), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([0, 0, w - 1, h - 1], radius=radius, fill=255)
    img.putalpha(mask)
    draw2 = ImageDraw.Draw(img)
    draw2.rounded_rectangle([0, 0, w - 1, h - 1], radius=radius,
                            outline=rgba(border_color, 200 if not disabled else 100),
                            width=border_w)
    if not pressed and not disabled:
        # Top highlight
        for i in range(2):
            a = 40 - i * 20
            draw2.line([(radius, border_w + i), (w - radius, border_w + i)],
                       fill=rgba(WHITE, max(a, 0)))
    if pressed:
        # Inner shadow at top
        for i in range(2):
            draw2.line([(radius, border_w + i), (w - radius, border_w + i)],
                       fill=rgba(BLACK, 60 - i * 20))
    if disabled:
        # Overlay gray
        overlay = Image.new("RGBA", (w, h), (80, 80, 90, 80))
        img = Image.alpha_composite(img, overlay)
        # Re-apply mask
        final = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        final.paste(img, mask=mask)
        img = final
    return img


def make_bar_bg(w, h):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    rounded_rect(draw, [0, 0, w - 1, h - 1], radius=4,
                 fill=rgba(DARK_NAVY, 200), outline=rgba(GRAY, 150), width=1)
    # Inner shadow
    draw.line([(3, 1), (w - 3, 1)], fill=rgba(BLACK, 80))
    return img


def make_bar_fill(w, h, color_left, color_right):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    gradient_h(draw, 0, 0, w, h, rgba(color_left), rgba(color_right))
    mask = Image.new("L", (w, h), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([0, 0, w - 1, h - 1], radius=4, fill=255)
    img.putalpha(mask)
    # Top shine
    draw2 = ImageDraw.Draw(img)
    for i in range(3):
        a = 60 - i * 20
        draw2.line([(3, 2 + i), (w - 3, 2 + i)], fill=rgba(WHITE, max(a, 0)))
    return img


def make_frame(size, border_color=GOLD, border_w=3, selected=False):
    w, h = size, size
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Dark interior
    draw.rectangle([border_w, border_w, w - border_w - 1, h - border_w - 1],
                   fill=rgba(DARK_NAVY, 180))
    # Border
    draw.rectangle([0, 0, w - 1, h - 1], outline=rgba(border_color, 255), width=border_w)
    if selected:
        # Glow effect - draw a slightly larger border with alpha
        glow_img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        gd = ImageDraw.Draw(glow_img)
        gd.rectangle([0, 0, w - 1, h - 1], outline=rgba(WHITE, 120), width=border_w + 1)
        glow_img = glow_img.filter(ImageFilter.GaussianBlur(2))
        img = Image.alpha_composite(glow_img, img)
        # Bright inner line
        d2 = ImageDraw.Draw(img)
        d2.rectangle([border_w - 1, border_w - 1, w - border_w, h - border_w],
                     outline=rgba(WHITE, 80), width=1)
    # Corner accents
    draw3 = ImageDraw.Draw(img)
    c = rgba(border_color, 255)
    s = 6
    # Top-left
    draw3.line([(0, 0), (s, 0)], fill=c, width=2)
    draw3.line([(0, 0), (0, s)], fill=c, width=2)
    # Top-right
    draw3.line([(w - 1, 0), (w - 1 - s, 0)], fill=c, width=2)
    draw3.line([(w - 1, 0), (w - 1, s)], fill=c, width=2)
    # Bottom-left
    draw3.line([(0, h - 1), (s, h - 1)], fill=c, width=2)
    draw3.line([(0, h - 1), (0, h - 1 - s)], fill=c, width=2)
    # Bottom-right
    draw3.line([(w - 1, h - 1), (w - 1 - s, h - 1)], fill=c, width=2)
    draw3.line([(w - 1, h - 1), (w - 1, h - 1 - s)], fill=c, width=2)
    return img


def draw_polygon(draw, points, fill=None, outline=None, width=1):
    if fill:
        draw.polygon(points, fill=fill)
    if outline:
        draw.polygon(points, outline=outline)


# ============================================================
# PANEL BACKGROUNDS
# ============================================================
print("Generating panels...")

save(make_panel(256, 256, border_color=GOLD, bg_top=DARK_NAVY, bg_bot=DARK_BG,
                border_w=2, alpha=220), "panel_dark.png")
save(make_panel(256, 256, border_color=GOLD, bg_top=MID_BG, bg_bot=LIGHT_BG,
                border_w=2, alpha=200), "panel_light.png")
save(make_panel(256, 256, border_color=GOLD, bg_top=DARK_NAVY, bg_bot=DARK_BG,
                border_w=3, alpha=230), "panel_gold.png")
save(make_panel(200, 120, border_color=GRAY, bg_top=DARK_NAVY, bg_bot=(15, 15, 30),
                border_w=1, alpha=240, radius=5), "panel_tooltip.png")

# ============================================================
# BUTTONS
# ============================================================
print("Generating buttons...")

# Standard buttons
save(make_button(160, 48, DEEP_PURPLE, DARK_BG, GOLD), "btn_normal.png")
save(make_button(160, 48, LIGHT_BG, DEEP_PURPLE, GOLD), "btn_hover.png")
save(make_button(160, 48, DEEP_PURPLE, DARK_BG, GOLD, pressed=True), "btn_pressed.png")
save(make_button(160, 48, DARK_GRAY, DARK_NAVY, GRAY, disabled=True), "btn_disabled.png")

# Gold buttons
gold_dark = (140, 105, 20)
gold_mid = (180, 140, 25)
save(make_button(160, 48, gold_mid, gold_dark, GOLD, border_w=2), "btn_gold_normal.png")
save(make_button(160, 48, GOLD, gold_mid, WHITE, border_w=2), "btn_gold_hover.png")
save(make_button(160, 48, gold_mid, gold_dark, GOLD, border_w=2, pressed=True), "btn_gold_pressed.png")

# Danger buttons
red_dark = (140, 30, 40)
red_mid = (200, 50, 60)
save(make_button(160, 48, red_mid, red_dark, RED_ACCENT, border_w=2), "btn_danger_normal.png")
save(make_button(160, 48, RED_ACCENT, red_mid, WHITE, border_w=2), "btn_danger_hover.png")

# Small buttons
save(make_button(80, 32, DEEP_PURPLE, DARK_BG, GOLD, radius=4), "btn_small_normal.png")
save(make_button(80, 32, LIGHT_BG, DEEP_PURPLE, GOLD, radius=4), "btn_small_hover.png")

# Icon buttons
save(make_button(48, 48, DEEP_PURPLE, DARK_BG, GOLD, radius=6), "btn_icon_normal.png")
save(make_button(48, 48, LIGHT_BG, DEEP_PURPLE, GOLD, radius=6), "btn_icon_hover.png")

# ============================================================
# BARS
# ============================================================
print("Generating bars...")

save(make_bar_bg(200, 20), "bar_health_bg.png")
save(make_bar_fill(200, 20, HEALTH_RED, HEALTH_RED_LIGHT), "bar_health_fill.png")
save(make_bar_bg(200, 20), "bar_mana_bg.png")
save(make_bar_fill(200, 20, MANA_DARK, MANA_BLUE), "bar_mana_fill.png")
save(make_bar_fill(200, 20, GOLD, XP_GOLD_LIGHT), "bar_xp_fill.png")
save(make_bar_fill(200, 20, GREEN_DARK, GREEN), "bar_production_fill.png")
save(make_bar_fill(200, 20, YELLOW_DARK, YELLOW), "bar_morale_fill.png")

# ============================================================
# BORDERS & FRAMES
# ============================================================
print("Generating frames and borders...")

save(make_frame(80, border_color=GOLD, border_w=3), "frame_portrait.png")
save(make_frame(80, border_color=GOLD, border_w=3, selected=True), "frame_portrait_selected.png")
save(make_frame(64, border_color=DEEP_PURPLE, border_w=2), "frame_spell.png")
save(make_frame(64, border_color=GOLD, border_w=2), "frame_building.png")

# Horizontal separator
img = Image.new("RGBA", (256, 4), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
gradient_h(draw, 0, 1, 256, 3, rgba(GOLD, 0), rgba(GOLD, 200))
# Fade out right side too
gradient_h(draw, 192, 1, 256, 3, rgba(GOLD, 200), rgba(GOLD, 0))
save(img, "border_separator_h.png")

# Vertical separator
img = Image.new("RGBA", (4, 256), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
gradient_v(draw, 1, 0, 3, 256, rgba(GOLD, 0), rgba(GOLD, 200))
gradient_v(draw, 1, 192, 3, 256, rgba(GOLD, 200), rgba(GOLD, 0))
save(img, "border_separator_v.png")

# Corner ornament (gold filigree-style)
img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
c = rgba(GOLD, 220)
cl = rgba(GOLD, 140)
# L-shape base
draw.line([(0, 0), (28, 0)], fill=c, width=2)
draw.line([(0, 0), (0, 28)], fill=c, width=2)
# Curl details
draw.arc([2, 2, 18, 18], start=180, end=270, fill=cl, width=2)
draw.arc([4, 4, 12, 12], start=180, end=270, fill=cl, width=1)
# Diamond accent
draw.polygon([(22, 3), (26, 7), (22, 11), (18, 7)], fill=c)
draw.polygon([(3, 22), (7, 26), (3, 30), (-1, 26)], fill=c)
# Small dots
draw.ellipse([12, 1, 15, 4], fill=cl)
draw.ellipse([1, 12, 4, 15], fill=cl)
save(img, "corner_ornament.png")

# ============================================================
# ICONS (32x32 and 24x24 and 16x16)
# ============================================================
print("Generating icons...")


def icon_32(name, draw_fn):
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_fn(draw, 32)
    save(img, name)


def icon_24(name, draw_fn):
    img = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_fn(draw, 24)
    save(img, name)


def icon_16(name, draw_fn):
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw_fn(draw, 16)
    save(img, name)


# Gold coin
def draw_gold_coin(draw, s):
    c = rgba(GOLD, 255)
    cl = rgba(XP_GOLD_LIGHT, 255)
    draw.ellipse([4, 4, s - 5, s - 5], fill=c, outline=rgba((160, 120, 20), 255), width=2)
    # Inner circle
    draw.ellipse([8, 8, s - 9, s - 9], outline=cl, width=1)
    # $ symbol approximation - vertical line
    mid = s // 2
    draw.line([(mid, 9), (mid, s - 10)], fill=rgba((120, 90, 15), 255), width=2)


icon_32("icon_gold.png", draw_gold_coin)


# Mana crystal
def draw_mana(draw, s):
    mid = s // 2
    pts = [(mid, 3), (s - 5, mid), (mid, s - 3), (5, mid)]
    draw.polygon(pts, fill=rgba(MANA_BLUE, 255), outline=rgba((100, 170, 255), 255))
    # Shine
    draw.line([(mid - 2, 8), (mid + 2, mid - 2)], fill=rgba(WHITE, 160), width=1)


icon_32("icon_mana.png", draw_mana)


# Food (wheat/grain)
def draw_food(draw, s):
    c = rgba(GOLD, 230)
    mid = s // 2
    # Stalk
    draw.line([(mid, s - 4), (mid, 8)], fill=rgba((140, 120, 40), 255), width=2)
    # Grain kernels
    for dy in range(0, 12, 3):
        y = 7 + dy
        draw.ellipse([mid - 4, y, mid - 1, y + 4], fill=c)
        draw.ellipse([mid + 1, y, mid + 4, y + 4], fill=c)


icon_32("icon_food.png", draw_food)


# Production (hammer)
def draw_production(draw, s):
    c = rgba(GRAY, 255)
    # Handle
    draw.line([(s // 2, s - 5), (s // 2, 12)], fill=rgba((120, 90, 50), 255), width=3)
    # Head
    draw.rectangle([6, 5, s - 7, 13], fill=c, outline=rgba(WHITE, 100))


icon_32("icon_production.png", draw_production)


# Research (scroll)
def draw_research(draw, s):
    c = rgba((200, 180, 140), 255)
    # Scroll body
    draw.rectangle([8, 5, s - 9, s - 5], fill=c)
    # Top roll
    draw.ellipse([6, 3, s - 7, 9], fill=rgba((220, 200, 160), 255),
                 outline=rgba((160, 140, 100), 255))
    # Bottom roll
    draw.ellipse([6, s - 9, s - 7, s - 3], fill=rgba((220, 200, 160), 255),
                 outline=rgba((160, 140, 100), 255))
    # Text lines
    for y in range(11, s - 10, 3):
        draw.line([(11, y), (s - 12, y)], fill=rgba((100, 80, 60), 180), width=1)


icon_32("icon_research.png", draw_research)


# Population (person silhouette)
def draw_population(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    # Head
    draw.ellipse([mid - 4, 4, mid + 4, 12], fill=c)
    # Body
    draw.polygon([(mid - 7, s - 4), (mid, 14), (mid + 7, s - 4)], fill=c)


icon_32("icon_population.png", draw_population)


# Attack (sword)
def draw_attack(draw, s):
    c = rgba((200, 200, 210), 255)
    # Blade
    draw.polygon([(s // 2, 3), (s // 2 + 3, s // 2), (s // 2, s // 2 + 2),
                   (s // 2 - 3, s // 2)], fill=c)
    # Crossguard
    draw.rectangle([s // 2 - 6, s // 2, s // 2 + 6, s // 2 + 3], fill=rgba(GOLD, 255))
    # Handle
    draw.rectangle([s // 2 - 1, s // 2 + 3, s // 2 + 1, s - 5], fill=rgba((120, 80, 40), 255))
    # Pommel
    draw.ellipse([s // 2 - 2, s - 6, s // 2 + 2, s - 3], fill=rgba(GOLD, 255))


icon_32("icon_attack.png", draw_attack)


# Defense (shield)
def draw_defense(draw, s):
    mid = s // 2
    pts = [(mid, 3), (s - 4, 6), (s - 5, s // 2 + 4), (mid, s - 3), (5, s // 2 + 4), (4, 6)]
    draw.polygon(pts, fill=rgba(DEEP_PURPLE, 255), outline=rgba(GOLD, 255))
    # Inner chevron
    draw.polygon([(mid, 8), (s - 8, 10), (mid, s - 7), (8, 10)],
                 outline=rgba(GOLD, 150), width=1)


icon_32("icon_defense.png", draw_defense)


# Movement (boot/arrow)
def draw_movement(draw, s):
    c = rgba(WHITE, 200)
    # Arrow pointing right
    mid = s // 2
    draw.polygon([(6, mid - 4), (s - 10, mid - 4), (s - 10, mid - 8),
                   (s - 4, mid), (s - 10, mid + 8), (s - 10, mid + 4), (6, mid + 4)], fill=c)


icon_32("icon_movement.png", draw_movement)


# Health (heart)
def draw_heart(draw, s):
    c = rgba(RED_ACCENT, 255)
    mid = s // 2
    # Two circles for top + triangle for bottom
    r = s // 5
    draw.ellipse([mid - 2 * r, 6, mid, 6 + 2 * r], fill=c)
    draw.ellipse([mid, 6, mid + 2 * r, 6 + 2 * r], fill=c)
    draw.polygon([(mid - 2 * r, 6 + r), (mid + 2 * r, 6 + r), (mid, s - 4)], fill=c)


icon_32("icon_health.png", draw_heart)


# Close X (24x24)
def draw_close(draw, s):
    c = rgba(RED_ACCENT, 230)
    w = 3
    draw.line([(5, 5), (s - 6, s - 6)], fill=c, width=w)
    draw.line([(s - 6, 5), (5, s - 6)], fill=c, width=w)


icon_24("icon_close.png", draw_close)


# Expand (24x24) - down arrow
def draw_expand(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    draw.polygon([(5, 7), (s - 5, 7), (mid, s - 7)], fill=c)


icon_24("icon_expand.png", draw_expand)


# Info (24x24)
def draw_info(draw, s):
    c = rgba(MANA_BLUE, 230)
    mid = s // 2
    draw.ellipse([3, 3, s - 4, s - 4], fill=c)
    draw.ellipse([mid - 2, 6, mid + 2, 10], fill=rgba(WHITE, 230))
    draw.rectangle([mid - 2, 12, mid + 1, s - 7], fill=rgba(WHITE, 230))


icon_24("icon_info.png", draw_info)


# Warning (24x24)
def draw_warning(draw, s):
    c = rgba(YELLOW, 240)
    mid = s // 2
    draw.polygon([(mid, 3), (s - 3, s - 4), (3, s - 4)], fill=c,
                 outline=rgba((180, 160, 20), 255))
    draw.rectangle([mid - 1, 8, mid + 1, 14], fill=rgba(BLACK, 230))
    draw.ellipse([mid - 1, 16, mid + 1, 18], fill=rgba(BLACK, 230))


icon_24("icon_warning.png", draw_warning)


# Lock (24x24)
def draw_lock(draw, s):
    c = rgba(GRAY, 230)
    mid = s // 2
    # Shackle
    draw.arc([mid - 5, 3, mid + 5, 13], start=180, end=0, fill=c, width=2)
    # Body
    draw.rectangle([mid - 6, 11, mid + 6, s - 4], fill=c)
    # Keyhole
    draw.ellipse([mid - 2, 14, mid + 2, 18], fill=rgba(DARK_NAVY, 255))


icon_24("icon_lock.png", draw_lock)


# Unlock (24x24)
def draw_unlock(draw, s):
    c = rgba(GOLD, 230)
    mid = s // 2
    # Shackle - offset/open
    draw.arc([mid - 5, 1, mid + 5, 11], start=180, end=350, fill=c, width=2)
    # Body
    draw.rectangle([mid - 6, 11, mid + 6, s - 4], fill=c)
    draw.ellipse([mid - 2, 14, mid + 2, 18], fill=rgba(DARK_NAVY, 255))


icon_24("icon_unlock.png", draw_unlock)


# Star (24x24)
def draw_star(draw, s):
    c = rgba(GOLD, 255)
    mid = s // 2
    # 5-pointed star
    pts = []
    for i in range(10):
        angle = math.radians(i * 36 - 90)
        r = (s // 2 - 3) if i % 2 == 0 else (s // 5)
        x = mid + r * math.cos(angle)
        y = mid + r * math.sin(angle)
        pts.append((x, y))
    draw.polygon(pts, fill=c)


icon_24("icon_star.png", draw_star)


# Chevrons (16x16)
def draw_chevron_right(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    draw.polygon([(4, 3), (s - 4, mid), (4, s - 3)], fill=c)


def draw_chevron_left(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    draw.polygon([(s - 4, 3), (4, mid), (s - 4, s - 3)], fill=c)


def draw_chevron_up(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    draw.polygon([(3, s - 4), (mid, 4), (s - 3, s - 4)], fill=c)


def draw_chevron_down(draw, s):
    c = rgba(WHITE, 200)
    mid = s // 2
    draw.polygon([(3, 4), (mid, s - 4), (s - 3, 4)], fill=c)


icon_16("icon_chevron_right.png", draw_chevron_right)
icon_16("icon_chevron_left.png", draw_chevron_left)
icon_16("icon_chevron_up.png", draw_chevron_up)
icon_16("icon_chevron_down.png", draw_chevron_down)

# ============================================================
# HUD ELEMENTS
# ============================================================
print("Generating HUD elements...")

# Minimap frame (200x200)
img = Image.new("RGBA", (200, 200), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
# Outer border
draw.rectangle([0, 0, 199, 199], outline=rgba(GOLD, 220), width=3)
# Inner dark area
draw.rectangle([3, 3, 196, 196], fill=rgba(DARK_NAVY, 200))
# Corner ornaments
for cx, cy in [(0, 0), (188, 0), (0, 188), (188, 188)]:
    draw.rectangle([cx, cy, cx + 11, cy + 11], fill=rgba(GOLD, 200))
    draw.rectangle([cx + 2, cy + 2, cx + 9, cy + 9], fill=rgba(DARK_NAVY, 200))
save(img, "hud_minimap_frame.png")

# Resource bar (400x40)
save(make_panel(400, 40, border_color=GOLD, bg_top=DARK_NAVY, bg_bot=(12, 12, 28),
                border_w=2, alpha=230, radius=4), "hud_resource_bar.png")

# Action bar (600x80)
save(make_panel(600, 80, border_color=GOLD, bg_top=(8, 8, 22), bg_bot=DARK_NAVY,
                border_w=2, alpha=235, radius=6), "hud_action_bar.png")

# Turn indicator (120x40)
img = make_panel(120, 40, border_color=GOLD, bg_top=DEEP_PURPLE, bg_bot=DARK_NAVY,
                 border_w=2, alpha=230, radius=5)
save(img, "hud_turn_indicator.png")

# Wizard portrait frame (64x64)
save(make_frame(64, border_color=GOLD, border_w=3), "hud_wizard_portrait_frame.png")

# ============================================================
# CURSORS
# ============================================================
print("Generating cursors...")


# Default cursor (arrow)
def make_cursor_default():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Arrow shape
    pts = [(2, 2), (2, 24), (8, 18), (14, 26), (18, 24), (12, 16), (20, 14)]
    draw.polygon(pts, fill=rgba(WHITE, 240), outline=rgba(BLACK, 200))
    return img


save(make_cursor_default(), "cursor_default.png")


# Move cursor (4-way arrows)
def make_cursor_move():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    mid = 16
    c = rgba(WHITE, 230)
    o = rgba(BLACK, 180)
    # Up arrow
    draw.polygon([(mid, 3), (mid + 5, 10), (mid - 5, 10)], fill=c, outline=o)
    # Down
    draw.polygon([(mid, 29), (mid + 5, 22), (mid - 5, 22)], fill=c, outline=o)
    # Left
    draw.polygon([(3, mid), (10, mid - 5), (10, mid + 5)], fill=c, outline=o)
    # Right
    draw.polygon([(29, mid), (22, mid - 5), (22, mid + 5)], fill=c, outline=o)
    # Center
    draw.ellipse([mid - 3, mid - 3, mid + 3, mid + 3], fill=c, outline=o)
    return img


save(make_cursor_move(), "cursor_move.png")


# Attack cursor (sword)
def make_cursor_attack():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    c = rgba((200, 200, 210), 255)
    # Diagonal sword
    draw.line([(4, 4), (22, 22)], fill=c, width=3)
    # Crossguard
    draw.line([(14, 20), (20, 14)], fill=rgba(GOLD, 255), width=3)
    # Handle
    draw.line([(22, 22), (28, 28)], fill=rgba((120, 80, 40), 255), width=3)
    # Tip
    draw.polygon([(2, 2), (4, 8), (8, 4)], fill=c)
    # Red tint overlay
    draw.ellipse([1, 1, 10, 10], outline=rgba(RED_ACCENT, 150), width=1)
    return img


save(make_cursor_attack(), "cursor_attack.png")


# Cast cursor (sparkle/wand)
def make_cursor_cast():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Wand
    draw.line([(20, 4), (8, 26)], fill=rgba((160, 120, 80), 255), width=2)
    # Sparkles at tip
    c = rgba(MANA_BLUE, 230)
    for angle in range(0, 360, 45):
        rad = math.radians(angle)
        x = 20 + int(6 * math.cos(rad))
        y = 4 + int(6 * math.sin(rad))
        draw.ellipse([x - 1, y - 1, x + 1, y + 1], fill=c)
    draw.ellipse([17, 1, 23, 7], fill=rgba(MANA_BLUE, 150))
    # Star at tip
    draw.ellipse([18, 2, 22, 6], fill=rgba(WHITE, 200))
    return img


save(make_cursor_cast(), "cursor_cast.png")


# Forbidden cursor (circle with slash)
def make_cursor_forbidden():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    c = rgba(RED_ACCENT, 230)
    draw.ellipse([4, 4, 28, 28], outline=c, width=3)
    draw.line([(8, 8), (24, 24)], fill=c, width=3)
    return img


save(make_cursor_forbidden(), "cursor_forbidden.png")

# ============================================================
# SCROLLBAR
# ============================================================
print("Generating scrollbar...")

# Track
img = Image.new("RGBA", (16, 128), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
rounded_rect(draw, [2, 0, 13, 127], radius=4, fill=rgba(DARK_NAVY, 200),
             outline=rgba(GRAY, 80), width=1)
save(img, "scrollbar_track.png")

# Thumb
for name, color_top, color_bot, border in [
    ("scrollbar_thumb.png", DEEP_PURPLE, DARK_BG, GOLD),
    ("scrollbar_thumb_hover.png", LIGHT_BG, DEEP_PURPLE, GOLD),
]:
    img = Image.new("RGBA", (16, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    gradient_v(draw, 2, 0, 14, 32, rgba(color_top), rgba(color_bot))
    mask = Image.new("L", (16, 32), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([2, 0, 13, 31], radius=4, fill=255)
    img.putalpha(mask)
    draw2 = ImageDraw.Draw(img)
    draw2.rounded_rectangle([2, 0, 13, 31], radius=4, outline=rgba(border, 180), width=1)
    # Grip lines
    for y in [12, 16, 20]:
        draw2.line([(5, y), (11, y)], fill=rgba(GOLD, 100), width=1)
    save(img, name)

# ============================================================
# TAB / HEADER
# ============================================================
print("Generating tabs and headers...")

# Active tab
img = Image.new("RGBA", (120, 36), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
gradient_v(draw, 0, 0, 120, 36, rgba(DEEP_PURPLE, 240), rgba(DARK_BG, 240))
mask = Image.new("L", (120, 36), 0)
mdraw = ImageDraw.Draw(mask)
mdraw.rounded_rectangle([0, 0, 119, 35], radius=6, fill=255)
# Flat bottom
mdraw.rectangle([0, 20, 119, 35], fill=255)
img.putalpha(mask)
draw2 = ImageDraw.Draw(img)
draw2.rounded_rectangle([0, 0, 119, 35], radius=6, outline=rgba(GOLD, 200), width=2)
# Bottom line matches panel
draw2.line([(0, 35), (119, 35)], fill=rgba(DARK_BG, 240), width=2)
# Top highlight
draw2.line([(8, 2), (112, 2)], fill=rgba(WHITE, 40), width=1)
save(img, "tab_active.png")

# Inactive tab
img = Image.new("RGBA", (120, 36), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
gradient_v(draw, 0, 0, 120, 36, rgba(DARK_NAVY, 180), rgba((8, 8, 20), 180))
mask = Image.new("L", (120, 36), 0)
mdraw = ImageDraw.Draw(mask)
mdraw.rounded_rectangle([0, 0, 119, 35], radius=6, fill=255)
mdraw.rectangle([0, 20, 119, 35], fill=255)
img.putalpha(mask)
draw2 = ImageDraw.Draw(img)
draw2.rounded_rectangle([0, 0, 119, 35], radius=6, outline=rgba(GRAY, 120), width=1)
save(img, "tab_inactive.png")

# Header bar
save(make_panel(512, 48, border_color=GOLD, bg_top=DEEP_PURPLE, bg_bot=DARK_NAVY,
                border_w=2, alpha=230, radius=4), "header_bar.png")

# ============================================================
# DONE
# ============================================================
print(f"\nGeneration complete! {file_count} UI assets saved to: {OUTPUT_DIR}")
