"""
Generate wizard portraits, realm icons, and class illustrations
for the Wizard Creation screen.
"""

import torch
from diffusers import AutoPipelineForText2Image
from PIL import Image, ImageEnhance
import os
import gc

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\wizards"

def gen(pipe, prompt, path, size=512):
    if os.path.exists(path):
        return
    img = pipe(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
               width=size, height=size).images[0]
    img = ImageEnhance.Color(img).enhance(1.15)
    img = ImageEnhance.Contrast(img).enhance(1.05)
    img.save(path, "PNG", optimize=True)

def main():
    print("=== Loading SD Turbo ===")
    pipe = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe = pipe.to("cuda")
    pipe.enable_attention_slicing()

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    portrait_style = "fantasy character portrait, head and shoulders, dark background, " \
                     "detailed face, dramatic lighting, digital painting, game art"

    # ── 14 Wizard Portraits (diverse, like MoM's wizard selection) ────────
    print("\n=== Generating 14 Wizard Portraits ===")
    portraits = [
        ("wizard_01", "elderly wise male wizard with long white beard, blue robes, pointed hat, kind eyes"),
        ("wizard_02", "young female sorceress with red hair, golden circlet, green eyes, confident smile"),
        ("wizard_03", "dark-skinned male wizard with shaved head, glowing tattoos, purple robes, intense gaze"),
        ("wizard_04", "elderly female witch with silver hair, crow on shoulder, dark green cloak, mysterious"),
        ("wizard_05", "young male warlock with black hair, scarred face, red eyes, leather armor, menacing"),
        ("wizard_06", "ethereal female elf wizard with platinum blonde hair, crystal crown, pale skin, serene"),
        ("wizard_07", "dwarven male wizard with braided red beard, rune-covered staff, iron helm, stout"),
        ("wizard_08", "ancient male necromancer with gaunt face, hollow eyes, black hood, skeletal hands"),
        ("wizard_09", "young female druid with flowers in brown hair, antler crown, forest cloak, nature magic"),
        ("wizard_10", "male fire mage with flame-red mohawk, ember eyes, brass armor, aggressive"),
        ("wizard_11", "female ice queen wizard with white hair, ice crown, blue skin tint, cold beauty"),
        ("wizard_12", "old male scholar wizard with spectacles, book in hand, brown robes, gentle"),
        ("wizard_13", "female demon-touched warlock with horns, violet skin, black and gold robes, exotic"),
        ("wizard_14", "male celestial wizard with golden halo, white robes, angelic features, divine glow"),
    ]

    for name, desc in portraits:
        print(f"  {name}...")
        gen(pipe, f"{desc}, {portrait_style}", os.path.join(OUTPUT_DIR, f"{name}.png"))

    # ── 9 Realm Icons (magic school symbols) ──────────────────────────────
    print("\n=== Generating 9 Realm Icons ===")
    icon_style = "magical symbol icon, circular emblem, glowing, dark background, game UI icon art, clean"
    realms = [
        ("realm_life", "golden sun symbol, holy light rays, white and gold, life magic"),
        ("realm_death", "purple skull with ghostly aura, dark purple, death magic"),
        ("realm_chaos", "red flame vortex spiral, orange and red, chaos fire magic"),
        ("realm_nature", "green tree of life with roots, emerald glow, nature earth magic"),
        ("realm_sorcery", "blue arcane crystal with runes, sapphire blue, sorcery magic"),
        ("realm_arcane", "golden eye with geometric patterns, amber glow, arcane knowledge magic"),
        ("realm_binding", "crimson chains in circle, dark red, binding contract magic"),
        ("realm_spirit", "violet ethereal ghost wisp, purple glow, spirit magic"),
        ("realm_glamour", "pink shimmering mirror with sparkles, rose glow, glamour illusion magic"),
    ]

    for name, desc in realms:
        print(f"  {name}...")
        gen(pipe, f"{desc}, {icon_style}", os.path.join(OUTPUT_DIR, f"{name}.png"))

    # ── 3 Class Illustrations (Wizard / Psyker / Warlock) ─────────────────
    print("\n=== Generating 3 Class Illustrations ===")
    class_style = "full body fantasy character illustration, dramatic pose, dark background, " \
                  "detailed armor and robes, digital painting, concept art"
    classes = [
        ("class_wizard", "classic wizard in flowing blue robes with staff, casting arcane spell, wise and powerful"),
        ("class_psyker", "psychic warrior in light armor with glowing mind powers, telekinetic energy around hands, intense concentration"),
        ("class_warlock", "dark warlock in black and red armor with demonic familiar, summoning hellfire, sinister and powerful"),
    ]

    for name, desc in classes:
        print(f"  {name}...")
        gen(pipe, f"{desc}, {class_style}", os.path.join(OUTPUT_DIR, f"{name}.png"))

    # ── Wizard Creation Background ────────────────────────────────────────
    print("\n=== Generating Creation Screen Background ===")
    gen(pipe,
        "mystical wizard study interior, ancient bookshelves, glowing crystal ball, "
        "candlelight, arcane symbols on walls, star map on ceiling, "
        "fantasy game menu background, dark atmospheric, digital painting",
        os.path.join(OUTPUT_DIR, "creation_background.png"))

    del pipe
    gc.collect()
    torch.cuda.empty_cache()

    total = len([f for f in os.listdir(OUTPUT_DIR) if f.endswith('.png')])
    print(f"\n=== Done — {total} wizard art assets ===")

if __name__ == "__main__":
    main()
