"""
Generate animated spell effect sprite sheets for all 9 realms.
Each spell effect type gets an 8-frame horizontal sprite sheet.
Style: MoM-inspired 2D pixel art spell animations with bright colors.
"""

import torch
from diffusers import AutoPipelineForText2Image
from PIL import Image, ImageEnhance, ImageFilter, ImageDraw
import os
import gc
import math

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\spell_vfx"

# Spell effect types per realm
SPELL_EFFECTS = {
    "life": [
        ("heal", "golden holy light healing beam, radiant sparkles, warm glow"),
        ("shield", "translucent golden holy shield barrier, divine protection aura"),
        ("smite", "holy lightning bolt from heaven, golden divine strike"),
        ("resurrect", "soul rising from body surrounded by golden light, angelic wings"),
    ],
    "death": [
        ("drain", "dark purple life drain tendrils, necrotic energy flowing"),
        ("curse", "swirling dark purple curse runes, shadowy magic"),
        ("raise_dead", "skeletal hand rising from ground, green necrotic glow"),
        ("shadow_bolt", "dark bolt of shadow energy, purple black projectile"),
    ],
    "chaos": [
        ("fireball", "bright orange fireball explosion, flames and sparks"),
        ("lightning", "electric blue lightning bolt strike, electrical discharge"),
        ("meteor", "flaming meteor falling from sky, fire trail impact"),
        ("doom_bolt", "red chaos energy beam, destructive magical blast"),
    ],
    "nature": [
        ("entangle", "green vines and roots erupting from ground, nature magic"),
        ("earthquake", "ground cracking open, brown earth shaking, rocks flying"),
        ("summon_beast", "green nature portal opening, animal spirit emerging"),
        ("growth", "plants rapidly growing, flowers blooming, green energy"),
    ],
    "sorcery": [
        ("dispel", "blue arcane dispel wave, runes shattering"),
        ("teleport", "blue magical teleportation circle, dimensional rift"),
        ("confusion", "swirling blue mind magic, hypnotic spirals"),
        ("counter_spell", "blue magical shield deflecting incoming spell"),
    ],
    "arcane": [
        ("magic_missile", "white glowing magic projectile, arcane energy"),
        ("enchant", "golden arcane runes circling target, enchantment magic"),
        ("detect", "expanding white detection pulse wave, scrying magic"),
        ("ward", "geometric arcane protective ward, glowing runes"),
    ],
    "binding": [
        ("chains", "dark red chains erupting from ground, binding magic"),
        ("dominate", "red hypnotic eyes, domination control beams"),
        ("soul_trap", "red soul container crystal, capturing spirit energy"),
        ("contract", "burning contract scroll with demonic seal"),
    ],
    "spirit": [
        ("ghost_touch", "translucent violet ghostly hand, spectral energy"),
        ("dream", "swirling purple dream mist, ethereal visions"),
        ("possession", "violet spirit entering body, possession magic"),
        ("astral_bolt", "bright violet astral energy projectile"),
    ],
    "glamour": [
        ("illusion", "shimmering pink mirror images, deceptive copies"),
        ("charm", "pink heart sparkles, enchanting charm magic"),
        ("true_sight", "prismatic rainbow eye revealing truth"),
        ("time_stop", "golden clock hands frozen, time magic ripple"),
    ],
}


def generate_sprite_sheet(pipe, prompt, output_path, frames=8, frame_size=128):
    """Generate an 8-frame horizontal sprite sheet by creating variations."""
    sheet_width = frame_size * frames
    sheet = Image.new("RGBA", (sheet_width, frame_size), (0, 0, 0, 0))

    base_prompt = f"{prompt}, magical effect, game sprite, dark background, bright glowing, pixel art style"

    for f in range(frames):
        # Vary the prompt slightly per frame to simulate animation progression
        phase = f / frames
        intensity = "small beginning" if phase < 0.25 else "growing medium" if phase < 0.5 else "full power peak" if phase < 0.75 else "fading dissipating"
        frame_prompt = f"{base_prompt}, {intensity}, frame {f+1} of {frames}"

        img = pipe(
            prompt=frame_prompt,
            num_inference_steps=4,
            guidance_scale=0.0,
            width=512, height=512,
        ).images[0]

        # Resize to frame size
        img = img.resize((frame_size, frame_size), Image.LANCZOS)
        img = ImageEnhance.Color(img).enhance(1.4)
        img = ImageEnhance.Brightness(img).enhance(1.2)

        # Convert to RGBA and make dark areas transparent
        img = img.convert("RGBA")
        pixels = img.load()
        for y in range(frame_size):
            for x in range(frame_size):
                r, g, b, a = pixels[x, y]
                brightness = (r + g + b) / 3
                if brightness < 30:
                    pixels[x, y] = (r, g, b, 0)  # Transparent dark background
                elif brightness < 60:
                    pixels[x, y] = (r, g, b, int(brightness * 4))  # Fade edges

        sheet.paste(img, (f * frame_size, 0), img)

    sheet.save(output_path, "PNG")
    return sheet


def generate_impact_flash(realm_color, output_path, frames=8, frame_size=128):
    """Generate a simple procedural impact/flash effect sprite sheet."""
    sheet = Image.new("RGBA", (frame_size * frames, frame_size), (0, 0, 0, 0))

    for f in range(frames):
        frame = Image.new("RGBA", (frame_size, frame_size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(frame)
        phase = f / frames
        cx, cy = frame_size // 2, frame_size // 2

        if phase < 0.5:
            # Expanding flash
            progress = phase * 2
            radius = int(10 + 50 * progress)
            for r in range(radius, 0, -2):
                alpha = int(200 * (r / radius) * (1 - progress * 0.5))
                color = (
                    min(255, int(realm_color[0] + (255 - realm_color[0]) * (1 - r / radius))),
                    min(255, int(realm_color[1] + (255 - realm_color[1]) * (1 - r / radius))),
                    min(255, int(realm_color[2] + (255 - realm_color[2]) * (1 - r / radius))),
                    alpha,
                )
                draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color)
        else:
            # Fading
            progress = (phase - 0.5) * 2
            radius = int(60 * (1 - progress))
            for r in range(radius, 0, -2):
                alpha = int(150 * (1 - progress) * (r / max(radius, 1)))
                color = (*realm_color, alpha)
                draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color)

            # Add sparkle particles
            import random
            random.seed(f * 42)
            for _ in range(int(8 * (1 - progress))):
                px = cx + random.randint(-radius - 10, radius + 10)
                py = cy + random.randint(-radius - 10, radius + 10)
                sz = random.randint(1, 3)
                alpha = int(200 * (1 - progress))
                draw.ellipse([px - sz, py - sz, px + sz, py + sz],
                             fill=(*realm_color, alpha))

        sheet.paste(frame, (f * frame_size, 0), frame)

    sheet.save(output_path, "PNG")


def main():
    print("=== Loading SD Turbo for spell VFX ===")
    pipe = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe = pipe.to("cuda")
    pipe.enable_attention_slicing()

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Realm colors for procedural effects
    realm_colors = {
        "life": (255, 220, 100),
        "death": (140, 40, 160),
        "chaos": (255, 80, 20),
        "nature": (50, 200, 50),
        "sorcery": (60, 120, 255),
        "arcane": (220, 180, 60),
        "binding": (180, 30, 30),
        "spirit": (150, 100, 255),
        "glamour": (255, 100, 200),
    }

    total = 0

    # Generate AI sprite sheets for each spell effect
    for realm, effects in SPELL_EFFECTS.items():
        realm_dir = os.path.join(OUTPUT_DIR, realm)
        os.makedirs(realm_dir, exist_ok=True)

        for effect_name, prompt in effects:
            outpath = os.path.join(realm_dir, f"{effect_name}_sheet.png")
            if os.path.exists(outpath):
                print(f"  Skip (exists): {realm}/{effect_name}")
                continue

            print(f"  Generating: {realm}/{effect_name}...")
            generate_sprite_sheet(pipe, prompt, outpath)
            total += 1

        # Generate procedural impact flash for each realm
        flash_path = os.path.join(realm_dir, "impact_flash.png")
        if not os.path.exists(flash_path):
            print(f"  Generating: {realm}/impact_flash (procedural)...")
            generate_impact_flash(realm_colors[realm], flash_path)
            total += 1

    # Generate universal effects
    universal_dir = os.path.join(OUTPUT_DIR, "universal")
    os.makedirs(universal_dir, exist_ok=True)

    universal_effects = [
        ("casting_glow", "magical casting glow around wizard hands, arcane energy building"),
        ("mana_drain", "blue mana energy being drained away, dissipating particles"),
        ("level_up", "golden level up sparkle explosion, achievement celebration"),
        ("death_effect", "unit dissolving into particles, death fade away"),
        ("summon_portal", "swirling magical portal opening, dimensional gateway"),
    ]

    for name, prompt in universal_effects:
        outpath = os.path.join(universal_dir, f"{name}_sheet.png")
        if os.path.exists(outpath):
            continue
        print(f"  Generating: universal/{name}...")
        generate_sprite_sheet(pipe, prompt, outpath)
        total += 1

    del pipe
    gc.collect()
    torch.cuda.empty_cache()

    file_count = sum(1 for _ in os.walk(OUTPUT_DIR) for f in _[2] if f.endswith('.png'))
    print(f"\n=== Done — {total} new + {file_count} total spell VFX sprite sheets ===")


if __name__ == "__main__":
    main()
