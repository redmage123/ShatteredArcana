"""
Generate art assets for all 91 buildings across 12 architecture styles.
Priority order:
  1. All 91 buildings in Human style (construction + complete) — baseline
  2. Universal buildings (46) in remaining 11 styles
  3. Racial buildings in their native style

Uses SD Turbo with img2img detail pass for quality.
"""

import torch
from diffusers import AutoPipelineForText2Image, AutoPipelineForImage2Image
from PIL import Image, ImageEnhance, ImageFilter
import os
import gc
import json

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\buildings_cityview"

# Architecture style descriptions for prompts
ARCH_STYLES = {
    "Human":       "medieval European stone castle with timber frames, thatched roofs, cobblestone",
    "Elven":       "elegant elven architecture with organic curves, living wood, crystal spires, flowing lines",
    "Dwarven":     "dwarven stone hall with geometric patterns, carved pillars, underground vault style",
    "Orcish":      "crude orcish construction with rough wood, animal bones, war trophies, hide tents",
    "Dark":        "gothic dark architecture with obsidian spires, shadow-wreathed, gargoyles, iron spikes",
    "Draconic":    "draconic volcanic stone with dragon motifs, lava channels, scaled roof tiles",
    "Demonic":     "demonic iron fortress with hellfire, chains, burning braziers, skull decorations",
    "Aquatic":     "aquatic coral and shell architecture, flowing water features, pearl decorations",
    "Fey":         "fey mushroom cap roofs, crystal springs, living flowers, bioluminescent",
    "Ethereal":    "ethereal translucent architecture, phase-shifted, echo patterns, ghostly glow",
    "Abyssal":     "abyssal bone and flesh architecture, organic growths, pulsing veins, eye motifs",
    "Crystalline": "crystalline prismatic architecture, crystal formations, rainbow refractions, geometric",
}

# All 46 universal buildings
UNIVERSAL_BUILDINGS = [
    ("Granary", "grain storage building with silos and wheat barrels"),
    ("Marketplace", "bustling market with colorful stalls and merchant carts"),
    ("Smithy", "blacksmith forge with glowing furnace and anvils"),
    ("Library", "grand library with tall bookshelves and reading desks"),
    ("Temple", "ornate temple with religious symbols and bell tower"),
    ("Shrine", "small sacred shrine with altar and offerings"),
    ("Barracks", "military barracks with training dummies and weapon racks"),
    ("Tavern", "cozy tavern with hanging sign and chimney smoke"),
    ("Docks", "harbor docks with moored ships and fishing nets"),
    ("WallsWood", "wooden palisade wall with watchtower and gate"),
    ("Memorial", "stone memorial monument with carved inscriptions"),
    ("Stable", "horse stable with paddock and hay bales"),
    ("Aqueduct", "stone aqueduct carrying water with arched bridges"),
    ("Bank", "ornate bank building with vault and gold symbols"),
    ("MageTower", "tall wizard tower with glowing crystal at top"),
    ("Cathedral", "grand gothic cathedral with rose window and spires"),
    ("Colosseum", "large arena colosseum with tiered seating"),
    ("Shipyard", "active shipyard with dry docks and ship under construction"),
    ("Lighthouse", "tall lighthouse on rocky coast with beacon light"),
    ("WallsStone", "tall stone fortress wall with battlements and arrow slits"),
    ("Observatory", "domed observatory tower with telescope"),
    ("ThievesGuild", "shadowy thieves guild hidden in alleyway"),
    ("Armory", "military armory filled with weapons and armor on racks"),
    ("Oracle", "mystical oracle temple with glowing pool and incense"),
    ("WallsIron", "massive iron-reinforced fortress wall with heavy gate"),
    ("EnchanterWorkshop", "enchanter workshop with glowing runes and magical tools"),
    ("SummoningCircle", "magical summoning circle with arcane symbols on floor"),
    ("AlchemistLab", "alchemist laboratory with bubbling potions and distillery"),
    ("FightersGuild", "elite fighters guild with trophy weapons and training ring"),
    ("WarCollege", "grand military academy with parade ground and flags"),
    ("WizardGuild", "mystical wizard guild hall with floating runes"),
    ("DragonRoost", "massive dragon roost with perch and dragon nest"),
    ("Palace", "opulent royal palace with golden domes and grand staircase"),
    ("PlanarBeacon", "magical planar beacon tower emitting cross-dimensional light"),
    ("Farm", "fertile farm with crop fields and scarecrow"),
    ("Plantation", "large plantation with organized crop rows and workers"),
    ("Sawmill", "sawmill with water wheel and stacked lumber"),
    ("Mine", "mine entrance in hillside with ore cart and tracks"),
    ("Quarry", "open stone quarry with cut blocks and cranes"),
    ("ArcheryRange", "archery range with targets and training bows"),
    ("SiegeWorkshop", "siege workshop with catapult under construction"),
    ("Amphitheater", "open-air amphitheater with curved stone seating"),
    ("University", "grand university building with columns and scholars"),
    ("Hospital", "hospital building with healing herbs and beds visible"),
    ("ManaVault", "magical mana vault with glowing crystal containers"),
    ("RuneForge", "rune inscription forge with glowing carved stones"),
]

# Racial buildings with their native architecture style
RACIAL_BUILDINGS = [
    ("KnightsChapterHouse", "knight chapter house with heraldic banners", "Human"),
    ("RoyalCourt", "royal court with throne and courtiers", "Human"),
    ("CathedralOfLight", "radiant cathedral of holy light", "Human"),
    ("StarlightSpire", "tall starlight spire with crystal peak", "Elven"),
    ("Moonwell", "glowing moonwell pool with silver water", "Elven"),
    ("AncientGrove", "ancient magical grove with living trees", "Elven"),
    ("AdamantineForge", "massive adamantine forge deep underground", "Dwarven"),
    ("DeepMineComplex", "extensive deep mine complex with ore veins", "Dwarven"),
    ("RunesmithHall", "dwarven runesmith hall with glowing rune tables", "Dwarven"),
    ("DragonHatchery", "volcanic dragon hatchery with eggs in lava", "Draconic"),
    ("VolcanicForge", "volcanic forge built over lava flow", "Draconic"),
    ("SkyRoost", "elevated sky roost platform for flying creatures", "Draconic"),
    ("ShadowAcademy", "dark shadow academy shrouded in darkness", "Dark"),
    ("SacrificialAltar", "sinister sacrificial altar with dark magic", "Dark"),
    ("WebCitadel", "web-covered citadel with spider motifs", "Dark"),
    ("HellFirePit", "hellfire pit burning with demonic flames", "Demonic"),
    ("SoulForge", "soul forge powered by captured spirits", "Demonic"),
    ("TortureChamber", "iron torture chamber with chains and tools", "Demonic"),
    ("WaaaghTotem", "crude orcish totem pole with war trophies", "Orcish"),
    ("WarPit", "blood-stained war pit arena for combat", "Orcish"),
    ("LootHoard", "overflowing loot hoard pile of stolen treasure", "Orcish"),
    ("TidalPool", "magical tidal pool with bioluminescent water", "Aquatic"),
    ("CoralPalace", "magnificent coral palace underwater", "Aquatic"),
    ("PearlDiversLodge", "pearl diver lodge on stilts over water", "Aquatic"),
    ("Necropolis", "massive necropolis with tombs and undead guards", "Dark"),
    ("BoneForge", "bone forge crafting weapons from remains", "Abyssal"),
    ("PlagueCauldron", "bubbling plague cauldron emitting green fumes", "Abyssal"),
    ("LuckyCloverFarm", "idyllic halfling farm with four-leaf clovers", "Human"),
    ("BurglarsGuild", "secret burglar guild hidden behind bookshelf", "Human"),
    ("FeastHall", "warm halfling feast hall with long tables of food", "Human"),
    ("RegenerationPool", "glowing green regeneration pool", "Orcish"),
    ("TrollBridge", "massive troll bridge over gorge collecting tolls", "Orcish"),
    ("CarnagePit", "brutal carnage pit for berserker training", "Orcish"),
    ("HyenaKennels", "gnoll hyena kennels with beast pens", "Orcish"),
    ("RaidersCamp", "gnoll raider camp with looted supplies", "Orcish"),
    ("BoneShrine", "gnoll bone shrine decorated with skulls", "Orcish"),
    ("SwampHatchery", "swamp hatchery with raptor eggs in mud nests", "Aquatic"),
    ("VenomLab", "lizardmen venom laboratory with poison vats", "Aquatic"),
    ("ScaleForge", "lizardmen scale forge crafting scaled armor", "Aquatic"),
    ("CaravanBazaar", "nomad caravan bazaar with desert tents", "Human"),
    ("HorseLordsHall", "nomad horse lord hall with mounted trophies", "Human"),
    ("DesertOasis", "desert oasis with palm trees and clear water", "Human"),
    ("PrimalShrine", "primal nature shrine with totem poles and vines", "Orcish"),
    ("BeastPens", "beast pens with exotic captured creatures", "Orcish"),
    ("TotemCircle", "magical totem circle with nature spirits", "Orcish"),
]


def main():
    print("=== Loading SD Turbo ===")
    pipe_t2i = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe_t2i = pipe_t2i.to("cuda")
    pipe_t2i.enable_attention_slicing()
    pipe_i2i = AutoPipelineForImage2Image.from_pipe(pipe_t2i)

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    base_style = "fantasy building, isometric view, detailed game art asset, bright colors, digital painting"
    total_generated = 0

    # Phase 1: All 46 universal buildings in Human style (construction + complete)
    print(f"\n=== Phase 1: {len(UNIVERSAL_BUILDINGS)} universal buildings (Human style) ===")
    for name, desc in UNIVERSAL_BUILDINGS:
        for state in ["construction", "complete"]:
            outpath = os.path.join(OUTPUT_DIR, f"{name}_{state}.png")
            if os.path.exists(outpath):
                continue  # Skip already generated

            state_desc = "under construction with scaffolding and workers" if state == "construction" else "completed and functional"
            prompt = f"{desc}, {state_desc}, {ARCH_STYLES['Human']}, {base_style}"

            img = pipe_t2i(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                           width=512, height=512).images[0]
            img = ImageEnhance.Color(img).enhance(1.15)
            img = ImageEnhance.Sharpness(img).enhance(1.2)
            img.save(outpath, "PNG", optimize=True)
            total_generated += 1

            if total_generated % 10 == 0:
                print(f"  Generated {total_generated} images...")

    print(f"  Phase 1 complete: {total_generated} images")

    # Phase 2: All 45 racial buildings in their native style
    print(f"\n=== Phase 2: {len(RACIAL_BUILDINGS)} racial buildings ===")
    for name, desc, style_name in RACIAL_BUILDINGS:
        for state in ["construction", "complete"]:
            outpath = os.path.join(OUTPUT_DIR, f"{name}_{state}.png")
            if os.path.exists(outpath):
                continue

            state_desc = "under construction with scaffolding" if state == "construction" else "completed and functional"
            arch_desc = ARCH_STYLES.get(style_name, ARCH_STYLES["Human"])
            prompt = f"{desc}, {state_desc}, {arch_desc}, {base_style}"

            img = pipe_t2i(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                           width=512, height=512).images[0]
            img = ImageEnhance.Color(img).enhance(1.15)
            img.save(outpath, "PNG", optimize=True)
            total_generated += 1

            if total_generated % 10 == 0:
                print(f"  Generated {total_generated} images...")

    print(f"  Phase 2 complete: {total_generated} total images")

    # Phase 3: Universal buildings in other architecture styles (top 5 most common)
    priority_styles = ["Elven", "Dwarven", "Orcish", "Dark", "Draconic"]
    print(f"\n=== Phase 3: Universal buildings × {len(priority_styles)} priority styles ===")
    for style_name in priority_styles:
        arch_desc = ARCH_STYLES[style_name]
        for name, desc in UNIVERSAL_BUILDINGS:
            outpath = os.path.join(OUTPUT_DIR, f"{name}_{style_name.lower()}_complete.png")
            if os.path.exists(outpath):
                continue

            prompt = f"{desc}, completed and functional, {arch_desc}, {base_style}"
            img = pipe_t2i(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                           width=512, height=512).images[0]
            img = ImageEnhance.Color(img).enhance(1.15)
            img.save(outpath, "PNG", optimize=True)
            total_generated += 1

            if total_generated % 50 == 0:
                print(f"  Generated {total_generated} images...")

    print(f"  Phase 3 complete: {total_generated} total images")

    # Phase 4: Remaining 7 architecture styles for universal buildings
    remaining_styles = ["Demonic", "Aquatic", "Fey", "Ethereal", "Abyssal", "Crystalline"]
    print(f"\n=== Phase 4: Universal buildings × {len(remaining_styles)} remaining styles ===")
    for style_name in remaining_styles:
        arch_desc = ARCH_STYLES[style_name]
        for name, desc in UNIVERSAL_BUILDINGS:
            outpath = os.path.join(OUTPUT_DIR, f"{name}_{style_name.lower()}_complete.png")
            if os.path.exists(outpath):
                continue

            prompt = f"{desc}, completed and functional, {arch_desc}, {base_style}"
            img = pipe_t2i(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                           width=512, height=512).images[0]
            img = ImageEnhance.Color(img).enhance(1.15)
            img.save(outpath, "PNG", optimize=True)
            total_generated += 1

            if total_generated % 50 == 0:
                print(f"  Generated {total_generated} images...")

    # Cleanup
    del pipe_t2i, pipe_i2i
    gc.collect()
    torch.cuda.empty_cache()

    # Count total files
    total_files = len([f for f in os.listdir(OUTPUT_DIR) if f.endswith('.png')])
    print(f"\n=== DONE ===")
    print(f"  New images generated this run: {total_generated}")
    print(f"  Total building art files: {total_files}")


if __name__ == "__main__":
    main()
