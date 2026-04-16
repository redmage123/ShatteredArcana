"""
Generate two assets:
1. Custom wand mouse cursor (32x32 PNG with transparency)
2. Illuminated manuscript backstory page with gothic drop cap
"""

import torch
from diffusers import AutoPipelineForText2Image, AutoPipelineForImage2Image
from PIL import Image, ImageEnhance, ImageFilter, ImageDraw, ImageFont
import os
import gc

CURSOR_OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Content\Textures\UI\T_Cursor_Wand.png"
MANUSCRIPT_OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Content\Textures\UI\T_Backstory.png"

def generate_wand_cursor(pipe):
    """Generate a 32x32 magic wand cursor with transparency."""
    print("=== Generating wand cursor ===")

    # Generate a larger wand image and scale down
    prompt = (
        "magic wand icon, glowing tip, golden handle with gems, "
        "fantasy RPG cursor, pixel art style, transparent background, "
        "diagonal pointing upper-left, simple clean design, game UI icon"
    )

    img = pipe(
        prompt=prompt,
        num_inference_steps=4,
        guidance_scale=0.0,
        width=512,
        height=512,
    ).images[0]

    # Create a proper cursor from the generated image
    # Resize to 64x64 first for detail, then we'll make a proper cursor
    img = img.resize((64, 64), Image.LANCZOS)
    img = img.convert("RGBA")

    # Create a clean wand cursor programmatically (SD won't do well at 32x32)
    cursor = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(cursor)

    # Wand shaft (diagonal, bottom-right to upper-left)
    for i in range(20):
        x = 28 - i
        y = 28 - i
        # Shaft: dark wood to golden
        r = int(120 + i * 5)
        g = int(80 + i * 4)
        b = int(30 + i * 1)
        draw.ellipse([x-1, y-1, x+1, y+1], fill=(r, g, b, 255))

    # Wand tip (glowing magical point at top-left)
    # Glow halo
    for r in range(6, 0, -1):
        alpha = int(60 * (r / 6))
        draw.ellipse([8-r, 8-r, 8+r, 8+r], fill=(200, 180, 255, alpha))

    # Bright tip
    draw.ellipse([6, 6, 10, 10], fill=(255, 220, 100, 255))
    draw.ellipse([7, 7, 9, 9], fill=(255, 255, 200, 255))

    # Star sparkle at tip
    draw.line([(8, 3), (8, 13)], fill=(255, 255, 180, 180), width=1)
    draw.line([(3, 8), (13, 8)], fill=(255, 255, 180, 180), width=1)

    # Handle decoration (gem at base)
    draw.ellipse([25, 25, 29, 29], fill=(180, 50, 50, 255))  # Ruby
    draw.ellipse([26, 26, 28, 28], fill=(255, 100, 100, 255))  # Ruby highlight

    # Hotspot is at the tip (8, 8)
    os.makedirs(os.path.dirname(CURSOR_OUTPUT), exist_ok=True)
    cursor.save(CURSOR_OUTPUT, "PNG")
    print(f"  Saved cursor: {CURSOR_OUTPUT} (32x32)")

    # Also save a 64x64 version for high DPI
    cursor_hd = cursor.resize((64, 64), Image.NEAREST)
    cursor_hd.save(CURSOR_OUTPUT.replace(".png", "_64.png"), "PNG")
    print(f"  Saved HD cursor: {CURSOR_OUTPUT.replace('.png', '_64.png')} (64x64)")


def generate_manuscript(pipe, pipe_i2i):
    """Generate an illuminated manuscript backstory page."""
    print("\n=== Generating illuminated manuscript backstory ===")

    # Generate the manuscript background with SD
    prompt = (
        "Illuminated medieval manuscript page, aged parchment texture, "
        "ornate gold and blue border decorations with vines and flowers, "
        "gothic medieval calligraphy text, "
        "large ornate decorated initial capital letter in gold leaf, "
        "medieval book of hours style, fantasy grimoire page, "
        "detailed miniature painting in the margin, "
        "warm aged parchment background"
    )

    base = pipe(
        prompt=prompt,
        num_inference_steps=4,
        guidance_scale=0.0,
        width=512,
        height=512,
    ).images[0]

    # Upscale with detail
    base_2x = base.resize((1024, 1024), Image.LANCZOS)
    hires = pipe_i2i(
        prompt=prompt,
        image=base_2x,
        num_inference_steps=4,
        guidance_scale=0.0,
        strength=0.35,
        width=1024,
        height=1024,
    ).images[0]

    # Final size: 800x1000 (portrait orientation like a manuscript page)
    manuscript = hires.resize((800, 1000), Image.LANCZOS)
    manuscript = ImageEnhance.Color(manuscript).enhance(1.2)
    manuscript = ImageEnhance.Contrast(manuscript).enhance(1.1)
    manuscript = ImageEnhance.Warmth(manuscript).enhance(1.1) if hasattr(ImageEnhance, 'Warmth') else manuscript

    # Overlay the backstory text
    # We'll create a text overlay with a gothic drop cap
    text_overlay = Image.new("RGBA", (800, 1000), (0, 0, 0, 0))
    tdraw = ImageDraw.Draw(text_overlay)

    backstory = (
        "n an age before memory, when the eight planes "
        "of existence were yet young, the great wizards "
        "discovered the Arcane Nexus — a web of ley lines "
        "binding all realms together. From Aurelith's golden "
        "spires to the abyssal depths of the demon planes, "
        "power flowed like rivers of starlight.\n\n"
        "But power breeds ambition. The wizard-kings turned "
        "upon each other, shattering the Nexus into a thousand "
        "fragments. Now the planes drift apart, their magic "
        "fading. Only one who masters all nine schools of "
        "sorcery can reforge the Arcana and claim the throne "
        "of all worlds.\n\n"
        "You are one such wizard. Will you conquer through "
        "steel and spell? Forge alliances across the planes? "
        "Or unravel the very fabric of reality itself?\n\n"
        "Eight planes await. A thousand spells lie dormant. "
        "One throne stands empty."
    )

    # Draw the illuminated drop cap "I" (first letter)
    # Large ornate "I" in gold
    drop_cap_size = 120
    tdraw.rectangle([60, 80, 60 + drop_cap_size, 80 + drop_cap_size],
                    fill=(180, 140, 40, 200))
    tdraw.rectangle([65, 85, 55 + drop_cap_size, 75 + drop_cap_size],
                    fill=(30, 20, 60, 220))

    # The letter "I" in gold
    try:
        large_font = ImageFont.truetype("arial.ttf", 100)
        small_font = ImageFont.truetype("arial.ttf", 22)
    except:
        large_font = ImageFont.load_default()
        small_font = ImageFont.load_default()

    # Gold ornate "I"
    tdraw.text((85, 75), "I", fill=(218, 165, 32, 255), font=large_font)

    # Gold border around the drop cap
    for offset in range(3):
        tdraw.rectangle(
            [58 - offset, 78 - offset, 62 + drop_cap_size + offset, 82 + drop_cap_size + offset],
            outline=(218, 165, 32, 180))

    # Main text body (wrapping manually)
    x_start = 190  # After the drop cap for first few lines
    y_pos = 90
    line_height = 30
    max_width = 700

    words = backstory.split()
    line = ""
    lines_drawn = 0

    for word in words:
        if word == "\n\n" or (line and "\n\n" in word):
            # Draw current line
            x = x_start if lines_drawn < 4 else 80
            tdraw.text((x, y_pos), line.strip(), fill=(40, 30, 20, 220), font=small_font)
            y_pos += line_height
            line = ""
            if "\n\n" in word:
                y_pos += line_height // 2  # Paragraph spacing
                word = word.replace("\n\n", "")
            lines_drawn += 1
            continue

        test_line = line + " " + word if line else word
        # Rough width estimate
        est_width = len(test_line) * 12
        avail_width = max_width - (x_start if lines_drawn < 4 else 80)

        if est_width > avail_width:
            x = x_start if lines_drawn < 4 else 80
            tdraw.text((x, y_pos), line.strip(), fill=(40, 30, 20, 220), font=small_font)
            y_pos += line_height
            line = word
            lines_drawn += 1
            if lines_drawn >= 4:
                x_start = 80  # Full width after drop cap
        else:
            line = test_line

    # Draw remaining text
    if line:
        x = 80
        tdraw.text((x, y_pos), line.strip(), fill=(40, 30, 20, 220), font=small_font)

    # Composite text onto manuscript
    manuscript = manuscript.convert("RGBA")
    manuscript = Image.alpha_composite(manuscript, text_overlay)
    manuscript = manuscript.convert("RGB")

    os.makedirs(os.path.dirname(MANUSCRIPT_OUTPUT), exist_ok=True)
    manuscript.save(MANUSCRIPT_OUTPUT, "PNG", optimize=True)
    print(f"  Saved manuscript: {MANUSCRIPT_OUTPUT} ({manuscript.size})")


def main():
    print("=== Loading SD Turbo ===")
    pipe = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe = pipe.to("cuda")
    pipe.enable_attention_slicing()
    pipe_i2i = AutoPipelineForImage2Image.from_pipe(pipe)

    generate_wand_cursor(pipe)
    generate_manuscript(pipe, pipe_i2i)

    del pipe, pipe_i2i
    gc.collect()
    torch.cuda.empty_cache()
    print("\n=== All assets generated ===")


if __name__ == "__main__":
    main()
