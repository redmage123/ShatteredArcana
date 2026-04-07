#!/usr/bin/env python3
"""
Phase 5: Generate unit sprites for all 42 races × 3 unit types = 126 sprites.
Phase 6: Generate hero portraits for 23 classes × 2 genders = 46 portraits.
Uses SDXL for sprites, can swap to JuggernautXL for portraits if available.
"""

import torch
import gc
import os
import sys
from pathlib import Path
from PIL import Image
import numpy as np

BASE_DIR = Path("/home/bbrelin/ShatteredArcana/Art/GameAssets")

# ─────────────────────────────────────────────────────────────────────────────
# RACES — grouped by plane
# ─────────────────────────────────────────────────────────────────────────────

RACES = {
    # Aurelith races
    "high_men":       ("Aurelith", "noble human knights in golden plate armor, fair skin, regal bearing"),
    "high_elves":     ("Aurelith", "elegant elven warriors in silvery armor, pointed ears, tall and graceful"),
    "dwarves":        ("Aurelith", "stout dwarven warriors in heavy iron armor, thick beards, battle axes"),
    "halflings":      ("Aurelith", "small halfling scouts, light leather armor, nimble and quick"),
    "gnomes":         ("Aurelith", "clever gnomish tinkers, goggles, gadgets, small stature"),
    "draconians":     ("Aurelith", "dragonborn warriors, scaled skin, draconic features, fierce"),
    # Noctharion races
    "dark_elves":     ("Noctharion", "dark elf assassins, obsidian armor, white hair, purple skin"),
    "demonkin":       ("Noctharion", "demon-blooded warriors, horns, dark red skin, infernal armor"),
    "shadow_folk":    ("Noctharion", "shadow creatures, partially transparent, dark flowing forms"),
    "vampires":       ("Noctharion", "vampire lords, pale aristocratic, crimson cape, fangs"),
    "liches":         ("Noctharion", "undead lich mages, skeletal, glowing eyes, dark robes"),
    # Verdantis races
    "lizardmen":      ("Verdantis", "reptilian lizardfolk warriors, scaled green skin, tribal armor"),
    "dryads":         ("Verdantis", "tree spirit dryads, bark skin, leaf hair, nature magic"),
    "beastmen":       ("Verdantis", "beast-hybrid warriors, fur and claws, primal rage"),
    "centaurs":       ("Verdantis", "centaur warriors, horse body, human torso, bow and spear"),
    "treants":        ("Verdantis", "living tree warriors, massive bark-covered, ancient"),
    # Infernyx races
    "demons":         ("Infernyx", "demonic warriors, red skin, horns, flaming weapons"),
    "efreeti":        ("Infernyx", "fire genie efreet, blazing skin, smoke and flame"),
    "ashen":          ("Infernyx", "ash-covered warriors, grey skin, ember eyes, volcanic"),
    "iron_devils":    ("Infernyx", "iron-clad devil soldiers, chain armor, barbed weapons"),
    "fire_elementals":("Infernyx", "pure fire elemental beings, living flame, no solid form"),
    # Aethermist races
    "crystal_elves":  ("Aethermist", "crystalline elf warriors, prismatic armor, translucent skin"),
    "celestials":     ("Aethermist", "angelic celestial beings, wings, golden halos, divine light"),
    "echomancers":    ("Aethermist", "sound-wielding mages, vibrating aura, sonic weapons"),
    "ether_weavers":  ("Aethermist", "ethereal spirit weavers, ghostly robes, magic threads"),
    "star_born":      ("Aethermist", "stellar beings, constellation patterns on skin, cosmic"),
    # Abyssal races
    "abyssal_terrors":("Abyssal", "deep sea horror creatures, tentacles, bioluminescent, teeth"),
    "merfolk":        ("Abyssal", "mer-people warriors, fish tails, coral armor, trident"),
    "chithari":       ("Abyssal", "insectoid hive warriors, chitinous armor, mandibles"),
    "kraken_spawn":   ("Abyssal", "octopoid creatures, multiple arms, ink cloud"),
    "deep_ones":      ("Abyssal", "ancient deep sea humanoids, fishy features, barnacles"),
    # Ethereal races
    "dream_weavers":  ("Ethereal", "dream-realm beings, shifting translucent forms, surreal"),
    "phase_stalkers": ("Ethereal", "phase-shifting predators, flickering between planes"),
    "thought_forms":  ("Ethereal", "pure psychic energy beings, crystallized thoughts"),
    "void_touched":   ("Ethereal", "void-corrupted humanoids, reality-warping aura"),
    "astral_nomads":  ("Ethereal", "astral traveler nomads, silver cords, floating"),
    # Feywild races
    "fey_elves":      ("Feywild", "fey elf warriors, flower-petal armor, butterfly wings"),
    "pixies":         ("Feywild", "tiny pixie sprites, dragonfly wings, glowing dust"),
    "erlking_guard":  ("Feywild", "wild hunt guard, antler helms, green and brown"),
    "satyrs":         ("Feywild", "goat-legged satyr warriors, pipes, revelry"),
    "spriggans":      ("Feywild", "spriggan earth-fey, bark and stone, shapeshifting"),
    "djinn":          ("Feywild", "air djinn genie, swirling wind form, blue mist"),
}

UNIT_TYPES = {
    "infantry": "foot soldier, sword and shield, standing ready, armored",
    "ranged":   "archer or ranged fighter, bow or staff, aiming stance",
    "cavalry":  "mounted warrior on beast or horse, charging pose, lance",
}

HERO_CLASSES = {
    "fighter":        ("male", "heavily armored warrior, greatsword, battle-scarred, determined"),
    "fighter_f":      ("female", "female warrior in plate armor, longsword, fierce expression"),
    "bowman":         ("male", "expert archer, leather armor, quiver of arrows, keen eyes"),
    "bowman_f":       ("female", "female ranger, hooded cloak, bow drawn, forest background"),
    "cleric":         ("male", "holy cleric, white and gold robes, healing staff, gentle face"),
    "cleric_f":       ("female", "priestess, ornate vestments, holy symbol, compassionate"),
    "magician":       ("male", "wizard, long robes, staff with crystal, long beard, wise"),
    "magician_f":     ("female", "sorceress, flowing magical robes, arcane energy, elegant"),
    "paladin":        ("male", "holy paladin, shining full plate, holy sword, noble"),
    "paladin_f":      ("female", "female paladin, gleaming armor, shield with cross, righteous"),
    "assassin":       ("male", "dark assassin, black leather, twin daggers, hooded"),
    "assassin_f":     ("female", "female assassin, sleek dark outfit, poison blade, shadows"),
    "bard":           ("male", "charismatic bard, lute, colorful clothes, charming smile"),
    "bard_f":         ("female", "female bard, harp, elegant dress, enchanting voice"),
    "necromancer":    ("male", "dark necromancer, skull staff, black robes, green death energy"),
    "necromancer_f":  ("female", "female necromancer, bone crown, dark magic, pale skin"),
    "druid":          ("male", "nature druid, animal pelts, wooden staff, wild hair"),
    "druid_f":        ("female", "female druid, vine crown, nature magic, serene"),
    "psyker":         ("male", "psychic warrior, glowing eyes, telekinetic aura, intense"),
    "psyker_f":       ("female", "female psychic, third eye, mental energy, focused"),
    "warlock":        ("male", "dark warlock, demonic pact symbol, hellfire hands, sinister"),
    "warlock_f":      ("female", "female warlock, eldritch patron mark, dark power, dangerous"),
    "dragon_knight":  ("male", "dragon rider knight, dragon scale armor, lance, majestic"),
    "dragon_knight_f":("female", "female dragon knight, dragon bond, scaled gauntlets, fierce"),
    "warlord":        ("male", "commanding warlord, war banner, heavy armor, battle commander"),
    "warlord_f":      ("female", "female warlord, tactical genius, ornate armor, authority"),
}

PLANE_STYLES = {
    "Aurelith":   "golden radiant lighting, warm amber tones",
    "Noctharion": "dark arcane atmosphere, deep purple shadows",
    "Verdantis":  "lush verdant green, bioluminescent",
    "Infernyx":   "volcanic red-orange, molten glow",
    "Aethermist": "ethereal crystal blue, floating particles",
    "Abyssal":    "deep ocean teal, bioluminescent cyan",
    "Ethereal":   "ghostly translucent, pale green-white",
    "Feywild":    "vibrant shifting colors, fey glamour",
}

NEGATIVE_SPRITE = (
    "text, watermark, background clutter, realistic photo, blurry, "
    "side view, landscape, multiple figures, border, frame"
)

NEGATIVE_PORTRAIT = (
    "deformed, bad anatomy, extra limbs, blurry eyes, text, watermark, "
    "cropped, low quality, cartoon, chibi, full body, landscape"
)


def remove_bg(img, threshold=25):
    arr = np.array(img.convert("RGBA"))
    corners = [arr[0,0,:3], arr[0,-1,:3], arr[-1,0,:3], arr[-1,-1,:3]]
    bg = np.mean(corners, axis=0).astype(np.uint8)
    diff = np.abs(arr[:,:,:3].astype(int) - bg.astype(int)).sum(axis=2)
    arr[:,:,3] = np.where(diff > threshold * 3, 255, 0).astype(np.uint8)
    return Image.fromarray(arr)


def main():
    from diffusers import StableDiffusionXLPipeline

    phases = sys.argv[1:] if len(sys.argv) > 1 else ["units", "heroes"]

    pipe = StableDiffusionXLPipeline.from_pretrained(
        "stabilityai/stable-diffusion-xl-base-1.0",
        torch_dtype=torch.float16, variant="fp16", use_safetensors=True,
    ).to("cuda")
    pipe.enable_attention_slicing()

    if "units" in phases:
        print(f"\n{'='*60}\nPHASE 5: UNIT SPRITES ({len(RACES)} races × {len(UNIT_TYPES)} types = {len(RACES)*len(UNIT_TYPES)})\n{'='*60}")

        total = len(RACES) * len(UNIT_TYPES)
        done = 0

        for race_idx, (race_name, (plane, race_desc)) in enumerate(RACES.items()):
            race_dir_raw = BASE_DIR / "raw" / "units" / race_name
            race_dir_proc = BASE_DIR / "processed" / "units" / race_name
            race_dir_raw.mkdir(parents=True, exist_ok=True)
            race_dir_proc.mkdir(parents=True, exist_ok=True)

            plane_style = PLANE_STYLES.get(plane, "")

            for unit_idx, (unit_type, unit_desc) in enumerate(UNIT_TYPES.items()):
                proc_path = race_dir_proc / f"{unit_type}.png"
                if proc_path.exists():
                    done += 1
                    continue

                raw_path = race_dir_raw / f"{unit_type}.png"
                prompt = (
                    f"fantasy unit sprite, {race_name.replace('_', ' ')} {unit_type}, "
                    f"top-down 3/4 view, {race_desc}, {unit_desc}, "
                    f"{plane_style}, pixel art style, transparent background, "
                    f"game character sprite, single figure, action pose"
                )

                seed = 5000 + race_idx * 10 + unit_idx
                gen = torch.Generator("cuda").manual_seed(seed)

                try:
                    result = pipe(prompt=prompt, negative_prompt=NEGATIVE_SPRITE,
                                  num_inference_steps=15, guidance_scale=8.0,
                                  width=1024, height=1024, generator=gen).images[0]
                    result.save(str(raw_path))
                    processed = remove_bg(result)
                    processed = processed.resize((64, 64), Image.LANCZOS)
                    processed.save(str(proc_path))
                    done += 1
                    print(f"  [{done}/{total}] {race_name}/{unit_type} ✓")
                except Exception as e:
                    print(f"  FAIL {race_name}/{unit_type}: {e}")

    if "heroes" in phases:
        print(f"\n{'='*60}\nPHASE 6: HERO PORTRAITS ({len(HERO_CLASSES)} portraits)\n{'='*60}")

        hero_raw = BASE_DIR / "raw" / "heroes"
        hero_proc = BASE_DIR / "processed" / "heroes"
        hero_raw.mkdir(parents=True, exist_ok=True)
        hero_proc.mkdir(parents=True, exist_ok=True)

        done = 0
        total = len(HERO_CLASSES)

        for idx, (class_key, (gender, class_desc)) in enumerate(HERO_CLASSES.items()):
            proc_path = hero_proc / f"{class_key}.png"
            if proc_path.exists():
                done += 1
                continue

            raw_path = hero_raw / f"{class_key}.png"
            prompt = (
                f"fantasy hero portrait, {class_desc}, {gender}, bust portrait, "
                f"detailed face, dramatic lighting, RPG character portrait, "
                f"painterly style, high detail face, ornate details"
            )

            seed = 6000 + idx
            gen = torch.Generator("cuda").manual_seed(seed)

            try:
                result = pipe(prompt=prompt, negative_prompt=NEGATIVE_PORTRAIT,
                              num_inference_steps=25, guidance_scale=8.0,
                              width=1024, height=1024, generator=gen).images[0]
                result.save(str(raw_path))
                # Center crop to square, resize to 256x256
                w, h = result.size
                crop_size = min(w, h)
                left = (w - crop_size) // 2
                top = (h - crop_size) // 2
                cropped = result.crop((left, top, left + crop_size, top + crop_size))
                cropped = cropped.resize((256, 256), Image.LANCZOS)
                cropped.save(str(proc_path))
                done += 1
                print(f"  [{done}/{total}] {class_key} ✓")
            except Exception as e:
                print(f"  FAIL {class_key}: {e}")

    del pipe
    gc.collect()
    torch.cuda.empty_cache()
    print(f"\n{'='*60}\nUNIT/HERO GENERATION COMPLETE\n{'='*60}")


if __name__ == "__main__":
    main()
