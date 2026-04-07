#!/usr/bin/env python3
"""
Generate icons for all 600 spells from the spell catalogue.
Uses realm-specific color coding and visual descriptions from each spell's desc field.
~80 minutes at 8 sec/icon.
Supports resume — skips already-generated icons.
"""

import torch
import gc
import os
import sys
from pathlib import Path
from PIL import Image
import numpy as np

BASE_DIR = Path("/home/bbrelin/ShatteredArcana/Art/GameAssets")

# Import spell catalogue
sys.path.insert(0, str(BASE_DIR))
from shattered_arcana_spells import SPELLS

REALM_COLORS = {
    "arcane":   "deep blue arcane energy",
    "nature":   "emerald green natural energy",
    "death":    "dark purple necrotic energy",
    "life":     "radiant white golden holy energy",
    "chaos":    "crimson red chaotic energy",
    "sorcery":  "golden amber mystical energy",
    "magma":    "orange molten fiery energy",
    "glamour":  "pink-violet fey enchanting energy",
    "binding":  "dark iron silver chain energy",
    "spirit":   "pale ghostly blue-white spectral energy",
}

NEGATIVE = (
    "text, watermark, realistic photo, character, person, landscape, "
    "complex scene, blurry, low quality, border, multiple objects"
)


def remove_bg(img, threshold=30):
    arr = np.array(img.convert("RGBA"))
    h, w = arr.shape[:2]
    cs = max(h, w) // 8
    corners = []
    for cy, cx in [(0,0),(0,w-cs),(h-cs,0),(h-cs,w-cs)]:
        corners.append(arr[cy:cy+cs, cx:cx+cs, :3].reshape(-1,3).mean(axis=0))
    bg = np.mean(corners, axis=0).astype(np.float32)
    diff = np.sqrt(((arr[:,:,:3].astype(np.float32) - bg)**2).sum(axis=2))
    thresh = max(30, np.percentile(diff, 20))
    arr[:,:,3] = np.where(diff > thresh, 255, 0).astype(np.uint8)
    return Image.fromarray(arr)


def main():
    from diffusers import StableDiffusionXLPipeline

    # Allow filtering by realm
    filter_realms = sys.argv[1:] if len(sys.argv) > 1 else list(SPELLS.keys())
    filter_realms = [r for r in filter_realms if r in SPELLS]

    # Count total
    total = sum(len(spells) for realm in filter_realms for spells in SPELLS[realm].values())
    print(f"Generating {total} spell icons for realms: {', '.join(filter_realms)}")

    pipe = StableDiffusionXLPipeline.from_pretrained(
        "stabilityai/stable-diffusion-xl-base-1.0",
        torch_dtype=torch.float16, variant="fp16", use_safetensors=True,
    ).to("cuda")
    pipe.enable_attention_slicing()

    done = 0
    skipped = 0
    failed = 0
    seed_counter = 10000

    for realm in filter_realms:
        realm_color = REALM_COLORS.get(realm, "magical energy")
        tiers = SPELLS[realm]

        print(f"\n{'='*60}")
        print(f"REALM: {realm.upper()} ({realm_color})")
        print(f"{'='*60}")

        for tier_name, spell_list in tiers.items():
            tier_num = int(tier_name.split("_")[1])
            print(f"\n  --- {tier_name} ({len(spell_list)} spells) ---")

            for spell in spell_list:
                spell_id = spell["id"]
                desc = spell["desc"]

                raw_dir = BASE_DIR / "raw" / "spells" / realm
                proc_dir = BASE_DIR / "processed" / "spells" / realm
                raw_dir.mkdir(parents=True, exist_ok=True)
                proc_dir.mkdir(parents=True, exist_ok=True)

                proc_path = proc_dir / f"{spell_id}.png"

                if proc_path.exists():
                    skipped += 1
                    continue

                raw_path = raw_dir / f"{spell_id}.png"

                prompt = (
                    f"magic spell icon, {desc}, {realm_color} glow, "
                    f"circular frame, fantasy game spellbook icon, "
                    f"dark background, centered magical effect, "
                    f"RPG ability icon, detailed, vibrant colors"
                )

                seed_counter += 1
                gen = torch.Generator("cuda").manual_seed(seed_counter)

                try:
                    result = pipe(
                        prompt=prompt,
                        negative_prompt=NEGATIVE,
                        num_inference_steps=15,
                        guidance_scale=9.0,
                        width=1024,
                        height=1024,
                        generator=gen,
                    ).images[0]

                    result.save(str(raw_path))
                    processed = remove_bg(result)
                    processed = processed.resize((48, 48), Image.LANCZOS)
                    processed.save(str(proc_path))

                    done += 1
                    progress = done + skipped
                    print(f"    [{progress}/{total}] {spell_id} ✓")

                except Exception as e:
                    failed += 1
                    print(f"    FAIL {spell_id}: {e}")

    del pipe
    gc.collect()
    torch.cuda.empty_cache()

    print(f"\n{'='*60}")
    print(f"SPELL ICON GENERATION COMPLETE")
    print(f"Generated: {done}, Skipped: {skipped}, Failed: {failed}")
    print(f"Total: {done + skipped}/{total}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
