"""Generate portraits for all 45 racial units (15 races × Infantry/Cavalry/Ranged)
plus 2 hero classes and the Settler — 48 SDXL portraits, painterly fantasy
bestiary style matching the summon and engineer pass.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/racial_units")
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 1024, 768
STEPS = 36
CFG = 7.5

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame, "
    "extra limbs, bad anatomy"
)

STYLE = (
    "fantasy unit portrait, single subject centered, dramatic painterly oil "
    "illustration, dark atmospheric background, volumetric light, Master of "
    "Magic unit-roster art style, highly detailed, rich colour, no text, "
    "no border"
)

# (race fragment, environment hint) keyed by race tag
RACES = {
    "HighMen":    ("a noble human warrior of a sunlit kingdom, fair skin, clean armour, heraldic emblem",
                   "stone keep battlements at sunrise"),
    "HighElves":  ("a tall elegant high elf with pointed ears, silver-blond hair, refined features, elven craftsmanship",
                   "moonlit silver forest"),
    "Dwarves":    ("a stout broad-shouldered dwarf with a thick braided beard and heavy iron gear",
                   "torchlit deep mountain hall"),
    "Draconians": ("a winged draconian warrior with green-gold scales and reptilian eyes, partial wings folded",
                   "volcanic cliffs and rising smoke"),
    "DarkElves":  ("a pale-skinned dark elf with white hair and obsidian armour, cruel expression",
                   "purple twilight crypt entrance"),
    "Demons":     ("a horned crimson-skinned demon warrior with smouldering eyes and infernal armour",
                   "burning ash plain under red sky"),
    "Merfolk":    ("a teal-skinned merfolk warrior with fin crests, coral and pearl armour, webbed hands",
                   "shallow turquoise lagoon"),
    "Halflings":  ("a small brave halfling with curly hair and oversized practical gear, brave expression",
                   "sunlit green hill country"),
    "Orcs":       ("a hulking green-skinned orc warrior with tusked jaw, scarred hide and crude iron armour",
                   "smoke-belching war camp at dusk"),
    "Gnolls":     ("a hyena-headed gnoll warrior with mangy fur, leather-and-bone armour, snarling muzzle",
                   "scrubland under blood-orange sun"),
    "Lizardmen":  ("a scaled lizardman warrior in copper-plated armour with reptilian features",
                   "steaming swamp with mossy stones"),
    "Undead":     ("a gaunt undead warrior with grey rotting flesh, sunken eyes, tattered shroud over rusted mail",
                   "fog-shrouded graveyard at midnight"),
    "Trolls":     ("a massive grey-green troll warrior with thick hide, blunt tusks and heavy iron-banded club",
                   "moonlit mountain ravine"),
    "Nomads":     ("a tanned desert nomad warrior in flowing white-and-tan robes over light mail, keffiyeh and veil",
                   "golden dune sea at sunset"),
    "Beastmen":   ("a horned beastman warrior with shaggy fur, hooved feet and primitive heavy armour",
                   "ancient stone-circle plain at dusk"),
}

# (unit type, gear fragment)
TYPES = {
    "Infantry": "in heavy melee armour wielding a sword and shield, standing planted in a battle stance",
    "Cavalry":  "mounted on a charging warhorse, lance lowered, banner streaming behind",
    "Ranged":   "drawing a longbow with an arrow nocked, focused archer's stance, quiver at hip",
}

# Deterministic per-unit seed so reruns are stable
def seed_for(spec_id: str) -> int:
    return 9000 + (hash(spec_id) & 0xFFFF)


UNITS = []
for race, (race_frag, env) in RACES.items():
    for utype, gear in TYPES.items():
        spec = f"{race}_{utype}"
        prompt = f"{race_frag}, {gear}, {env}, {STYLE}"
        UNITS.append((spec, prompt, seed_for(spec)))

# Heroes + Settler
UNITS.append((
    "Hero_Fighter",
    "a battle-scarred human hero knight in ornate plate armour and crimson cape, "
    "two-handed greatsword held upright, scarred face and resolute expression, "
    "torch-lit ruined castle gate, " + STYLE,
    seed_for("Hero_Fighter")))
UNITS.append((
    "Hero_Magician",
    "a robed human archmagi hero with long grey beard, glowing staff carved with runes, "
    "arcane sigils swirling in the air around their hand, blue-violet magical light, "
    "ancient library tower interior, " + STYLE,
    seed_for("Hero_Magician")))
UNITS.append((
    "Settler",
    "a peaceful pioneer family with an ox-drawn cart laden with tools and provisions, "
    "father holding reins, mother and child beside, warm hopeful expression, "
    "rolling green frontier landscape under golden hour light, " + STYLE,
    seed_for("Settler")))


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print(f"Pipeline ready. {len(UNITS)} unit portraits to generate.\n")

    for spec, prompt, seed in UNITS:
        out_path = os.path.join(OUT_DIR, f"{spec}.png")
        if os.path.exists(out_path):
            print(f"[{spec}] exists, skipping")
            continue
        print(f"[{spec}] seed={seed} {W}x{H}")
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(prompt=prompt, negative_prompt=NEG, width=W, height=H,
                      num_inference_steps=STEPS, guidance_scale=CFG, generator=gen)
        result.images[0].save(out_path)
        print(f"   -> {out_path} ({os.path.getsize(out_path)//1024} KB)")

    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
