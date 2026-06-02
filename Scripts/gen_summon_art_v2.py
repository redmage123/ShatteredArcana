"""Generate portraits for the full CoM summonable roster (port pass).
Skips creatures already on disk. Matches the painterly bestiary style of
the earlier 9 summon portraits. Output in ~/summons/ as <SpecID>.png.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/summons")
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 1024, 768
STEPS = 36
CFG = 7.5

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, multiple panels, out of frame, cropped"
)

STYLE = (
    "fantasy creature bestiary portrait, single subject centered, dramatic "
    "painterly oil illustration, dark atmospheric background, volumetric light, "
    "Master of Magic creature art style, highly detailed, rich colour, "
    "ominous and heroic, no text, no border"
)

CREATURES = [
    # ── Life realm ────────────────────────────────────────────────────────
    ("Summon_Unicorns", "a herd of three majestic white unicorns with golden horns and "
     "flowing manes, glowing aura of holy light, gentle and noble", 7101),
    ("Summon_ArchAngel", "a towering archangel in radiant golden armor with four feathered "
     "wings, wielding a flaming greatsword, halo of divine fire, awe-inspiring", 7102),

    # ── Death realm ───────────────────────────────────────────────────────
    ("Summon_Ghouls", "a pack of feral undead ghouls with grey rotting flesh, hunched and "
     "clawed, lurking in graveyard mist, hungry and savage", 7103),
    ("Summon_Werewolves", "a pack of hulking werewolves with shaggy black fur and yellow "
     "fangs, eyes glowing red, mid-howl under a blood moon", 7104),
    ("Summon_NightStalker", "a tall lanky undead night stalker with pale dead skin, hollow "
     "black eye sockets, long curved claws, shrouded in darkness", 7105),
    ("Summon_ShadowDemons", "a flock of shadow demons made of living darkness with red "
     "burning eyes, batlike wings of pure shadow, malevolent", 7106),
    ("Summon_DemonLord", "a colossal infernal demon lord with red scaled skin, two great "
     "ram horns, burning eyes, wielding a flaming greataxe, sulphurous", 7107),
    ("Summon_DeathKnights", "three undead death knights in spiked black armor with skull "
     "helmets, glowing green soul fire eyes, mounted on skeletal horses", 7108),

    # ── Chaos realm ───────────────────────────────────────────────────────
    ("Summon_FireElemental", "a writhing humanoid figure made entirely of living flame and "
     "molten lava, glowing orange and red, primal elemental fury", 7109),
    ("Summon_FireGiant", "a massive towering fire giant with cracked obsidian skin glowing "
     "with internal magma, wielding a flaming greatclub, volcanic", 7110),
    ("Summon_DoomBat", "an enormous nightmarish bat with leathery wings and razor fangs, "
     "glowing red eyes, swooping through smoky skies, terrifying", 7111),
    ("Summon_Chimera", "a chimera with the body of a lion, the head of a snarling goat, and "
     "a hissing serpent tail, breathing fire, monstrous", 7112),
    ("Summon_Efreet", "a powerful efreeti genie of fire with red skin, golden jewelry and "
     "turban, wreathed in flame, emerging from a column of smoke", 7113),
    ("Summon_Hydra", "a massive multi-headed hydra with five reptilian serpent heads, green "
     "scales, snarling and dripping venom, swamp emerging", 7114),
    ("Summon_ChaosSpawn", "a writhing tentacled chaos spawn abomination with many mouths "
     "and eyes, mutated flesh, otherworldly and grotesque", 7115),

    # ── Nature realm ──────────────────────────────────────────────────────
    ("Summon_Sprites", "a swarm of tiny winged forest sprites with glowing translucent "
     "dragonfly wings, mischievous little fey faces, sparkling magic dust", 7116),
    ("Summon_Cockatrices", "a pair of bizarre cockatrices with rooster heads, reptilian "
     "wings and serpent tails, beady yellow eyes that petrify", 7117),
    ("Summon_Basilisk", "a giant eight-legged basilisk lizard with venomous green scales "
     "and glowing yellow eyes, lurking in mossy stone ruins", 7118),
    ("Summon_StoneGiant", "a colossal stone giant carved from living granite, massive "
     "boulder fists, mossy with age, ponderous and immovable", 7119),
    ("Summon_EarthElemental", "a hulking humanoid figure made entirely of compacted earth "
     "and embedded stones, glowing crystal eyes, primal nature spirit", 7120),
    ("Summon_Gorgons", "two armored gorgons with horned bull heads and metallic hides, "
     "exhaling clouds of petrifying breath, savage and ancient", 7121),
    ("Summon_Behemoth", "an enormous quadrupedal behemoth with thick grey hide, massive "
     "tusks and curling horns, ground-shaking and ancient", 7122),
    ("Summon_Colossus", "a towering colossus statue come to life, polished bronze armored "
     "figure of immense stature, glowing rune eyes, monumental", 7123),
    ("Summon_GreatWyrm", "a vast ancient earth wyrm with serpentine green-scaled body, "
     "horned reptilian head, glowing emerald eyes, primordial", 7124),

    # ── Sorcery realm ─────────────────────────────────────────────────────
    ("Summon_PhantomBeast", "a translucent ghostly tiger of pure spectral energy, "
     "shimmering pale blue, ethereal and predatory, illusory", 7125),
    ("Summon_AirElemental", "a swirling vortex humanoid of wind and storm clouds, crackling "
     "with lightning, body of living tempest, ephemeral", 7126),
    ("Summon_Nagas", "two serpent-bodied naga warriors with human upper bodies and snake "
     "lower halves, wielding curved scimitars, exotic and cunning", 7127),
    ("Summon_StormGiant", "a towering storm giant with stormy blue skin and white beard, "
     "lightning crackling around fists, wielding a thunderhammer", 7128),
    ("Summon_Djinn", "a powerful djinn genie of air with blue skin, flowing turban and "
     "robes, lower body trailing into wind, regal and mysterious", 7129),
    ("Summon_SkyDrake", "a magnificent sky drake dragon with sapphire scales and feathery "
     "blue wings, lightning crackling along its body, soaring high", 7130),

    # ── Arcane realm ──────────────────────────────────────────────────────
    ("Summon_MagicSpirit", "a small floating wisp of arcane silver-blue light, ethereal "
     "spirit of pure magic, drifting through the air, ghostly", 7131),
    ("Summon_FloatingIsland", "a small floating island of grass and rock hovering in the "
     "sky, with a single ancient tree and tumbling waterfall, magical", 7132),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print(f"Pipeline ready. Generating {len(CREATURES)} summon portraits.\n")

    for spec, fragment, seed in CREATURES:
        out_path = os.path.join(OUT_DIR, f"{spec}.png")
        if os.path.exists(out_path):
            print(f"[{spec}] exists, skipping")
            continue
        prompt = f"{fragment}, {STYLE}"
        print(f"[{spec}] seed={seed} {W}x{H}", flush=True)
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(prompt=prompt, negative_prompt=NEG, width=W, height=H,
                      num_inference_steps=STEPS, guidance_scale=CFG, generator=gen)
        result.images[0].save(out_path)
        print(f"   -> {out_path} ({os.path.getsize(out_path)//1024} KB)", flush=True)
    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
