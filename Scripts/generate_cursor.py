"""Generate a magic wand cursor (32x32 with transparency)."""
from PIL import Image, ImageDraw
import os
import math

OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\ui\cursor_wand.png"

def main():
    size = 32
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Wand shaft — diagonal line from bottom-left to upper-right
    # Tip at (4, 4), handle at (26, 26)
    shaft_color = (180, 140, 60, 255)     # Bronze/wood
    shaft_dark = (120, 90, 40, 255)       # Shadow side
    shaft_highlight = (220, 180, 100, 255) # Light side

    # Draw shaft (3px wide diagonal)
    for offset in range(-1, 2):
        draw.line([(6 + offset, 6), (26 + offset, 26)], fill=shaft_color, width=1)
    # Highlight edge
    draw.line([(5, 6), (25, 26)], fill=shaft_highlight, width=1)
    # Shadow edge
    draw.line([(7, 6), (27, 26)], fill=shaft_dark, width=1)

    # Wand tip — bright glowing star at (4, 4)
    tip_x, tip_y = 5, 5
    gold = (255, 220, 80, 255)
    white = (255, 255, 240, 255)
    glow = (255, 200, 50, 120)

    # Glow halo
    for r in range(6, 0, -1):
        alpha = int(80 * (r / 6))
        draw.ellipse([tip_x - r, tip_y - r, tip_x + r, tip_y + r],
                     fill=(255, 200, 50, alpha))

    # Star sparkle — 4 pointed
    for angle in [0, 90, 45, 135]:
        rad = math.radians(angle)
        x2 = tip_x + int(4 * math.cos(rad))
        y2 = tip_y - int(4 * math.sin(rad))
        draw.line([(tip_x, tip_y), (x2, y2)], fill=gold, width=1)

    # Center bright dot
    draw.ellipse([tip_x - 1, tip_y - 1, tip_x + 1, tip_y + 1], fill=white)

    # Handle gem — small colored dot at handle end
    gem_x, gem_y = 26, 26
    draw.ellipse([gem_x - 2, gem_y - 2, gem_x + 2, gem_y + 2], fill=(100, 50, 180, 255))  # Purple gem
    draw.ellipse([gem_x - 1, gem_y - 1, gem_x + 1, gem_y + 1], fill=(180, 120, 255, 255))  # Gem highlight

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    img.save(OUTPUT, "PNG")
    print(f"Saved: {OUTPUT}")

if __name__ == "__main__":
    main()
