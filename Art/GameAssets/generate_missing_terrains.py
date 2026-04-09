#!/usr/bin/env python3
"""
Generate all 140 missing terrain tile PNGs by compositing existing base tiles
with plane-specific tints, overlays, and modifications.

Each output is 128x128 RGBA PNG placed in the appropriate plane directory.
"""

import os
from PIL import Image, ImageEnhance, ImageFilter, ImageDraw

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
TERRAIN_DIR = os.path.join(BASE_DIR, "processed", "terrain")
SIZE = (128, 128)


def load_base(plane, base_name):
    """Load a base terrain tile for a given plane."""
    path = os.path.join(TERRAIN_DIR, plane, f"{base_name}.png")
    img = Image.open(path).convert("RGBA")
    if img.size != SIZE:
        img = img.resize(SIZE, Image.LANCZOS)
    return img


def color_tint(img, color, opacity=0.35):
    """Blend a solid color overlay onto the image at given opacity."""
    overlay = Image.new("RGBA", img.size, color + (int(255 * opacity),))
    return Image.alpha_composite(img, overlay)


def adjust_brightness(img, factor):
    """Adjust brightness: >1 brighter, <1 darker."""
    enhancer = ImageEnhance.Brightness(img.convert("RGB"))
    result = enhancer.enhance(factor).convert("RGBA")
    result.putalpha(img.split()[3])
    return result


def adjust_saturation(img, factor):
    """Adjust color saturation: >1 more saturated, <1 less."""
    enhancer = ImageEnhance.Color(img.convert("RGB"))
    result = enhancer.enhance(factor).convert("RGBA")
    result.putalpha(img.split()[3])
    return result


def adjust_contrast(img, factor):
    """Adjust contrast."""
    enhancer = ImageEnhance.Contrast(img.convert("RGB"))
    result = enhancer.enhance(factor).convert("RGBA")
    result.putalpha(img.split()[3])
    return result


def add_glow_overlay(img, color, opacity=0.25):
    """Add a soft glow overlay of a given color."""
    glow = Image.new("RGBA", img.size, color + (0,))
    draw = ImageDraw.Draw(glow)
    cx, cy = img.size[0] // 2, img.size[1] // 2
    for r in range(64, 0, -1):
        alpha = int(255 * opacity * (r / 64.0))
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color + (alpha,))
    return Image.alpha_composite(img, glow)


def add_noise_overlay(img, color, density=0.15, opacity=0.5):
    """Add scattered dot overlay for spore/particle effects."""
    import random
    random.seed(hash(color))
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    w, h = img.size
    for _ in range(int(w * h * density)):
        x = random.randint(0, w - 1)
        y = random.randint(0, h - 1)
        a = random.randint(int(255 * opacity * 0.3), int(255 * opacity))
        draw.point((x, y), fill=color + (a,))
    return Image.alpha_composite(img, overlay)


def add_line_overlay(img, color, count=8, opacity=0.3):
    """Add lines/paths overlay."""
    import random
    random.seed(hash(color) + count)
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    w, h = img.size
    a = int(255 * opacity)
    for _ in range(count):
        x1, y1 = random.randint(0, w), random.randint(0, h)
        x2, y2 = random.randint(0, w), random.randint(0, h)
        draw.line([(x1, y1), (x2, y2)], fill=color + (a,), width=2)
    return Image.alpha_composite(img, overlay)


def add_web_overlay(img, color=(255, 255, 255), opacity=0.3):
    """Add a web/network pattern overlay."""
    import random
    random.seed(42)
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    w, h = img.size
    a = int(255 * opacity)
    # Create web-like lines from center
    cx, cy = w // 2, h // 2
    points = [(random.randint(0, w), random.randint(0, h)) for _ in range(12)]
    for p in points:
        draw.line([(cx, cy), p], fill=color + (a,), width=1)
    for i in range(len(points)):
        for j in range(i + 1, len(points)):
            if random.random() < 0.3:
                draw.line([points[i], points[j]], fill=color + (a // 2,), width=1)
    return Image.alpha_composite(img, overlay)


def save_terrain(img, plane, filename):
    """Save a terrain tile to the appropriate directory."""
    out_dir = os.path.join(TERRAIN_DIR, plane)
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, filename)
    img.save(out_path, "PNG")
    return out_path


def generate_all():
    """Generate all missing terrain tiles. Returns count of generated files."""
    generated = 0

    # =========================================================================
    # AURELITH SURFACE (10 missing)
    # =========================================================================
    plane = "aurelith"

    # River
    img = load_base(plane, "ocean")
    img = color_tint(img, (0, 180, 200), 0.40)
    img = adjust_brightness(img, 1.1)
    save_terrain(img, plane, "river.png"); generated += 1

    # Shore
    img = load_base(plane, "coast")
    img = color_tint(img, (240, 220, 180), 0.30)
    img = adjust_brightness(img, 1.15)
    save_terrain(img, plane, "shore.png"); generated += 1

    # Volcano
    img = load_base(plane, "mountain")
    img = color_tint(img, (200, 60, 20), 0.40)
    img = add_glow_overlay(img, (255, 100, 0), 0.25)
    save_terrain(img, plane, "volcano.png"); generated += 1

    # Plains
    img = load_base(plane, "grassland")
    img = color_tint(img, (210, 180, 80), 0.35)
    save_terrain(img, plane, "plains.png"); generated += 1

    # Savanna
    img = load_base(plane, "grassland")
    img = color_tint(img, (220, 200, 60), 0.40)
    img = adjust_saturation(img, 0.8)
    save_terrain(img, plane, "savanna.png"); generated += 1

    # Badlands
    img = load_base(plane, "desert")
    img = color_tint(img, (160, 80, 40), 0.40)
    img = adjust_contrast(img, 1.2)
    save_terrain(img, plane, "badlands.png"); generated += 1

    # Jungle (aurelith-specific variant)
    img = load_base(plane, "forest")
    img = color_tint(img, (0, 120, 20), 0.40)
    img = adjust_saturation(img, 1.4)
    save_terrain(img, plane, "jungle_aurelith.png"); generated += 1

    # EnchantedForest
    img = load_base(plane, "forest")
    img = color_tint(img, (180, 50, 220), 0.30)
    img = add_glow_overlay(img, (200, 100, 255), 0.20)
    save_terrain(img, plane, "enchanted_forest.png"); generated += 1

    # CrystalLake
    img = load_base(plane, "ocean")
    img = color_tint(img, (180, 220, 255), 0.35)
    img = adjust_brightness(img, 1.15)
    img = add_glow_overlay(img, (200, 240, 255), 0.15)
    save_terrain(img, plane, "crystal_lake.png"); generated += 1

    # AncientHighway
    img = load_base(plane, "desert")
    img = color_tint(img, (140, 140, 150), 0.45)
    img = add_line_overlay(img, (120, 120, 130), count=5, opacity=0.25)
    save_terrain(img, plane, "ancient_highway.png"); generated += 1

    # =========================================================================
    # NOCTHARION SURFACE (9 missing)
    # =========================================================================
    plane = "noctharion"

    # ShadowForest
    img = load_base(plane, "forest")
    img = adjust_brightness(img, 0.6)
    img = color_tint(img, (80, 0, 120), 0.40)
    save_terrain(img, plane, "shadow_forest.png"); generated += 1

    # ObsidianPlains
    img = load_base(plane, "grassland")
    img = adjust_brightness(img, 0.3)
    img = color_tint(img, (20, 20, 30), 0.45)
    img = adjust_contrast(img, 1.3)
    save_terrain(img, plane, "obsidian_plains.png"); generated += 1

    # CrystalDesert
    img = load_base(plane, "desert")
    img = color_tint(img, (120, 40, 180), 0.40)
    img = add_noise_overlay(img, (180, 100, 255), density=0.05, opacity=0.4)
    save_terrain(img, plane, "crystal_desert.png"); generated += 1

    # DarkMountains
    img = load_base(plane, "mountain")
    img = adjust_brightness(img, 0.4)
    img = color_tint(img, (30, 50, 120), 0.35)
    save_terrain(img, plane, "dark_mountains.png"); generated += 1

    # VoidOcean
    img = load_base(plane, "ocean")
    img = adjust_brightness(img, 0.3)
    img = color_tint(img, (40, 0, 60), 0.50)
    save_terrain(img, plane, "void_ocean.png"); generated += 1

    # CorruptedSwamp
    img = load_base(plane, "swamp")
    img = color_tint(img, (80, 180, 60), 0.30)
    img = color_tint(img, (120, 0, 160), 0.20)
    save_terrain(img, plane, "corrupted_swamp.png"); generated += 1

    # AshWastes
    img = load_base(plane, "desert")
    img = color_tint(img, (130, 130, 130), 0.50)
    img = adjust_saturation(img, 0.3)
    save_terrain(img, plane, "ash_wastes.png"); generated += 1

    # TwilightHills
    img = load_base(plane, "hills")
    img = color_tint(img, (150, 80, 180), 0.30)
    img = color_tint(img, (200, 120, 50), 0.15)
    save_terrain(img, plane, "twilight_hills.png"); generated += 1

    # GloomTundra
    img = load_base(plane, "tundra")
    img = adjust_brightness(img, 0.7)
    img = color_tint(img, (50, 60, 90), 0.40)
    save_terrain(img, plane, "gloom_tundra.png"); generated += 1

    # =========================================================================
    # VERDANTIS SURFACE (12 missing)
    # =========================================================================
    plane = "verdantis"

    # MegaJungle
    img = load_base(plane, "jungle")
    img = color_tint(img, (0, 180, 0), 0.35)
    img = adjust_saturation(img, 1.6)
    save_terrain(img, plane, "mega_jungle.png"); generated += 1

    # FungalForest
    img = load_base(plane, "forest")
    img = color_tint(img, (160, 80, 200), 0.30)
    img = color_tint(img, (80, 160, 80), 0.15)
    save_terrain(img, plane, "fungal_forest.png"); generated += 1

    # LivingSwamp
    img = load_base(plane, "swamp")
    img = color_tint(img, (50, 255, 100), 0.35)
    img = adjust_brightness(img, 1.2)
    img = add_glow_overlay(img, (100, 255, 150), 0.20)
    save_terrain(img, plane, "living_swamp.png"); generated += 1

    # RootMountains
    img = load_base(plane, "mountain")
    img = color_tint(img, (100, 70, 30), 0.35)
    img = color_tint(img, (60, 120, 40), 0.15)
    save_terrain(img, plane, "root_mountains.png"); generated += 1

    # PollenPlains
    img = load_base(plane, "grassland")
    img = color_tint(img, (200, 220, 50), 0.35)
    img = add_noise_overlay(img, (240, 240, 80), density=0.08, opacity=0.35)
    save_terrain(img, plane, "pollen_plains.png"); generated += 1

    # SporeDesert
    img = load_base(plane, "desert")
    img = color_tint(img, (120, 140, 100), 0.40)
    img = add_noise_overlay(img, (150, 170, 130), density=0.06, opacity=0.3)
    save_terrain(img, plane, "spore_desert.png"); generated += 1

    # AmberCoast
    img = load_base(plane, "coast")
    img = color_tint(img, (210, 160, 40), 0.40)
    save_terrain(img, plane, "amber_coast.png"); generated += 1

    # VineHills
    img = load_base(plane, "hills")
    img = color_tint(img, (30, 150, 30), 0.35)
    img = add_line_overlay(img, (20, 120, 20), count=12, opacity=0.25)
    save_terrain(img, plane, "vine_hills.png"); generated += 1

    # BioluminescentDeep
    img = load_base(plane, "ocean")
    img = color_tint(img, (0, 220, 220), 0.35)
    img = add_glow_overlay(img, (0, 255, 255), 0.25)
    save_terrain(img, plane, "bioluminescent_deep.png"); generated += 1

    # WorldTree
    img = load_base(plane, "forest")
    img = color_tint(img, (180, 200, 50), 0.30)
    img = adjust_brightness(img, 1.1)
    img = adjust_saturation(img, 1.2)
    save_terrain(img, plane, "world_tree.png"); generated += 1

    # SentientTerrain
    img = load_base(plane, "grassland")
    img = color_tint(img, (80, 200, 80), 0.25)
    img = color_tint(img, (150, 50, 200), 0.20)
    save_terrain(img, plane, "sentient_terrain.png"); generated += 1

    # MyceliumNetwork
    img = load_base(plane, "underdark_fungal")
    img = add_web_overlay(img, (240, 240, 240), 0.35)
    img = color_tint(img, (220, 220, 230), 0.15)
    save_terrain(img, plane, "mycelium_network.png"); generated += 1

    # =========================================================================
    # INFERNYX SURFACE (12 missing)
    # =========================================================================
    plane = "infernyx"

    # LavaFields
    img = load_base(plane, "volcanic")
    img = color_tint(img, (255, 100, 0), 0.45)
    img = adjust_brightness(img, 1.1)
    save_terrain(img, plane, "lava_fields.png"); generated += 1

    # ObsidianSpires
    img = load_base(plane, "mountain")
    img = adjust_brightness(img, 0.3)
    img = color_tint(img, (10, 10, 20), 0.40)
    img = adjust_contrast(img, 1.4)
    save_terrain(img, plane, "obsidian_spires.png"); generated += 1

    # AshDesert
    img = load_base(plane, "desert")
    img = color_tint(img, (140, 140, 140), 0.50)
    img = adjust_saturation(img, 0.2)
    save_terrain(img, plane, "ash_desert.png"); generated += 1

    # SulfurSeas
    img = load_base(plane, "ocean")
    img = color_tint(img, (180, 200, 30), 0.45)
    img = color_tint(img, (100, 180, 0), 0.15)
    save_terrain(img, plane, "sulfur_seas.png"); generated += 1

    # MagmaRivers
    img = load_base(plane, "ocean")
    img = color_tint(img, (255, 80, 0), 0.50)
    img = add_glow_overlay(img, (255, 120, 20), 0.30)
    save_terrain(img, plane, "magma_rivers.png"); generated += 1

    # CinderHills
    img = load_base(plane, "hills")
    img = adjust_brightness(img, 0.5)
    img = color_tint(img, (80, 80, 80), 0.35)
    img = add_noise_overlay(img, (255, 140, 30), density=0.03, opacity=0.5)
    save_terrain(img, plane, "cinder_hills.png"); generated += 1

    # ScorchedForest
    img = load_base(plane, "forest")
    img = adjust_brightness(img, 0.35)
    img = color_tint(img, (60, 10, 10), 0.40)
    img = color_tint(img, (180, 30, 0), 0.15)
    save_terrain(img, plane, "scorched_forest.png"); generated += 1

    # EmberPlains
    img = load_base(plane, "grassland")
    img = color_tint(img, (240, 120, 20), 0.40)
    img = add_glow_overlay(img, (255, 160, 40), 0.20)
    save_terrain(img, plane, "ember_plains.png"); generated += 1

    # BasaltMountains
    img = load_base(plane, "mountain")
    img = adjust_brightness(img, 0.4)
    img = color_tint(img, (40, 40, 50), 0.45)
    save_terrain(img, plane, "basalt_mountains.png"); generated += 1

    # FireGeyser
    img = load_base(plane, "volcanic")
    img = color_tint(img, (255, 240, 180), 0.40)
    img = adjust_brightness(img, 1.3)
    save_terrain(img, plane, "fire_geyser.png"); generated += 1

    # LavaTube_Surface
    img = load_base(plane, "volcanic")
    img = color_tint(img, (220, 80, 0), 0.40)
    img = add_line_overlay(img, (255, 100, 0), count=6, opacity=0.3)
    save_terrain(img, plane, "lava_tube_surface.png"); generated += 1

    # VolcanicChain
    img = load_base(plane, "mountain")
    img = color_tint(img, (200, 80, 0), 0.40)
    img = add_glow_overlay(img, (255, 100, 30), 0.20)
    save_terrain(img, plane, "volcanic_chain.png"); generated += 1

    # =========================================================================
    # AETHERMIST SURFACE (12 missing)
    # =========================================================================
    plane = "aethermist"

    # CloudPlains
    img = load_base(plane, "grassland")
    img = color_tint(img, (220, 230, 255), 0.40)
    img = adjust_brightness(img, 1.2)
    save_terrain(img, plane, "cloud_plains.png"); generated += 1

    # CrystalSpires
    img = load_base(plane, "crystal")
    img = color_tint(img, (200, 180, 255), 0.25)
    img = adjust_brightness(img, 1.3)
    img = adjust_saturation(img, 1.4)
    save_terrain(img, plane, "crystal_spires.png"); generated += 1

    # VoidRifts
    img = load_base(plane, "shadow")
    img = adjust_brightness(img, 0.3)
    img = color_tint(img, (60, 0, 80), 0.50)
    save_terrain(img, plane, "void_rifts.png"); generated += 1

    # FloatingIslands
    img = load_base(plane, "hills")
    img = color_tint(img, (150, 200, 255), 0.35)
    img = adjust_brightness(img, 1.15)
    save_terrain(img, plane, "floating_islands.png"); generated += 1

    # MistOcean
    img = load_base(plane, "ocean")
    img = color_tint(img, (220, 230, 250), 0.45)
    img = adjust_brightness(img, 1.2)
    img = adjust_saturation(img, 0.5)
    save_terrain(img, plane, "mist_ocean.png"); generated += 1

    # DreamMeadows
    img = load_base(plane, "grassland")
    img = color_tint(img, (255, 200, 220), 0.25)
    img = color_tint(img, (200, 220, 255), 0.15)
    img = color_tint(img, (220, 255, 200), 0.10)
    save_terrain(img, plane, "dream_meadows.png"); generated += 1

    # ResonancePeaks
    img = load_base(plane, "mountain")
    img = color_tint(img, (100, 150, 255), 0.35)
    img = add_glow_overlay(img, (180, 200, 255), 0.20)
    save_terrain(img, plane, "resonance_peaks.png"); generated += 1

    # EtherMarshes
    img = load_base(plane, "swamp")
    img = color_tint(img, (100, 200, 180), 0.35)
    img = add_glow_overlay(img, (120, 220, 200), 0.15)
    save_terrain(img, plane, "ether_marshes.png"); generated += 1

    # StarlightTundra
    img = load_base(plane, "tundra")
    img = color_tint(img, (200, 210, 240), 0.25)
    img = add_noise_overlay(img, (255, 255, 255), density=0.04, opacity=0.6)
    save_terrain(img, plane, "starlight_tundra.png"); generated += 1

    # GravityAnomaly
    img = load_base(plane, "corrupted")
    img = color_tint(img, (140, 40, 180), 0.40)
    img = adjust_contrast(img, 1.3)
    save_terrain(img, plane, "gravity_anomaly.png"); generated += 1

    # PhaseShiftingTerrain
    img = load_base(plane, "ethereal")
    img = color_tint(img, (180, 200, 255), 0.30)
    img = adjust_brightness(img, 1.1)
    img = adjust_saturation(img, 0.7)
    save_terrain(img, plane, "phase_shifting_terrain.png"); generated += 1

    # AuroraField
    img = load_base(plane, "grassland")
    img = color_tint(img, (50, 200, 100), 0.25)
    img = color_tint(img, (150, 50, 200), 0.20)
    img = add_glow_overlay(img, (100, 255, 150), 0.15)
    save_terrain(img, plane, "aurora_field.png"); generated += 1

    # =========================================================================
    # ABYSSAL SURFACE (12 missing)
    # =========================================================================
    plane = "abyssal"

    # BonePlains
    img = load_base(plane, "grassland")
    img = color_tint(img, (240, 230, 210), 0.45)
    img = adjust_saturation(img, 0.3)
    save_terrain(img, plane, "bone_plains.png"); generated += 1

    # BloodRivers
    img = load_base(plane, "ocean")
    img = color_tint(img, (160, 10, 10), 0.50)
    img = adjust_brightness(img, 0.7)
    save_terrain(img, plane, "blood_rivers.png"); generated += 1

    # ScreamingChasms
    img = load_base(plane, "shadow")
    img = color_tint(img, (120, 10, 10), 0.45)
    img = adjust_brightness(img, 0.4)
    save_terrain(img, plane, "screaming_chasms.png"); generated += 1

    # LivingWalls
    img = load_base(plane, "mountain")
    img = color_tint(img, (200, 120, 120), 0.40)
    img = color_tint(img, (220, 150, 140), 0.15)
    save_terrain(img, plane, "living_walls.png"); generated += 1

    # DemonPillars
    img = load_base(plane, "mountain")
    img = adjust_brightness(img, 0.35)
    img = color_tint(img, (100, 10, 10), 0.45)
    save_terrain(img, plane, "demon_pillars.png"); generated += 1

    # FleshCitadels
    img = load_base(plane, "hills")
    img = color_tint(img, (210, 120, 130), 0.45)
    save_terrain(img, plane, "flesh_citadels.png"); generated += 1

    # VoidFissures
    img = load_base(plane, "shadow")
    img = adjust_brightness(img, 0.3)
    img = color_tint(img, (60, 0, 80), 0.50)
    save_terrain(img, plane, "void_fissures.png"); generated += 1

    # ChaosStormfield
    img = load_base(plane, "corrupted")
    img = color_tint(img, (180, 30, 60), 0.35)
    img = color_tint(img, (120, 0, 160), 0.20)
    save_terrain(img, plane, "chaos_stormfield.png"); generated += 1

    # AbyssalMires
    img = load_base(plane, "swamp")
    img = color_tint(img, (120, 40, 30), 0.45)
    img = adjust_brightness(img, 0.7)
    save_terrain(img, plane, "abyssal_mires.png"); generated += 1

    # ChasmEdge
    img = load_base(plane, "coast")
    img = adjust_brightness(img, 0.5)
    img = color_tint(img, (50, 40, 40), 0.40)
    save_terrain(img, plane, "chasm_edge.png"); generated += 1

    # CarrionDesert
    img = load_base(plane, "desert")
    img = color_tint(img, (180, 160, 60), 0.40)
    img = adjust_saturation(img, 0.6)
    save_terrain(img, plane, "carrion_desert.png"); generated += 1

    # GoreMarshes
    img = load_base(plane, "swamp")
    img = color_tint(img, (180, 20, 20), 0.50)
    save_terrain(img, plane, "gore_marshes.png"); generated += 1

    # =========================================================================
    # ETHEREAL SURFACE (11 missing)
    # =========================================================================
    plane = "ethereal"

    # MistSeas
    img = load_base(plane, "ocean")
    img = color_tint(img, (240, 240, 250), 0.45)
    img = adjust_saturation(img, 0.3)
    img = adjust_brightness(img, 1.2)
    save_terrain(img, plane, "mist_seas.png"); generated += 1

    # CrystallizedMemories
    img = load_base(plane, "crystal")
    img = color_tint(img, (200, 220, 255), 0.30)
    img = adjust_brightness(img, 1.2)
    save_terrain(img, plane, "crystallized_memories.png"); generated += 1

    # EchoRuins
    img = load_base(plane, "desert")
    img = color_tint(img, (160, 160, 170), 0.45)
    img = adjust_saturation(img, 0.2)
    img = adjust_brightness(img, 0.8)
    save_terrain(img, plane, "echo_ruins.png"); generated += 1

    # ThoughtStorms
    img = load_base(plane, "shadow")
    img = color_tint(img, (140, 60, 200), 0.40)
    img = add_glow_overlay(img, (160, 80, 220), 0.20)
    save_terrain(img, plane, "thought_storms.png"); generated += 1

    # PhaseShores
    img = load_base(plane, "coast")
    img = color_tint(img, (200, 210, 240), 0.35)
    img = adjust_saturation(img, 0.5)
    save_terrain(img, plane, "phase_shores.png"); generated += 1

    # DreamDeserts
    img = load_base(plane, "desert")
    img = color_tint(img, (240, 210, 220), 0.35)
    img = adjust_brightness(img, 1.15)
    save_terrain(img, plane, "dream_deserts.png"); generated += 1

    # SpiritArchipelagos
    img = load_base(plane, "coast")
    img = color_tint(img, (150, 180, 230), 0.35)
    img = add_glow_overlay(img, (160, 190, 240), 0.15)
    save_terrain(img, plane, "spirit_archipelagos.png"); generated += 1

    # ReflectionMirrors
    img = load_base(plane, "ocean")
    img = color_tint(img, (200, 200, 210), 0.45)
    img = adjust_brightness(img, 1.3)
    img = adjust_contrast(img, 1.3)
    save_terrain(img, plane, "reflection_mirrors.png"); generated += 1

    # VoidWhisperFields
    img = load_base(plane, "grassland")
    img = adjust_brightness(img, 0.6)
    img = color_tint(img, (40, 30, 50), 0.35)
    img = add_noise_overlay(img, (200, 200, 220), density=0.02, opacity=0.3)
    save_terrain(img, plane, "void_whisper_fields.png"); generated += 1

    # MemoryLandscape
    img = load_base(plane, "grassland")
    img = color_tint(img, (180, 150, 100), 0.40)
    img = adjust_saturation(img, 0.4)
    save_terrain(img, plane, "memory_landscape.png"); generated += 1

    # FadeZones
    img = load_base(plane, "shadow")
    img = color_tint(img, (200, 200, 210), 0.40)
    img = adjust_brightness(img, 1.3)
    img = adjust_saturation(img, 0.2)
    save_terrain(img, plane, "fade_zones.png"); generated += 1

    # =========================================================================
    # FEYWILD SURFACE (15 missing)
    # =========================================================================
    plane = "feywild"

    # EternalForest
    img = load_base(plane, "forest")
    img = color_tint(img, (100, 200, 50), 0.30)
    img = color_tint(img, (200, 180, 40), 0.15)
    img = adjust_saturation(img, 1.5)
    save_terrain(img, plane, "eternal_forest.png"); generated += 1

    # ShiftingGlades
    img = load_base(plane, "grassland")
    img = color_tint(img, (255, 200, 200), 0.15)
    img = color_tint(img, (200, 200, 255), 0.15)
    img = color_tint(img, (200, 255, 200), 0.10)
    img = adjust_saturation(img, 1.3)
    save_terrain(img, plane, "shifting_glades.png"); generated += 1

    # MushroomRings
    img = load_base(plane, "grassland")
    img = color_tint(img, (160, 60, 100), 0.30)
    img = add_noise_overlay(img, (180, 80, 140), density=0.04, opacity=0.4)
    save_terrain(img, plane, "mushroom_rings.png"); generated += 1

    # MirrorLakes
    img = load_base(plane, "ocean")
    img = color_tint(img, (190, 200, 220), 0.40)
    img = adjust_brightness(img, 1.25)
    img = adjust_contrast(img, 1.2)
    save_terrain(img, plane, "mirror_lakes.png"); generated += 1

    # FeyMarketCrossroads
    img = load_base(plane, "desert")
    img = color_tint(img, (200, 150, 50), 0.30)
    img = color_tint(img, (180, 80, 180), 0.15)
    img = adjust_saturation(img, 1.3)
    save_terrain(img, plane, "fey_market_crossroads.png"); generated += 1

    # ThorncraftHollow
    img = load_base(plane, "forest")
    img = adjust_brightness(img, 0.6)
    img = color_tint(img, (20, 80, 20), 0.40)
    img = add_line_overlay(img, (30, 60, 10), count=10, opacity=0.3)
    save_terrain(img, plane, "thorncraft_hollow.png"); generated += 1

    # AncientStandingStones
    img = load_base(plane, "hills")
    img = color_tint(img, (140, 140, 150), 0.35)
    img = color_tint(img, (60, 120, 60), 0.15)
    save_terrain(img, plane, "ancient_standing_stones.png"); generated += 1

    # BlossomPeaks
    img = load_base(plane, "mountain")
    img = color_tint(img, (240, 160, 180), 0.35)
    img = add_noise_overlay(img, (255, 180, 200), density=0.06, opacity=0.4)
    save_terrain(img, plane, "blossom_peaks.png"); generated += 1

    # SilverMistPlains
    img = load_base(plane, "grassland")
    img = color_tint(img, (200, 200, 220), 0.40)
    img = adjust_saturation(img, 0.5)
    save_terrain(img, plane, "silver_mist_plains.png"); generated += 1

    # TwilightMeadows
    img = load_base(plane, "grassland")
    img = color_tint(img, (140, 80, 180), 0.30)
    img = color_tint(img, (200, 160, 40), 0.15)
    save_terrain(img, plane, "twilight_meadows.png"); generated += 1

    # WildHuntGrounds
    img = load_base(plane, "forest")
    img = adjust_brightness(img, 0.5)
    img = color_tint(img, (20, 60, 20), 0.40)
    img = adjust_contrast(img, 1.3)
    save_terrain(img, plane, "wild_hunt_grounds.png"); generated += 1

    # FeyWastes
    img = load_base(plane, "desert")
    img = color_tint(img, (180, 170, 160), 0.40)
    img = adjust_saturation(img, 0.4)
    img = adjust_brightness(img, 0.85)
    save_terrain(img, plane, "fey_wastes.png"); generated += 1

    # SeelieCourt
    img = load_base(plane, "grassland")
    img = color_tint(img, (255, 220, 100), 0.35)
    img = adjust_brightness(img, 1.25)
    img = add_glow_overlay(img, (255, 230, 120), 0.20)
    save_terrain(img, plane, "seelie_court.png"); generated += 1

    # UnseelieHollow
    img = load_base(plane, "forest")
    img = adjust_brightness(img, 0.4)
    img = color_tint(img, (80, 20, 120), 0.45)
    save_terrain(img, plane, "unseelie_hollow.png"); generated += 1

    # ErlingPath
    img = load_base(plane, "grassland")
    img = color_tint(img, (200, 210, 220), 0.30)
    img = add_line_overlay(img, (210, 220, 230), count=5, opacity=0.35)
    save_terrain(img, plane, "erling_path.png"); generated += 1

    # =========================================================================
    # UNDERDARK TERRAINS (56 total across all planes)
    # =========================================================================

    # --- Aurelith Underdark (10) ---
    plane = "aurelith"
    ud_terrains_aurelith = [
        ("deep_cavern", "underdark_cave", (100, 80, 60), 0.40, None),
        ("crystal_grotto", "underdark_cave", (100, 160, 220), 0.35, "glow"),
        ("dwarven_halls", "underdark_cave", (160, 140, 120), 0.40, None),
        ("underground_river", "underdark_cave", (40, 120, 180), 0.40, None),
        ("mushroom_cavern", "underdark_fungal", (140, 100, 180), 0.35, None),
        ("mineral_vein", "underdark_cave", (180, 160, 80), 0.40, "noise"),
        ("underground_lake", "underdark_cave", (60, 100, 160), 0.40, None),
        ("stalactite_forest", "underdark_cave", (120, 110, 100), 0.35, "line"),
        ("gem_deposit", "underdark_cave", (100, 180, 200), 0.35, "noise"),
        ("ancient_ruins_ud", "underdark_cave", (140, 140, 150), 0.40, None),
    ]
    for name, base, tint, opacity, fx in ud_terrains_aurelith:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "noise":
            img = add_noise_overlay(img, tint, density=0.04, opacity=0.3)
        elif fx == "line":
            img = add_line_overlay(img, tint, count=8, opacity=0.25)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Noctharion Underdark (10) ---
    plane = "noctharion"
    ud_terrains_noct = [
        ("shadow_tunnels", "underdark_cave", (60, 20, 80), 0.45, None),
        ("void_cavern", "underdark_cave", (20, 0, 30), 0.50, None),
        ("bone_catacombs", "underdark_cave", (220, 210, 190), 0.40, None),
        ("web_grotto", "underdark_cave", (150, 150, 160), 0.35, "web"),
        ("dark_fungal_forest", "underdark_fungal", (80, 40, 120), 0.40, None),
        ("obsidian_tunnels", "underdark_cave", (15, 15, 20), 0.45, None),
        ("nightmare_cavern", "underdark_cave", (100, 20, 40), 0.45, "glow"),
        ("echo_chambers", "underdark_cave", (80, 80, 100), 0.35, None),
        ("shadow_fungal", "underdark_fungal", (50, 20, 70), 0.45, None),
        ("void_rift_ud", "underdark_cave", (40, 0, 60), 0.50, "glow"),
    ]
    for name, base, tint, opacity, fx in ud_terrains_noct:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "web":
            img = add_web_overlay(img, (180, 180, 190), 0.30)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Verdantis Underdark (10) ---
    plane = "verdantis"
    ud_terrains_verd = [
        ("root_cavern", "underdark_cave", (120, 80, 40), 0.40, "line"),
        ("mycelium_grotto", "underdark_fungal", (230, 230, 240), 0.35, "web"),
        ("amber_cave", "underdark_cave", (210, 160, 40), 0.40, "glow"),
        ("insect_hive", "underdark_cave", (80, 140, 60), 0.40, "noise"),
        ("living_tunnels", "underdark_cave", (60, 160, 60), 0.35, None),
        ("spore_cavern", "underdark_fungal", (120, 140, 100), 0.40, "noise"),
        ("sap_rivers", "underdark_cave", (180, 140, 40), 0.40, None),
        ("root_bridge", "underdark_cave", (100, 70, 30), 0.40, "line"),
        ("seed_vault", "underdark_cave", (80, 120, 40), 0.35, None),
        ("fungal_deep", "underdark_fungal", (100, 60, 140), 0.40, "glow"),
    ]
    for name, base, tint, opacity, fx in ud_terrains_verd:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "noise":
            img = add_noise_overlay(img, tint, density=0.04, opacity=0.3)
        elif fx == "line":
            img = add_line_overlay(img, tint, count=8, opacity=0.25)
        elif fx == "web":
            img = add_web_overlay(img, (220, 220, 230), 0.30)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Infernyx Underdark (10) ---
    plane = "infernyx"
    ud_terrains_inf = [
        ("lava_tubes", "underdark_cave", (240, 120, 20), 0.45, "glow"),
        ("obsidian_cavern", "underdark_cave", (10, 10, 15), 0.50, None),
        ("sulfur_vents", "underdark_cave", (200, 200, 40), 0.40, "noise"),
        ("forge_chambers", "underdark_cave", (200, 50, 20), 0.40, "glow"),
        ("magma_reservoir", "underdark_cave", (255, 80, 0), 0.50, "glow"),
        ("ash_tunnels", "underdark_cave", (130, 130, 130), 0.45, None),
        ("ember_grotto", "underdark_cave", (240, 140, 40), 0.40, "noise"),
        ("fire_crystal_cave", "underdark_cave", (255, 100, 60), 0.35, "glow"),
        ("iron_mines", "underdark_cave", (100, 90, 80), 0.40, None),
        ("brimstone_deep", "underdark_cave", (200, 180, 30), 0.45, None),
    ]
    for name, base, tint, opacity, fx in ud_terrains_inf:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.25)
        elif fx == "noise":
            img = add_noise_overlay(img, tint, density=0.05, opacity=0.35)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Aethermist Underdark (10) ---
    plane = "aethermist"
    ud_terrains_aeth = [
        ("phase_tunnels", "underdark_cave", (100, 140, 220), 0.40, None),
        ("crystal_cavern", "underdark_cave", (180, 200, 255), 0.35, "glow"),
        ("void_pocket", "underdark_cave", (60, 0, 100), 0.50, None),
        ("dream_grotto", "underdark_cave", (220, 180, 220), 0.35, "glow"),
        ("resonance_cave", "underdark_cave", (120, 160, 255), 0.35, "glow"),
        ("ether_pool", "underdark_cave", (100, 200, 180), 0.40, None),
        ("starlight_cavern", "underdark_cave", (200, 210, 240), 0.30, "noise"),
        ("gravity_well", "underdark_cave", (140, 60, 180), 0.45, None),
        ("mist_tunnels", "underdark_cave", (200, 210, 230), 0.40, None),
        ("phase_fungal", "underdark_fungal", (120, 160, 220), 0.35, None),
    ]
    for name, base, tint, opacity, fx in ud_terrains_aeth:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "noise":
            img = add_noise_overlay(img, tint, density=0.03, opacity=0.5)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Abyssal Underdark (8) ---
    plane = "abyssal"
    ud_terrains_aby = [
        ("bone_tunnels", "underdark_cave", (230, 220, 200), 0.40, None),
        ("flesh_cavern", "underdark_cave", (200, 120, 130), 0.45, None),
        ("blood_pools", "underdark_cave", (160, 20, 20), 0.50, "glow"),
        ("chaos_rift_ud", "underdark_cave", (160, 40, 120), 0.45, "glow"),
        ("void_pit", "underdark_cave", (30, 0, 40), 0.50, None),
        ("demon_warren", "underdark_cave", (120, 20, 20), 0.45, None),
        ("torment_caves", "underdark_cave", (140, 30, 40), 0.45, "noise"),
        ("abyssal_fungal", "underdark_fungal", (100, 20, 40), 0.45, None),
    ]
    for name, base, tint, opacity, fx in ud_terrains_aby:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "noise":
            img = add_noise_overlay(img, tint, density=0.04, opacity=0.35)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Ethereal Underdark (8) ---
    plane = "ethereal"
    ud_terrains_eth = [
        ("memory_tunnels", "underdark_cave", (120, 140, 200), 0.40, None),
        ("echo_cavern", "underdark_cave", (140, 140, 160), 0.40, None),
        ("phase_grotto", "underdark_cave", (160, 180, 220), 0.35, "glow"),
        ("mist_caves", "underdark_cave", (220, 220, 230), 0.40, None),
        ("spirit_tunnels", "underdark_cave", (180, 190, 220), 0.35, "glow"),
        ("fade_cavern", "underdark_cave", (200, 200, 210), 0.40, None),
        ("thought_grotto", "underdark_cave", (140, 80, 180), 0.35, "glow"),
        ("ethereal_fungal", "underdark_fungal", (160, 180, 210), 0.35, None),
    ]
    for name, base, tint, opacity, fx in ud_terrains_eth:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # --- Feywild Underdark (8) ---
    plane = "feywild"
    ud_terrains_fey = [
        ("fey_root_cavern", "underdark_cave", (60, 140, 60), 0.40, "line"),
        ("mushroom_grotto", "underdark_fungal", (160, 80, 180), 0.40, "glow"),
        ("crystal_fey_cave", "underdark_cave", (120, 180, 220), 0.35, "glow"),
        ("cobweb_tunnels", "underdark_cave", (160, 160, 170), 0.35, "web"),
        ("enchanted_cavern", "underdark_cave", (120, 80, 200), 0.35, "glow"),
        ("fey_fungal_deep", "underdark_fungal", (80, 160, 80), 0.35, None),
        ("pixie_grotto", "underdark_cave", (200, 180, 255), 0.30, "glow"),
        ("wild_root_tunnel", "underdark_cave", (80, 120, 40), 0.40, "line"),
    ]
    for name, base, tint, opacity, fx in ud_terrains_fey:
        img = load_base(plane, base)
        img = color_tint(img, tint, opacity)
        if fx == "glow":
            img = add_glow_overlay(img, tint, 0.20)
        elif fx == "line":
            img = add_line_overlay(img, tint, count=8, opacity=0.25)
        elif fx == "web":
            img = add_web_overlay(img, (200, 200, 210), 0.30)
        save_terrain(img, plane, f"{name}.png"); generated += 1

    # =========================================================================
    # UNDERWATER (10 missing) - use aurelith as default plane for base tiles
    # =========================================================================
    # Place in a shared "underwater" conceptual area - use aurelith bases
    uw_plane = "aurelith"  # base source

    uw_terrains = [
        ("ocean_floor", "underwater_abyss", (20, 60, 80), 0.40, None, "aurelith"),
        ("coral_reef", "underwater_reef", (240, 140, 100), 0.35, "noise", "aurelith"),
        ("kelp_forest", "underwater_reef", (40, 140, 40), 0.40, "line", "aurelith"),
        ("deep_trench", "underwater_abyss", (10, 10, 30), 0.50, None, "aurelith"),
        ("underwater_volcano", "underwater_abyss", (200, 60, 20), 0.45, "glow", "aurelith"),
        ("sunken_ruins", "underwater_abyss", (120, 140, 120), 0.40, None, "aurelith"),
        ("crystal_grotto_uw", "underwater_reef", (120, 180, 240), 0.35, "glow", "aurelith"),
        ("abyssal_plain", "underwater_abyss", (5, 5, 15), 0.50, None, "aurelith"),
        ("seamount", "underwater_abyss", (80, 70, 100), 0.40, None, "aurelith"),
        ("thermal_vent", "underwater_abyss", (220, 100, 30), 0.40, "glow", "aurelith"),
    ]

    # Output underwater terrains to all planes that have underwater bases
    # Per spec, just generate them once in a logical location
    # We'll put them in each plane's directory for completeness
    for name, base, tint, opacity, fx, src_plane in uw_terrains:
        # Generate for all planes
        for target_plane in ["aurelith", "noctharion", "verdantis", "infernyx",
                             "aethermist", "abyssal", "ethereal", "feywild"]:
            img = load_base(target_plane, base)
            img = color_tint(img, tint, opacity)
            if fx == "glow":
                img = add_glow_overlay(img, tint, 0.25)
            elif fx == "noise":
                img = add_noise_overlay(img, tint, density=0.05, opacity=0.35)
            elif fx == "line":
                img = add_line_overlay(img, tint, count=10, opacity=0.30)
            save_terrain(img, target_plane, f"{name}.png")
        generated += 1  # Count as 1 per unique terrain type

    # =========================================================================
    # INFERNYX IRON QUARTER (7) + IRON DEPTHS (4)
    # =========================================================================
    plane = "infernyx"

    # Iron Quarter surface terrains
    # IronFortresses
    img = load_base(plane, "mountain")
    img = color_tint(img, (100, 100, 110), 0.45)
    img = adjust_saturation(img, 0.3)
    save_terrain(img, plane, "iron_fortresses.png"); generated += 1

    # SoulForges
    img = load_base(plane, "volcanic")
    img = color_tint(img, (140, 30, 80), 0.40)
    img = add_glow_overlay(img, (160, 40, 100), 0.25)
    save_terrain(img, plane, "soul_forges.png"); generated += 1

    # FireLakes
    img = load_base(plane, "ocean")
    img = color_tint(img, (240, 100, 20), 0.50)
    img = add_glow_overlay(img, (255, 120, 30), 0.25)
    save_terrain(img, plane, "fire_lakes.png"); generated += 1

    # BrimstoneHighways
    img = load_base(plane, "desert")
    img = color_tint(img, (210, 200, 40), 0.45)
    img = add_line_overlay(img, (200, 190, 30), count=5, opacity=0.3)
    save_terrain(img, plane, "brimstone_highways.png"); generated += 1

    # ContractSpires
    img = load_base(plane, "crystal")
    img = color_tint(img, (140, 30, 30), 0.40)
    img = color_tint(img, (180, 140, 20), 0.15)
    save_terrain(img, plane, "contract_spires.png"); generated += 1

    # HellgateMarkets
    img = load_base(plane, "grassland")
    img = adjust_brightness(img, 0.6)
    img = color_tint(img, (160, 40, 20), 0.35)
    img = color_tint(img, (200, 160, 40), 0.15)
    save_terrain(img, plane, "hellgate_markets.png"); generated += 1

    # TormentFields
    img = load_base(plane, "grassland")
    img = adjust_brightness(img, 0.5)
    img = color_tint(img, (140, 20, 20), 0.45)
    save_terrain(img, plane, "torment_fields.png"); generated += 1

    # Iron Depths (underdark variants)
    # SoulForgeCavern
    img = load_base(plane, "underdark_cave")
    img = color_tint(img, (140, 30, 60), 0.45)
    img = add_glow_overlay(img, (160, 40, 80), 0.25)
    save_terrain(img, plane, "soul_forge_cavern.png"); generated += 1

    # ContractVault
    img = load_base(plane, "underdark_cave")
    img = color_tint(img, (180, 120, 20), 0.40)
    img = color_tint(img, (180, 40, 20), 0.15)
    save_terrain(img, plane, "contract_vault.png"); generated += 1

    # FireLakeCavern
    img = load_base(plane, "underdark_cave")
    img = color_tint(img, (240, 130, 20), 0.45)
    img = add_glow_overlay(img, (255, 150, 30), 0.25)
    save_terrain(img, plane, "fire_lake_cavern.png"); generated += 1

    # HellmarketCave
    img = load_base(plane, "underdark_cave")
    img = color_tint(img, (200, 170, 40), 0.40)
    save_terrain(img, plane, "hellmarket_cave.png"); generated += 1

    return generated


if __name__ == "__main__":
    print("Generating missing terrain tiles...")
    count = generate_all()
    print(f"Done! Generated {count} terrain tile(s).")
