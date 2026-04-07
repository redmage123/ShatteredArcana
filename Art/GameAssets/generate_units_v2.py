#!/usr/bin/env python3
"""
Phase 5 v2: Re-generate unit sprites with fixed prompts.
Problem: v1 generated sprite sheets instead of single figures.
Fix: stronger single-figure emphasis, better negative prompt, adjusted guidance.
"""

import torch
import gc
import os
import sys
from pathlib import Path
from PIL import Image
import numpy as np

BASE_DIR = Path("/home/bbrelin/ShatteredArcana/Art/GameAssets")

RACES = {
    "high_men":       ("Aurelith", "noble human knight, golden plate armor, sword and shield, regal"),
    "high_elves":     ("Aurelith", "elegant elf warrior, silvery chainmail, longbow, pointed ears, tall"),
    "dwarves":        ("Aurelith", "stout dwarf warrior, heavy iron plate, battle axe, thick beard"),
    "halflings":      ("Aurelith", "small halfling scout, leather armor, short sword, nimble"),
    "gnomes":         ("Aurelith", "gnome tinker, goggles, mechanical crossbow, small stature"),
    "draconians":     ("Aurelith", "dragonborn warrior, bronze scaled skin, halberd, draconic"),
    "dark_elves":     ("Noctharion", "dark elf assassin, obsidian armor, twin daggers, white hair, purple skin"),
    "demonkin":       ("Noctharion", "demon-blooded warrior, curved horns, dark red skin, flaming sword"),
    "shadow_folk":    ("Noctharion", "shadow creature, partially transparent, dark wispy form, glowing eyes"),
    "vampires":       ("Noctharion", "vampire lord, pale aristocratic face, crimson cape, rapier"),
    "liches":         ("Noctharion", "undead lich mage, skeletal face, glowing green eyes, dark staff"),
    "lizardmen":      ("Verdantis", "lizardfolk warrior, green scaled skin, bone spear, tribal markings"),
    "dryads":         ("Verdantis", "tree spirit dryad, bark-textured skin, leaf crown, wooden staff"),
    "beastmen":       ("Verdantis", "beast-hybrid warrior, fur-covered, claws, bestial rage"),
    "centaurs":       ("Verdantis", "centaur warrior, horse body, human torso, longbow drawn"),
    "treants":        ("Verdantis", "living tree creature, massive bark body, branch arms, ancient"),
    "demons":         ("Infernyx", "demon warrior, red skin, black horns, flaming greatsword"),
    "efreeti":        ("Infernyx", "fire genie efreet, blazing orange skin, smoke trail, scimitar"),
    "ashen":          ("Infernyx", "ash-covered warrior, grey cracked skin, ember eyes, mace"),
    "iron_devils":    ("Infernyx", "iron-clad devil, chain armor, barbed spear, metallic skin"),
    "fire_elementals":("Infernyx", "living fire elemental, pure flame body, no solid form, burning"),
    "crystal_elves":  ("Aethermist", "crystal elf warrior, prismatic translucent armor, crystal blade"),
    "celestials":     ("Aethermist", "angelic being, white feathered wings, golden halo, holy sword"),
    "echomancers":    ("Aethermist", "sound mage, vibrating blue aura, sonic gauntlets, focused"),
    "ether_weavers":  ("Aethermist", "ethereal spirit weaver, ghostly robes, glowing thread magic"),
    "star_born":      ("Aethermist", "stellar being, constellation patterns on dark skin, cosmic glow"),
    "abyssal_terrors":("Abyssal", "deep sea horror, tentacle face, bioluminescent, massive teeth"),
    "merfolk":        ("Abyssal", "mer-warrior, fish-scaled torso, coral armor, trident, fins"),
    "chithari":       ("Abyssal", "insectoid warrior, chitinous black armor, mandibles, four arms"),
    "kraken_spawn":   ("Abyssal", "octopoid creature, multiple tentacle arms, ink-dark skin"),
    "deep_ones":      ("Abyssal", "ancient deep sea humanoid, fish features, barnacle-encrusted"),
    "dream_weavers":  ("Ethereal", "dream being, shifting translucent form, surreal floating"),
    "phase_stalkers": ("Ethereal", "phase-shifting predator, flickering outline, partially invisible"),
    "thought_forms":  ("Ethereal", "psychic energy being, crystallized thought, glowing geometric"),
    "void_touched":   ("Ethereal", "void-corrupted humanoid, reality-warping black aura, cracked skin"),
    "astral_nomads":  ("Ethereal", "astral traveler, silver cord trailing, floating robes, starfield"),
    "fey_elves":      ("Feywild", "fey elf warrior, flower-petal armor, butterfly wings, radiant"),
    "pixies":         ("Feywild", "pixie sprite, tiny with dragonfly wings, glowing dust trail"),
    "erlking_guard":  ("Feywild", "wild hunt guard, antler helm, green leather, oak spear"),
    "satyrs":         ("Feywild", "goat-legged satyr, pan pipes, vine-wrapped staff, mischievous"),
    "spriggans":      ("Feywild", "spriggan earth-fey, bark and stone body, moss-covered, bulky"),
    "djinn":          ("Feywild", "air djinn, swirling blue mist lower body, golden vest, turbaned"),
}

UNIT_TYPES = {
    "infantry": "standing in combat stance, weapon raised, armored, facing viewer",
    "ranged":   "aiming ranged weapon, bow drawn or staff pointed, alert stance",
    "cavalry":  "mounted on war beast or horse, charging forward, lance or weapon raised",
}

PLANE_STYLES = {
    "Aurelith":   "warm golden lighting",
    "Noctharion": "dark purple ambient glow",
    "Verdantis":  "lush green natural light",
    "Infernyx":   "fiery red-orange glow",
    "Aethermist": "ethereal crystal blue light",
    "Abyssal":    "deep teal bioluminescent",
    "Ethereal":   "ghostly pale green-white",
    "Feywild":    "vibrant fey multicolor",
}

# FIXED negative prompt — explicitly reject sprite sheets and multiple figures
NEGATIVE = (
    "multiple figures, sprite sheet, grid, tiled, repeated, pattern, "
    "group of characters, crowd, army, collection, montage, collage, "
    "text, watermark, border, frame, landscape, background scenery, "
    "realistic photograph, blurry, low quality, jpeg artifacts, "
    "side view from behind, top of head only"
)


def remove_bg_smart(img: Image.Image) -> Image.Image:
    """Background removal using edge detection + corner sampling."""
    arr = np.array(img.convert("RGBA"))
    h, w = arr.shape[:2]

    # Sample larger corner regions for background color
    corner_size = max(h, w) // 8
    corners = []
    for cy, cx in [(0, 0), (0, w-corner_size), (h-corner_size, 0), (h-corner_size, w-corner_size)]:
        region = arr[cy:cy+corner_size, cx:cx+corner_size, :3]
        corners.append(region.reshape(-1, 3).mean(axis=0))
    bg_color = np.mean(corners, axis=0).astype(np.float32)

    # Color distance from background
    diff = np.sqrt(((arr[:,:,:3].astype(np.float32) - bg_color) ** 2).sum(axis=2))

    # Adaptive threshold based on color variance
    threshold = max(30, np.percentile(diff, 25))

    # Create alpha: foreground where sufficiently different from background
    alpha = np.where(diff > threshold, 255, 0).astype(np.uint8)

    # Clean up with morphological operations (close small holes)
    from PIL import ImageFilter
    alpha_img = Image.fromarray(alpha, mode='L')
    alpha_img = alpha_img.filter(ImageFilter.MaxFilter(3))  # dilate
    alpha_img = alpha_img.filter(ImageFilter.MinFilter(3))  # erode
    alpha = np.array(alpha_img)

    arr[:,:,3] = alpha
    return Image.fromarray(arr)


def main():
    from diffusers import StableDiffusionXLPipeline

    # Clear old sprites
    print("Clearing old unit sprites...")
    proc_units = BASE_DIR / "processed" / "units"
    raw_units = BASE_DIR / "raw" / "units"
    import shutil
    if proc_units.exists():
        shutil.rmtree(str(proc_units))
    if raw_units.exists():
        shutil.rmtree(str(raw_units))

    print("Loading SDXL pipeline...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        "stabilityai/stable-diffusion-xl-base-1.0",
        torch_dtype=torch.float16, variant="fp16", use_safetensors=True,
    ).to("cuda")
    pipe.enable_attention_slicing()

    total = len(RACES) * len(UNIT_TYPES)
    done = 0

    for race_idx, (race_name, (plane, race_desc)) in enumerate(RACES.items()):
        race_raw = raw_units / race_name
        race_proc = proc_units / race_name
        race_raw.mkdir(parents=True, exist_ok=True)
        race_proc.mkdir(parents=True, exist_ok=True)

        plane_style = PLANE_STYLES.get(plane, "")

        for unit_idx, (unit_type, unit_pose) in enumerate(UNIT_TYPES.items()):
            raw_path = race_raw / f"{unit_type}.png"
            proc_path = race_proc / f"{unit_type}.png"

            # Key fix: emphasize SINGLE CHARACTER
            prompt = (
                f"single character portrait, one {race_name.replace('_', ' ')} {unit_type}, "
                f"{race_desc}, {unit_pose}, "
                f"{plane_style}, fantasy game character art, "
                f"centered in frame, full body visible, "
                f"clean simple background, isolated figure, "
                f"detailed character design, professional game art"
            )

            seed = 7000 + race_idx * 10 + unit_idx  # New seed range to avoid old results
            gen = torch.Generator("cuda").manual_seed(seed)

            try:
                result = pipe(
                    prompt=prompt,
                    negative_prompt=NEGATIVE,
                    num_inference_steps=20,   # More steps for quality
                    guidance_scale=9.0,       # Higher guidance for prompt adherence
                    width=1024,
                    height=1024,
                    generator=gen,
                ).images[0]

                result.save(str(raw_path))

                # Better background removal
                processed = remove_bg_smart(result)
                processed = processed.resize((64, 64), Image.LANCZOS)
                processed.save(str(proc_path))

                done += 1
                print(f"  [{done}/{total}] {race_name}/{unit_type} ✓")

            except Exception as e:
                print(f"  FAIL {race_name}/{unit_type}: {e}")
                done += 1

    del pipe
    gc.collect()
    torch.cuda.empty_cache()

    print(f"\n{'='*60}")
    print(f"UNIT SPRITES V2 COMPLETE: {done}/{total}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
