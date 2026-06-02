"""Generate ~24 item portraits for the CoM-style item-enchantment system on
SDXL. Each category gets 2-3 variants so the forge widget can pick a
distinct picture per slot + power class. Saves to ~/item_art/<name>.png.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR    = os.path.expanduser("~/item_art")
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 768, 1024  # tall portrait
STEPS = 32
CFG = 7.0

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame, people, hands"
)

STYLE = (
    "fantasy artifact illustration, single isolated object centered, "
    "dramatic painterly oil illustration, Master of Magic item-roster art, "
    "dark velvet display background, soft volumetric light, highly detailed, "
    "no text, no border, no person"
)

ITEMS = [
    ("sword_01",     "an ornate medieval longsword with engraved silver blade and gold-wrapped grip"),
    ("sword_02",     "a curved fantasy scimitar with damascus steel pattern, ruby-set hilt"),
    ("sword_03",     "a two-handed flamberge greatsword with wavy blade and dragon-head pommel"),
    ("axe_01",       "a heavy double-bladed war axe with iron-banded haft and runic etching"),
    ("axe_02",       "a single-bladed bearded axe with leather-wrapped handle and bone inlay"),
    ("mace_01",      "a flanged steel mace with spiked head and reinforced shaft"),
    ("mace_02",      "a ceremonial gold-and-jewel-encrusted mace, royal regalia look"),
    ("bow_01",       "a recurve elven longbow of polished yew, silver tracery"),
    ("bow_02",       "a heavy composite bow of horn and sinew, dark gnarled wood"),
    ("staff_01",     "a tall wizard staff with crystal orb at the top and runic carvings"),
    ("staff_02",     "a gnarled druidic staff of living wood with curling vines and leaves"),
    ("wand_01",      "a slender ebony wand tipped with a glowing violet gem"),
    ("armor_chain",  "a sleeveless chainmail hauberk over padded leather"),
    ("armor_plate",  "an ornate full plate cuirass with engraved breastplate"),
    ("armor_robe",   "a wizard's enchanted robe of midnight blue with silver star embroidery"),
    ("helm_01",      "a horned steel helm with cheek guards"),
    ("helm_02",      "a circlet crown of gold with embedded sapphires"),
    ("boots_01",     "leather riding boots with iron-buckled straps"),
    ("boots_02",     "elegant silver-tipped enchanted boots with feathered wings on the heels"),
    ("shield_01",    "a kite shield with heraldic dragon emblem"),
    ("shield_02",    "a round buckler shield of polished bronze"),
    ("ring_01",      "an ornate gold ring with a glowing emerald cabochon"),
    ("ring_02",      "a silver ring with twisted serpent band and ruby eyes"),
    ("ring_03",      "a wide platinum ring with sapphire and runic engraving"),
    ("amulet_01",    "a pendant amulet with a teardrop crystal in gold filigree"),
    ("amulet_02",    "a bone-and-feather shaman's amulet with raw turquoise"),
    ("amulet_03",    "a holy silver amulet with sunburst and emerald centerpiece"),
    ("cloak_01",     "a flowing crimson cloak with gold trim, draped on a stand"),
    ("cloak_02",     "an indigo wizard's cloak with star-patterned hem"),
    ("orb_01",       "a perfect crystal orb on a brass tripod, glowing softly"),
    ("orb_02",       "a swirling black-and-purple sorcery orb on an obsidian base"),
]


def seed_for(name: str) -> int:
    return 60000 + (hash(name) & 0xFFFF)


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)

    for name, frag in ITEMS:
        out = os.path.join(OUT_DIR, f"{name}.png")
        if os.path.exists(out):
            print(f"[{name}] exists, skipping")
            continue
        seed = seed_for(name)
        prompt = f"{frag}, {STYLE}"
        print(f"[{name}] seed={seed}")
        gen = torch.Generator("cuda").manual_seed(seed)
        res = pipe(prompt=prompt, negative_prompt=NEG, width=W, height=H,
                   num_inference_steps=STEPS, guidance_scale=CFG, generator=gen)
        res.images[0].save(out)
        print(f"   -> {out} ({os.path.getsize(out)//1024} KB)")

    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
