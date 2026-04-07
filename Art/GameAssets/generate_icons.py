#!/usr/bin/env python3
"""
Phases 2-4: Generate map feature icons, building icons, and spell icons.
Uses SDXL with icon-optimized prompts.
~140 icons total.
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
# MAP FEATURE ICONS (Phase 2)
# ─────────────────────────────────────────────────────────────────────────────

MAP_FEATURES = {
    "ley_line_node":    "glowing magical ley line nexus point, swirling arcane energy",
    "portal_ley":       "shimmering ley portal gateway, blue-white energy ring",
    "portal_planar":    "planar gate between dimensions, crackling reality tear",
    "portal_wild":      "wild unstable portal, chaotic multicolor energy vortex",
    "mine_gold":        "gold mine entrance with glittering ore, pickaxe and cart",
    "mine_mithril":     "mithril mine entrance, silvery blue gleaming ore",
    "mine_adamantine":  "adamantine mine, dark metallic ore with purple sheen",
    "mine_crystal":     "crystal mine, prismatic gemstone deposits, rainbow light",
    "ruin_ancient":     "crumbling ancient ruins, overgrown stone pillars",
    "ruin_tower":       "ruined wizard tower, broken spire, magical residue",
    "dungeon_entrance": "dark dungeon entrance in hillside, iron gate, torch",
    "explorable_shrine":"small magical shrine, glowing altar, prayer stones",
    "explorable_grove": "enchanted grove, ancient tree with glowing leaves",
    "resource_forest":  "managed timber forest, stacked logs, sawmill",
    "resource_farm":    "fertile farmland, golden wheat fields, barn",
    "resource_quarry":  "stone quarry, carved rock face, stone blocks",
    "city_small":       "small village, few houses, wooden fence",
    "city_medium":      "medium town, stone buildings, market square",
    "city_large":       "large city, castle walls, cathedral spire",
    "army_marker":      "military banner on pole, crossed swords emblem",
    "fleet_marker":     "sailing ship flag, anchor and wave emblem",
    "hero_marker":      "heroic figure silhouette, glowing aura, star",
    "dragon_lair":      "dragon cave entrance, scorched earth, treasure glint",
    "fortress":         "fortified stronghold, thick walls, watchtower",
    "wizard_tower":     "tall wizard tower, glowing windows, arcane symbols",
}

# ─────────────────────────────────────────────────────────────────────────────
# BUILDING ICONS (Phase 3)
# ─────────────────────────────────────────────────────────────────────────────

BUILDINGS = {
    "granary":              "stone granary building, wheat storage, barrel",
    "barracks":             "military barracks, soldiers quarters, armor rack",
    "stable":               "horse stable, wooden barn, hay bales",
    "smithy":               "blacksmith forge, anvil, glowing furnace, hammer",
    "library":              "grand library, bookshelves, scrolls, candles",
    "shrine":               "small religious shrine, prayer altar, incense",
    "temple":               "ornate temple, domed roof, religious symbols",
    "marketplace":          "bustling marketplace, merchant stalls, goods",
    "walls_wood":           "wooden palisade wall section, sharpened logs",
    "walls_stone":          "stone castle wall section, crenellations",
    "walls_iron":           "iron-reinforced fortress wall, metal plates",
    "wizard_guild":         "wizard guild hall, arcane symbols, magical aura",
    "fighters_guild":       "fighters guild, crossed swords sign, training dummy",
    "thieves_guild":        "shadowy thieves guild, hidden door, lockpicks",
    "shipyard":             "coastal shipyard, ship frame, crane, dock",
    "docks":                "harbor docks, moored ships, cargo crates",
    "alchemist_lab":        "alchemist laboratory, bubbling potions, herbs",
    "oracle":               "oracle chamber, crystal ball, mystical fog",
    "summoning_circle":     "magical summoning circle, rune-inscribed floor",
    "war_college":          "military war college, tactical maps, banners",
    "mage_tower":           "mage tower, glowing spire, floating books",
    "cathedral":            "grand cathedral, stained glass, tall spires",
    "palace":               "royal palace, golden domes, grand stairs",
    "colosseum":            "gladiatorial colosseum, arena, spectator seats",
    "armory":               "weapon armory, swords shields armor on racks",
    "aqueduct":             "stone aqueduct, flowing water channel, arches",
    "bank":                 "treasury bank, gold vault, secure door, coins",
    "tavern":               "lively tavern, hanging sign, warm light, mugs",
    "memorial":             "stone memorial monument, carved names, wreath",
    "lighthouse":           "coastal lighthouse, beacon light, rocky shore",
    "observatory":          "astronomical observatory, telescope dome, stars",
    "enchanter_workshop":   "enchanter workshop, glowing runes, magic anvil",
    "dragon_roost":         "dragon nesting roost, large stone perch, fire marks",
    "planar_beacon":        "planar beacon tower, interdimensional energy beam",
}

# ─────────────────────────────────────────────────────────────────────────────
# SPELL ICONS (Phase 4)
# ─────────────────────────────────────────────────────────────────────────────

REALM_COLORS = {
    "arcane":   "deep blue arcane",
    "nature":   "emerald green natural",
    "death":    "dark purple necrotic",
    "life":     "radiant white holy golden",
    "chaos":    "crimson red chaotic",
    "sorcery":  "golden amber mystical",
    "magma":    "orange molten fiery",
    "glamour":  "pink-violet fey enchanting",
    "binding":  "dark iron chain silver",
    "spirit":   "pale ghostly blue-white",
}

SPELLS_BY_REALM = {
    "arcane": {
        "magic_bolt":     "arcane bolt of blue energy, magical projectile",
        "dispel_magic":   "dispelling wave, magic dissolving, rune breaking",
        "teleport":       "teleportation circle, spatial warp, flash",
        "arcane_shield":  "magical shield barrier, blue force field",
        "detect_magic":   "magical detection eye, scanning beam",
        "counterspell":   "spell interruption, clashing magical forces",
        "time_stop":      "frozen time, clock hands stopped, blue haze",
        "mana_drain":     "siphoning magical energy, blue energy flow",
    },
    "nature": {
        "entangle":       "grasping vines and roots, tangling growth",
        "call_lightning": "lightning bolt from storm clouds, electric strike",
        "heal":           "green healing light, nature restoration glow",
        "summon_beast":   "spirit animal emerging, nature summoning",
        "earthquake":     "cracking earth, seismic shockwave, splitting ground",
        "wall_of_thorns": "thorny barrier wall, sharp brambles",
        "regeneration":   "cellular regrowth, green healing pulse",
        "wildfire":       "spreading natural fire, burning grassland",
    },
    "death": {
        "raise_dead":     "skeletal hand rising from grave, necromantic glow",
        "death_ray":      "dark beam of death energy, skull projectile",
        "fear":           "terrifying spectral face, fear aura, darkness",
        "plague":         "sickly green cloud, disease spreading, rot",
        "soul_steal":     "ghostly spirit being extracted, soul energy",
        "animate_dead":   "skeleton army rising, purple energy",
        "wither":         "decay touch, aging effect, crumbling",
        "darkness":       "sphere of absolute darkness, light consuming",
    },
    "life": {
        "healing_word":   "spoken healing word, golden letters, warmth",
        "bless":          "divine blessing, halo effect, holy light",
        "resurrection":   "soul returning to body, white radiance",
        "holy_smite":     "divine smite, golden beam from heaven",
        "protection":     "protective golden aura, shield of faith",
        "turn_undead":    "holy light repelling undead, burning evil",
        "divine_favor":   "golden blessing aura, strengthening light",
        "sanctify":       "purifying holy ground, cleansing circle",
    },
    "chaos": {
        "fireball":       "explosive fireball, flame sphere, detonation",
        "chaos_bolt":     "unpredictable chaotic energy bolt, multicolor",
        "warp_reality":   "reality distortion, space bending, glitch",
        "doom":           "impending doom mark, red rune of death",
        "meteor_strike":  "flaming meteor falling from sky, impact crater",
        "corruption":     "chaotic corruption spreading, reality decay",
        "wild_surge":     "uncontrolled magic surge, random energy",
        "annihilate":     "total destruction sphere, disintegration",
    },
    "sorcery": {
        "enchant_weapon": "weapon being enchanted, golden rune glow",
        "clairvoyance":   "all-seeing eye, remote viewing, crystal ball",
        "mass_invisibility":"figures fading transparent, invisibility wave",
        "spell_lock":     "locked spell containment, sealed magic",
        "phantom_army":   "illusory soldiers, ghostly formation",
        "mind_control":   "psychic domination, glowing eyes, puppet strings",
        "flight":         "magical levitation, floating figure, wind",
        "haste":          "speed boost, blurred motion lines, clock",
    },
    "magma": {
        "lava_burst":     "erupting lava geyser, molten rock spray",
        "wall_of_fire":   "towering fire wall, infernal barrier",
        "magma_armor":    "armor of cooling lava, stone and fire",
        "volcanic_blast": "volcanic eruption explosion, ash cloud",
        "forge_weapon":   "weapon being forged in fire, hammer and anvil",
        "heat_metal":     "metal glowing red hot, thermal energy",
        "obsidian_spear": "obsidian volcanic glass spear, sharp and deadly",
        "caldera":        "opening volcanic caldera, magma pool forming",
    },
    "glamour": {
        "charm":          "enchanting charm effect, heart sparkles, allure",
        "illusion":       "mirror illusion, false reality, shimmer",
        "fairy_fire":     "faerie fire outline, colorful luminescent",
        "sleep":          "magical sleep dust, dreaming sparkles, moon",
        "polymorph":      "shape-shifting transformation, morphing body",
        "mirror_image":   "multiple duplicate images, reflections",
        "faerie_ring":    "mushroom fairy ring, magical boundary",
        "glamour_veil":   "beautiful concealing veil, enchanting mist",
    },
}

NEGATIVE_ICON = (
    "text, watermark, realistic photo, blurry, low contrast, "
    "complex background, gradient background, multiple objects, "
    "photograph, 3d render, landscape"
)

NEGATIVE_SPELL = (
    "text, watermark, realistic photo, character, landscape, "
    "complex scene, blurry, low quality, frame border"
)


def remove_background(img: Image.Image, threshold: int = 20) -> Image.Image:
    """Simple background removal for icon-style images."""
    arr = np.array(img.convert("RGBA"))
    # Detect near-uniform corners as background color
    corners = [arr[0,0,:3], arr[0,-1,:3], arr[-1,0,:3], arr[-1,-1,:3]]
    bg_color = np.mean(corners, axis=0).astype(np.uint8)
    # Create alpha mask
    diff = np.abs(arr[:,:,:3].astype(int) - bg_color.astype(int)).sum(axis=2)
    alpha = np.where(diff > threshold * 3, 255, 0).astype(np.uint8)
    arr[:,:,3] = alpha
    return Image.fromarray(arr)


def generate_category(pipe, category_name, items, output_size, prompt_template,
                      negative, raw_dir, proc_dir, seed_offset=0):
    """Generate a category of icons."""
    total = len(items)
    generated = 0
    for idx, (name, description) in enumerate(items.items()):
        raw_path = raw_dir / f"{name}.png"
        proc_path = proc_dir / f"{name}.png"

        if proc_path.exists():
            continue

        prompt = prompt_template.format(description=description)
        seed = seed_offset + idx + 100
        generator = torch.Generator("cuda").manual_seed(seed)

        try:
            result = pipe(
                prompt=prompt,
                negative_prompt=negative,
                num_inference_steps=20,
                guidance_scale=8.5,
                width=1024,
                height=1024,
                generator=generator,
            ).images[0]

            result.save(str(raw_path))

            # Postprocess: remove background, resize
            processed = remove_background(result)
            processed = processed.resize((output_size, output_size), Image.LANCZOS)
            processed.save(str(proc_path))

            generated += 1
            print(f"  [{generated}/{total}] {category_name}/{name} ✓")

        except Exception as e:
            print(f"  [{generated}/{total}] {category_name}/{name} FAILED: {e}")

    return generated


def main():
    from diffusers import StableDiffusionXLPipeline

    phases = sys.argv[1:] if len(sys.argv) > 1 else ["map_features", "buildings", "spells"]

    print(f"Generating icons for phases: {phases}")

    pipe = StableDiffusionXLPipeline.from_pretrained(
        "stabilityai/stable-diffusion-xl-base-1.0",
        torch_dtype=torch.float16, variant="fp16", use_safetensors=True,
    ).to("cuda")
    pipe.enable_attention_slicing()

    if "map_features" in phases:
        print(f"\n{'='*60}\nPHASE 2: MAP FEATURE ICONS ({len(MAP_FEATURES)} icons)\n{'='*60}")
        raw = BASE_DIR / "raw" / "map_features"
        proc = BASE_DIR / "processed" / "map_features"
        template = "fantasy game map icon, {description}, isolated object, centered, clean outline, detailed miniature, RPG map marker style, transparent background, single object, top-down view"
        generate_category(pipe, "map_features", MAP_FEATURES, 48, template, NEGATIVE_ICON, raw, proc, 2000)

    if "buildings" in phases:
        print(f"\n{'='*60}\nPHASE 3: BUILDING ICONS ({len(BUILDINGS)} icons)\n{'='*60}")
        raw = BASE_DIR / "raw" / "buildings"
        proc = BASE_DIR / "processed" / "buildings"
        template = "fantasy building icon, {description}, golden stone aesthetic, isometric view, game UI icon style, detailed miniature architecture, warm lighting, RPG city builder style, single building, centered"
        generate_category(pipe, "buildings", BUILDINGS, 64, template, NEGATIVE_ICON, raw, proc, 3000)

    if "spells" in phases:
        print(f"\n{'='*60}\nPHASE 4: SPELL ICONS\n{'='*60}")
        total_spells = sum(len(s) for s in SPELLS_BY_REALM.values())
        print(f"Total: {total_spells} spells across {len(SPELLS_BY_REALM)} realms")
        seed_off = 4000
        for realm, spells in SPELLS_BY_REALM.items():
            realm_color = REALM_COLORS.get(realm, "magical")
            print(f"\n  Realm: {realm} ({len(spells)} spells, {realm_color})")
            raw = BASE_DIR / "raw" / "spells" / realm
            proc = BASE_DIR / "processed" / "spells" / realm
            template = f"magic spell icon, {{description}}, {realm_color} energy glow, circular frame, fantasy game spellbook icon, mystical particles, dark background, centered magical effect, RPG ability icon"
            generate_category(pipe, f"spells/{realm}", spells, 48, template, NEGATIVE_SPELL, raw, proc, seed_off)
            seed_off += 100

    del pipe
    gc.collect()
    torch.cuda.empty_cache()
    print(f"\n{'='*60}\nICON GENERATION COMPLETE\n{'='*60}")


if __name__ == "__main__":
    main()
