"""Generate the CoM-style city panorama art set with SDXL on the dev server:

  - 34 building sprites x 2 states (complete + construction) = 68 PNGs
    on a flat neutral background so rembg can cleanly strip alpha.
  - 8 plane backdrops (panoramic 16:8 with sky + terrain, no buildings).

Files land in ~/cityview/raw/ as PNG. A companion postproc step (rembg) is
run via gen_city_panorama_alpha.py to convert sprites to RGBA.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
RAW_DIR = os.path.expanduser("~/cityview/raw")
BG_DIR  = os.path.expanduser("~/cityview/backgrounds")
os.makedirs(RAW_DIR, exist_ok=True)
os.makedirs(BG_DIR,  exist_ok=True)

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame, "
    "people, characters, figures, vehicles"
)

# Sprite style: isolated subject, neutral flat backdrop, oblique 3/4 view so
# perspective reads at panorama scale. rembg strips the backdrop.
SPRITE_STYLE = (
    "isometric oblique three-quarter view, single isolated subject centered, "
    "no people, no creatures, on a flat solid medium-grey background, "
    "fantasy painterly oil illustration, Master of Magic city-view art style, "
    "highly detailed, rich colour, no text, no border"
)

CONSTRUCTION_NOTE = (
    "under construction with wooden scaffolding, exposed timber framing, "
    "half-finished stonework, builder's planks and ropes, dust and sawdust, "
)

# (id, description). The layout map in CoMCityPanoramaWidget.cpp drives placement.
BUILDINGS = [
    ("palace",            "a grand fantasy royal palace with high gilded domes and tall flag-bearing spires"),
    ("cathedral",         "a tall gothic stone cathedral with rose window and twin pointed spires"),
    ("war_college",       "a fortified martial academy with crenellated keep and training yard"),
    ("wizard_guild",      "an arcane wizard guild of dark stone with glowing rune-etched windows"),
    ("mage_tower",        "a slender high mage's tower of pale stone with a glowing orb at its peak"),
    ("observatory",       "a domed astronomical observatory with brass telescope poking through the dome"),
    ("barracks",          "a sturdy military barracks of timber and stone with weapon racks visible"),
    ("marketplace",       "a bustling market hall with red-and-white striped awnings and trade stalls"),
    ("library",           "a calm scholarly library of tan stone with tall arched windows and ivy"),
    ("temple",            "a serene fantasy temple of white marble with golden symbol above its doors"),
    ("smithy",            "a soot-blackened blacksmith's forge with chimney smoke and glowing fire pit"),
    ("stable",            "a wooden horse stable with hay loft and open Dutch doors"),
    ("granary",           "a tall round wood-and-thatch granary with a conveyor ramp"),
    ("tavern",            "a warm timber tavern with hanging painted sign and lit lanterns"),
    ("fighters_guild",    "a fortified fighters' guildhall of heavy stone with weapon trophy mounts"),
    ("thieves_guild",     "a discreet thieves' guild in shadowed back-alley brick with iron gate"),
    ("armory",            "a stone armory hall with iron-banded doors and display weapons outside"),
    ("enchanter_workshop","an enchanter's stone workshop with glowing crystal in skylight"),
    ("alchemist_lab",     "an alchemist's lab of timber with bubbling green and purple flasks in windows"),
    ("summoning_circle",  "an open-air ritual circle of standing stones with glowing summoning runes"),
    ("oracle",            "a small marble oracle shrine with steam rising from its central basin"),
    ("aqueduct",          "an arched stone aqueduct carrying glittering water"),
    ("bank",              "a solid columned stone bank with gold coin emblem above the door"),
    ("colosseum",         "a tall stone amphitheatre with archways and torches"),
    ("shrine",            "a tiny stone shrine with offering candles and statue niche"),
    ("memorial",          "a tall carved stone memorial obelisk with engraved runes"),
    ("lighthouse",        "a tall white lighthouse with a bright lantern at its peak"),
    ("planar_beacon",     "a strange planar beacon of floating crystals with violet energy beam"),
    ("docks",             "wooden harbour docks with crates and a moored sailing ship in the water"),
    ("shipyard",          "a coastal shipyard with a wooden ship hull under construction on a slipway"),
    ("dragon_roost",      "a massive cliffside dragon roost with claw-scored stone perches"),
    ("walls_wood",        "a long fantasy wooden palisade wall with sharpened timber stakes"),
    ("walls_stone",       "a long fantasy stone city wall with crenellations and torches"),
    ("walls_iron",        "a long fantasy iron-banded battlemented wall with reinforced gates"),
]

# Plane backdrops — wide 16:8, plane-flavoured sky + terrain. No buildings.
PLANES = [
    ("aurelith",   "a sunlit golden-hour fantasy landscape, rolling green hills and distant blue mountains, "
                   "soft warm sky with painted clouds, empty foreground meadow, no people"),
    ("noctharion", "a moonlit shadow plane fantasy landscape, dark indigo rolling lands under a huge pale moon, "
                   "swirling violet mist, jagged silver-grey peaks, empty foreground, no people"),
    ("verdantis",  "a lush wild fantasy jungle plain, vibrant emerald canopy, distant misty waterfall cliffs, "
                   "humid green light, empty foreground clearing, no people"),
    ("infernyx",   "a hellish fantasy infernal plane, black volcanic plain under blood-red sky, "
                   "distant lava flows and ash storms, jagged obsidian ridges, empty foreground, no people"),
    ("aethermist", "an ethereal sky-realm fantasy plane, floating mossy stone islands in pearl-white cloud sea, "
                   "soft luminous sky, distant cloud cathedrals, empty foreground stone platform, no people"),
    ("abyssal",    "a sunken oceanic fantasy plane, kelp-forest seafloor under deep teal water shafts of light, "
                   "distant coral spires, schools of glow-fish silhouettes, empty sandy foreground, no people"),
    ("ethereal",   "a translucent fantasy spirit plane, ghostly silver-white landscape of soft glowing hills, "
                   "distant pale spectral peaks, drifting motes of light, empty foreground, no people"),
    ("feywild",    "a vivid fey fantasy plane, hyper-saturated pink-and-teal forest, giant glowing mushrooms, "
                   "twin moons in a starry sky, soft mystical glow, empty foreground glade, no people"),
]


STYLE_PANORAMA = (
    "wide cinematic fantasy matte painting, dramatic painterly oil illustration, "
    "Master of Magic city-view backdrop art style, no buildings, no people, "
    "no text, no border, rich atmospheric colour, highly detailed"
)


def seed_for(name: str) -> int:
    return 20000 + (hash(name) & 0xFFFF)


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)

    # --- Building sprites: complete + construction states -------------------
    print(f"\nGenerating {len(BUILDINGS) * 2} building sprites (768x768) ...")
    for bid, desc in BUILDINGS:
        for state, mod in (("complete", ""), ("construction", CONSTRUCTION_NOTE)):
            out_path = os.path.join(RAW_DIR, f"{bid}_{state}.png")
            if os.path.exists(out_path):
                print(f"[{bid}_{state}] exists, skipping")
                continue
            prompt = f"{mod}{desc}, {SPRITE_STYLE}"
            seed = seed_for(f"{bid}_{state}")
            print(f"[{bid}_{state}] seed={seed}")
            gen = torch.Generator("cuda").manual_seed(seed)
            res = pipe(prompt=prompt, negative_prompt=NEG, width=768, height=768,
                       num_inference_steps=30, guidance_scale=7.0, generator=gen)
            res.images[0].save(out_path)

    # --- Plane backdrops: 1536x768 (matches the 960x480 canvas aspect) -----
    print(f"\nGenerating {len(PLANES)} plane backdrops (1536x768) ...")
    for pname, frag in PLANES:
        out_path = os.path.join(BG_DIR, f"city_bg_{pname}.png")
        if os.path.exists(out_path):
            print(f"[bg {pname}] exists, skipping")
            continue
        prompt = f"{frag}, {STYLE_PANORAMA}"
        seed = seed_for(f"bg_{pname}")
        print(f"[bg {pname}] seed={seed}")
        gen = torch.Generator("cuda").manual_seed(seed)
        res = pipe(prompt=prompt, negative_prompt=NEG, width=1536, height=768,
                   num_inference_steps=32, guidance_scale=7.0, generator=gen)
        res.images[0].save(out_path)

    print(f"\nDone. raw={RAW_DIR}  bg={BG_DIR}")


if __name__ == "__main__":
    main()
