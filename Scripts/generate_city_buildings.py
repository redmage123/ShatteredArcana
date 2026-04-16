"""
Generate city building art for the city panorama view.
For each building: construction state + completed state.
Uses SD Turbo for fast generation.
"""

import torch
from diffusers import AutoPipelineForText2Image, AutoPipelineForImage2Image
from PIL import Image, ImageEnhance, ImageFilter
import os
import gc

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\buildings_cityview"

BUILDINGS = {
    # Building name: (construction prompt addition, completed prompt addition)
    "barracks": ("wooden military training yard under construction, scaffolding, lumber",
                 "medieval stone barracks with training dummies and weapon racks, military building"),
    "granary": ("grain storage building under construction, half-built stone walls, wooden beams",
                "stone granary with grain silos, stacked wheat barrels, thatched roof"),
    "smithy": ("blacksmith forge being built, anvil, brick chimney half-done",
               "medieval blacksmith forge, glowing furnace, anvils, hanging tools, smoking chimney"),
    "marketplace": ("market stalls being assembled, wooden frames, fabric awnings",
                    "bustling medieval marketplace, colorful stalls, awnings, crates of goods"),
    "library": ("stone library building under construction, columns half-erected",
                "grand stone library with tall windows, columns, scrollwork decorations"),
    "temple": ("temple foundations and walls under construction, altar stone placed",
               "ornate stone temple with stained glass, bell tower, religious symbols"),
    "cathedral": ("massive cathedral under construction, flying buttresses, scaffolding",
                  "grand gothic cathedral with rose window, twin spires, gargoyles"),
    "walls_wood": ("wooden palisade fence being erected, log posts, workers",
                   "completed wooden palisade wall with watchtower, gate, sharpened stakes"),
    "walls_stone": ("stone wall foundation being laid, cut stones, mortar",
                    "tall stone fortress wall with battlements, arrow slits, iron gate"),
    "mage_tower": ("wizard tower under construction, magical scaffolding, glowing runes",
                   "tall wizard tower with glowing crystal at top, arcane symbols, mystical aura"),
    "palace": ("grand palace foundation, massive columns being erected, marble blocks",
               "opulent royal palace with golden domes, banners, grand staircase, throne visible"),
    "stable": ("horse stable being built, wooden beams, hay bales",
               "medieval horse stable with horses, hay, saddles, fenced paddock"),
    "tavern": ("tavern building under construction, wooden frame, stone fireplace base",
               "cozy medieval tavern with hanging sign, warm light from windows, chimney smoke"),
    "docks": ("wooden dock pilings being hammered into water, half-built pier",
              "medieval harbor docks with moored ships, crane, warehouse, fishing nets"),
    "shipyard": ("shipyard scaffolding, ship hull on dry dock, timber stacks",
                 "active medieval shipyard, ship under sail, dry docks, maritime tools"),
    "war_college": ("military academy under construction, stone walls, training yard",
                    "grand military war college, flags, parade ground, armored statues"),
    "wizard_guild": ("wizard guild building under construction, magical wards, glowing foundation",
                     "mystical wizard guild hall, floating runes, crystal windows, arcane architecture"),
}

def main():
    print("=== Loading SD Turbo ===")
    pipe_t2i = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe_t2i = pipe_t2i.to("cuda")
    pipe_t2i.enable_attention_slicing()
    pipe_i2i = AutoPipelineForImage2Image.from_pipe(pipe_t2i)

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    base_style = "fantasy medieval building, isometric view, detailed illustration, " \
                 "game art asset, clean background, bright colors, digital painting"

    total = len(BUILDINGS) * 2
    count = 0

    for name, (construct_desc, complete_desc) in BUILDINGS.items():
        for state, desc in [("construction", construct_desc), ("complete", complete_desc)]:
            count += 1
            print(f"\n  [{count}/{total}] {name} ({state})...")

            prompt = f"{desc}, {base_style}"

            # Generate base
            base = pipe_t2i(
                prompt=prompt, num_inference_steps=4,
                guidance_scale=0.0, width=512, height=512,
            ).images[0]

            # Detail pass
            base_2x = base.resize((1024, 1024), Image.LANCZOS)
            hires = pipe_i2i(
                prompt=prompt, image=base_2x,
                num_inference_steps=4, guidance_scale=0.0,
                strength=0.3, width=1024, height=1024,
            ).images[0]

            # Enhance
            hires = ImageEnhance.Color(hires).enhance(1.15)
            hires = ImageEnhance.Sharpness(hires).enhance(1.2)

            # Save at 512x512 (game asset size)
            final = hires.resize((512, 512), Image.LANCZOS)
            path = os.path.join(OUTPUT_DIR, f"{name}_{state}.png")
            final.save(path, "PNG", optimize=True)
            print(f"    Saved: {path}")

    # Generate city backgrounds for different planes
    print("\n=== Generating city panorama backgrounds ===")
    bg_prompts = {
        "aurelith": "medieval fantasy city panorama, golden sunlight, green hills, blue sky, stone buildings, bright and hopeful",
        "noctharion": "dark gothic fantasy city panorama, moonlit, shadowy spires, purple sky, eerie atmosphere",
        "verdantis": "forest elven city panorama, tree houses, lush green canopy, crystal streams, nature magic",
        "infernyx": "volcanic dark city panorama, lava rivers, obsidian towers, red glowing sky, harsh",
        "abyssal": "demonic fortress city panorama, bone architecture, hellfire, dark red atmosphere",
    }

    for plane, prompt in bg_prompts.items():
        print(f"\n  City background: {plane}...")
        full_prompt = f"{prompt}, wide panoramic view, fantasy game art, detailed digital painting"

        base = pipe_t2i(
            prompt=full_prompt, num_inference_steps=4,
            guidance_scale=0.0, width=512, height=512,
        ).images[0]

        # Upscale to panoramic
        base_wide = base.resize((1024, 512), Image.LANCZOS)
        hires = pipe_i2i(
            prompt=full_prompt, image=base_wide,
            num_inference_steps=4, guidance_scale=0.0,
            strength=0.35, width=1024, height=512,
        ).images[0]

        hires = ImageEnhance.Color(hires).enhance(1.15)
        final = hires.resize((960, 480), Image.LANCZOS)
        path = os.path.join(OUTPUT_DIR, f"city_bg_{plane}.png")
        final.save(path, "PNG", optimize=True)
        print(f"    Saved: {path}")

    del pipe_t2i, pipe_i2i
    gc.collect()
    torch.cuda.empty_cache()
    print(f"\n=== Done — {count} building sprites + {len(bg_prompts)} backgrounds ===")


if __name__ == "__main__":
    main()
