#!/usr/bin/env python3
"""
Phase 1: Generate terrain tilesets for all 8 planes.
160 tiles = 20 terrain types × 8 planes.
Uses SDXL base with consistent per-plane style prompts.
"""

import torch
import gc
import os
import sys
from pathlib import Path
from PIL import Image
import numpy as np

# Paths
BASE_DIR = Path("/home/bbrelin/ShatteredArcana/Art/GameAssets")
RAW_DIR = BASE_DIR / "raw" / "terrain"
PROC_DIR = BASE_DIR / "processed" / "terrain"
QA_DIR = BASE_DIR / "qa" / "tileability"
LOG_FILE = BASE_DIR / "logs" / "terrain_generation.log"

# ─────────────────────────────────────────────────────────────────────────────
# PLANE STYLE PROMPTS
# ─────────────────────────────────────────────────────────────────────────────

PLANE_STYLES = {
    "aurelith":   "golden radiant lighting, warm amber tones, white marble accents, sunlit, high fantasy, celestial gold",
    "noctharion": "dark arcane atmosphere, deep purple shadows, obsidian black, silver runes, eldritch glow, midnight",
    "verdantis":  "lush verdant green, bioluminescent accents, living wood, overgrown vines, primal nature, emerald",
    "infernyx":   "volcanic red-orange, molten lava glow, dark iron, ash and ember, sulfurous, charred",
    "aethermist": "ethereal crystal blue, floating particles, iridescent crystal, celestial white, spirit energy, aurora",
    "abyssal":    "deep ocean teal, bioluminescent cyan, tentacle textures, dark water, coral and bone, abyss",
    "ethereal":   "ghostly translucent, pale green-white, dreamlike distortion, alien geometry, mist, spectral",
    "feywild":    "vibrant shifting colors, mushroom purple-pink, fey glamour, iridescent, whimsical twilight, enchanted",
}

TERRAIN_TYPES = {
    "ocean":           "deep open water, gentle waves, ocean surface pattern",
    "coast":           "sandy shoreline meeting water, beach and shallow surf",
    "grassland":       "rolling grass plains, short green grass, scattered wildflowers",
    "forest":          "dense tree canopy from above, forest floor shadows, leaf pattern",
    "hills":           "rolling hills terrain, grassy mounds, gentle elevation changes",
    "mountain":        "rocky mountain peaks from above, grey stone, jagged ridges",
    "desert":          "sandy desert dunes, arid wasteland, wind-swept sand patterns",
    "tundra":          "frozen tundra, ice and snow cover, sparse dead vegetation",
    "swamp":           "murky swampland, dark water pools, moss and reeds, mud",
    "jungle":          "thick tropical jungle canopy, dense vegetation, hanging vines",
    "volcanic":        "volcanic terrain, lava cracks in dark rock, glowing fissures",
    "crystal":         "crystal formations growing from ground, prismatic mineral deposits",
    "shadow":          "shadow-corrupted ground, dark tendrils spreading, void cracks",
    "corrupted":       "corrupted blighted land, diseased vegetation, purple toxic veins",
    "ethereal":        "translucent ghostly ground, fading reality, dimensional rifts",
    "fey":             "fairy ring mushroom ground, glowing flowers, shifting color patches",
    "underdark_cave":  "underground cave floor, stalagmites from above, dark stone cavern",
    "underdark_fungal":"underground mushroom forest from above, bioluminescent giant fungi",
    "underwater_reef": "colorful coral reef from above, sea anemones, tropical fish",
    "underwater_abyss":"deep ocean floor, hydrothermal vents, abyssal darkness, pressure",
}

NEGATIVE_PROMPT = (
    "text, watermark, border, frame, 3d render, perspective distortion, "
    "characters, people, buildings, horizon line, sky, side view, "
    "blurry, low quality, jpeg artifacts, signature"
)

# ─────────────────────────────────────────────────────────────────────────────
# TILEABILITY POSTPROCESSING
# ─────────────────────────────────────────────────────────────────────────────

def make_tileable(img: Image.Image, blend_width: int = 64) -> Image.Image:
    """Make an image tileable by swapping quadrants and blending seams."""
    arr = np.array(img).astype(np.float32)
    h, w = arr.shape[:2]
    hh, hw = h // 2, w // 2

    # Swap quadrants (top-left ↔ bottom-right, top-right ↔ bottom-left)
    swapped = np.zeros_like(arr)
    swapped[:hh, :hw] = arr[hh:, hw:]
    swapped[:hh, hw:] = arr[hh:, :hw]
    swapped[hh:, :hw] = arr[:hh, hw:]
    swapped[hh:, hw:] = arr[:hh, :hw]

    # Blend the center seams
    for axis in [0, 1]:
        center = hh if axis == 0 else hw
        for offset in range(-blend_width, blend_width):
            pos = center + offset
            if 0 <= pos < (h if axis == 0 else w):
                alpha = 0.5 + 0.5 * (offset / blend_width)
                alpha = max(0.0, min(1.0, alpha))
                if axis == 0:
                    swapped[pos, :] = swapped[pos, :] * alpha + arr[pos, :] * (1 - alpha)
                else:
                    swapped[:, pos] = swapped[:, pos] * alpha + arr[:, pos] * (1 - alpha)

    return Image.fromarray(swapped.clip(0, 255).astype(np.uint8))


def create_tile_preview(tile: Image.Image, output_path: Path, grid_size: int = 3):
    """Create a 3×3 tiled preview to visually verify tileability."""
    tw, th = tile.size
    preview = Image.new("RGB", (tw * grid_size, th * grid_size))
    for row in range(grid_size):
        for col in range(grid_size):
            preview.paste(tile, (col * tw, row * th))
    preview.save(str(output_path))


# ─────────────────────────────────────────────────────────────────────────────
# MAIN GENERATION
# ─────────────────────────────────────────────────────────────────────────────

def main():
    from diffusers import StableDiffusionXLPipeline

    # Determine which planes/terrains to generate (support resume)
    planes = list(PLANE_STYLES.keys())
    terrains = list(TERRAIN_TYPES.keys())

    # Check for command-line plane filter
    if len(sys.argv) > 1:
        planes = [p for p in sys.argv[1:] if p in PLANE_STYLES]
        if not planes:
            print(f"Unknown planes: {sys.argv[1:]}. Valid: {list(PLANE_STYLES.keys())}")
            return

    total = len(planes) * len(terrains)
    print(f"Generating {total} terrain tiles ({len(planes)} planes × {len(terrains)} terrains)")
    print(f"Output: {RAW_DIR}")

    # Load SDXL
    print("Loading SDXL pipeline...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        "stabilityai/stable-diffusion-xl-base-1.0",
        torch_dtype=torch.float16,
        variant="fp16",
        use_safetensors=True,
    ).to("cuda")
    pipe.enable_attention_slicing()

    log_lines = []
    generated = 0
    skipped = 0

    for plane_idx, plane in enumerate(planes):
        plane_style = PLANE_STYLES[plane]
        print(f"\n{'='*60}")
        print(f"PLANE: {plane} ({plane_idx+1}/{len(planes)})")
        print(f"{'='*60}")

        for terrain_idx, (terrain_name, terrain_desc) in enumerate(TERRAIN_TYPES.items()):
            raw_path = RAW_DIR / plane / f"{terrain_name}.png"
            proc_path = PROC_DIR / plane / f"{terrain_name}.png"

            # Skip if already generated
            if proc_path.exists():
                skipped += 1
                continue

            # Build prompt
            prompt = (
                f"{plane_style}, top-down view, seamless tileable texture, "
                f"{terrain_desc}, game art terrain tile, flat perspective, "
                f"no horizon, uniform lighting, detailed texture, clean edges"
            )

            # Deterministic seed: plane_idx * 1000 + terrain_idx
            seed = plane_idx * 1000 + terrain_idx + 42
            generator = torch.Generator("cuda").manual_seed(seed)

            try:
                result = pipe(
                    prompt=prompt,
                    negative_prompt=NEGATIVE_PROMPT,
                    num_inference_steps=20,
                    guidance_scale=7.0,
                    width=1024,
                    height=1024,
                    generator=generator,
                ).images[0]

                # Save raw
                result.save(str(raw_path))

                # Make tileable and downscale to 128×128
                tileable = make_tileable(result)
                tile_128 = tileable.resize((128, 128), Image.LANCZOS)
                tile_128.save(str(proc_path))

                # QA: create tiled preview
                qa_path = QA_DIR / f"{plane}_{terrain_name}_3x3.png"
                create_tile_preview(tile_128, qa_path)

                generated += 1
                total_done = generated + skipped
                print(f"  [{total_done}/{total}] {plane}/{terrain_name} ✓ (seed={seed})")
                log_lines.append(f"OK {plane}/{terrain_name} seed={seed}")

            except Exception as e:
                print(f"  [{generated+skipped}/{total}] {plane}/{terrain_name} FAILED: {e}")
                log_lines.append(f"FAIL {plane}/{terrain_name}: {e}")

    # Cleanup
    del pipe
    gc.collect()
    torch.cuda.empty_cache()

    # Write log
    with open(str(LOG_FILE), "w") as f:
        f.write("\n".join(log_lines))

    print(f"\n{'='*60}")
    print(f"TERRAIN GENERATION COMPLETE")
    print(f"Generated: {generated}, Skipped: {skipped}, Total: {generated+skipped}/{total}")
    print(f"Raw: {RAW_DIR}")
    print(f"Processed: {PROC_DIR}")
    print(f"QA: {QA_DIR}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
