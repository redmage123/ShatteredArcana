"""Generate 14 wizard combat backgrounds with SDXL on the dev server.

Each wizard gets a 1920x1080 widescreen scene of them in combat, using
their distinctive school of magic. Output is dropped in
~/wizard-bgs/ as wizard_NN.png (1-indexed to match the UE asset
naming convention used by CoMWizardConfigWidget).
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/wizard-bgs")
os.makedirs(OUT_DIR, exist_ok=True)

# Match the main-menu war-in-heaven painting: 1920x1080 baroque oil on canvas.
W, H = 1920, 1080
STEPS = 38
CFG = 8.0

NEG = (
    "ugly, deformed, disfigured, mutated, low quality, low resolution, "
    "blurry, jpeg artifacts, watermark, signature, text, logo, frame, border, "
    "cartoon, anime, illustration, childish, cute, modern clothing, photo, "
    "photograph, render, 3d render, cgi, plastic, two heads, extra limbs, "
    "extra fingers, bad anatomy, bad hands, out of frame, cropped, duplicate"
)

# Style locked to match the menu background: Baroque oil-painting battle
# scene in the manner of Rembrandt / Caravaggio / Rubens (war-in-heaven).
STYLE = (
    "baroque oil painting on canvas, in the style of Rembrandt and Caravaggio "
    "and Rubens, war in heaven composition, dramatic tenebrist chiaroscuro, "
    "golden warm key light against deep cool shadow, classical fine art "
    "masterpiece, museum quality, ornate flowing drapery, heroic figural "
    "composition, ultra-detailed brushwork, rich oil pigments, varnished "
    "canvas texture, 17th century European fine art, no text, no border"
)

WIZARDS = [
    # (idx, name, prompt-fragment, seed)
    (1, "Merlin",
     "Merlin the Court Mage, elderly white-bearded archmage in flowing royal-blue robes "
     "embroidered with silver runes, raising a wooden staff topped with a glowing crystal, "
     "casting an arcane shield of luminous geometric runes around himself, "
     "ruined throne hall battlefield, golden order magic radiating outward, "
     "shafts of dawn light through shattered cathedral windows", 4201),

    (2, "Morgana",
     "Morgana the Betrayed, raven-haired sorceress in tattered black silk and obsidian armor, "
     "tears of crimson light streaking down her pale face, hurling a wave of black death magic "
     "with both hands, smoke wraiths and spectral skulls swirling around her, "
     "burning ruined courtyard at twilight, embers and ash in air", 4202),

    (3, "Zephyros",
     "Zephyros the Stormborn, lean wizard with windswept silver hair in pale violet robes "
     "etched with cloud sigils, standing at center of a raging planar storm vortex, "
     "lightning arcing between his outstretched hands, illusory copies of himself flickering "
     "in the rain, mountain peak battlefield, chromatic sky with three distant moons", 4203),

    (4, "Hecate",
     "Hecate the Veil-Walker, hooded crone-priestess in deep purple shrouds with bone fetishes, "
     "eyes glowing white, arms raised invoking spirits, translucent ghosts of the dead pouring "
     "from a torn dimensional rift behind her, graveyard battlefield at midnight under "
     "blood moon, pale green necromantic glow", 4204),

    (5, "Malachar",
     "Malachar the Soul-Bound, bald sharp-featured warlock in black-and-gold contract robes "
     "covered in chains and binding sigils, hovering above the ground, summoning a massive "
     "demonic pact-circle of fiery red runes beneath his feet, infernal pact-binding magic, "
     "obsidian temple at dusk, sulfurous orange clouds", 4205),

    (6, "Lunara",
     "Lunara the Glamour-Weaver, ethereal fey enchantress with butterfly-wing cloak and silver "
     "antler crown, dancing barefoot in a ring of phosphorescent mushrooms, waves of iridescent "
     "glamour magic rippling outward distorting reality, twin moons through mist, "
     "enchanted Feywild glade battlefield, fireflies and dream-petals", 4206),

    (7, "Grimnar",
     "Grimnar the Runesmith, broad-shouldered dwarven battle-mage with braided red beard in "
     "iron-and-leather rune-armor, swinging a glowing rune-hammer that strikes the stone earth "
     "and erupts golden green nature-runes outward, mountain pass battlefield, "
     "ancestral stone giants in background, snowy peaks and aurora", 4207),

    (8, "Nekros",
     "Nekros the Necromancer, gaunt cadaverous lich-mage in moldering charcoal robes, skeletal "
     "fingers raising an army of undead warriors from a fog-shrouded battlefield, sickly green "
     "soul-fire pouring from his eyes and mouth, broken tombstones and bone shards, "
     "moonlit graveyard, rotting fog", 4208),

    (9, "Gaia",
     "Gaia the Verdant Voice, dark-skinned druid-priestess in living vine robes with flowering "
     "hair, eyes glowing emerald, conjuring a colossal animated tree-guardian behind her by "
     "raising both hands, golden pollen and butterflies swirling, primeval forest battlefield, "
     "shafts of sunlight through ancient canopy", 4209),

    (10, "Pyraxis",
     "Pyraxis the Scorcher, fire-eyed berserker mage in scorched red-and-bronze armor with "
     "molten cracks, hurling a roaring fireball from one hand while a phoenix of flame circles "
     "him, volcanic battlefield with rivers of lava, ash sky, embers raining", 4210),

    (11, "Glaciel",
     "Glaciel the Frostqueen, regal pale sorceress with platinum braid in white-and-ice-blue "
     "robes and a crown of frozen spikes, exhaling a torrent of ice and time-stopping frost, "
     "crystalline ice spikes erupting from the ground, frozen tundra battlefield at "
     "twilight, aurora borealis above", 4211),

    (12, "Aldric",
     "Aldric the Scholar, kindly old archivist-mage with round spectacles and a deep grey "
     "scholarly robe lined with arcane bookmarks, surrounded by floating glowing tomes and "
     "spinning rings of script, casting a luminous golden Arcane spell circle, "
     "ancient library battlefield with broken pillars, dust motes in shafts of light", 4212),

    (13, "Lilith",
     "Lilith the Abyssal Pact-Mistress, beautiful crimson-haired demoness-mage in obsidian "
     "armor with infernal red-violet runes and folded leathery wings, summoning a binding "
     "contract of crimson chains that bind a chained demon behind her, abyssal cathedral "
     "battlefield, hellfire glow, dark cathedral arches", 4213),

    (14, "Solarius",
     "Solarius the Lightbringer, golden-armored paladin-archmage with halo of pure sunlight, "
     "raising a radiant longsword that emits a vertical pillar of holy fire, smiting an "
     "unseen enemy, divine light burning away shadows, mountaintop temple battlefield "
     "at sunrise, golden clouds and sunbeams", 4214),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH,
        torch_dtype=torch.float16,
        use_safetensors=True,
        variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print("Pipeline ready.\n")

    for idx, name, fragment, seed in WIZARDS:
        out_path = os.path.join(OUT_DIR, f"wizard_{idx:02d}.png")
        if os.path.exists(out_path):
            print(f"[{idx:02d}] {name}: already exists, skipping")
            continue
        prompt = f"{fragment}, {STYLE}"
        print(f"[{idx:02d}] {name}  seed={seed}  {W}x{H}  steps={STEPS}")
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(
            prompt=prompt,
            negative_prompt=NEG,
            width=W, height=H,
            num_inference_steps=STEPS,
            guidance_scale=CFG,
            generator=gen,
        )
        img = result.images[0]
        img.save(out_path)
        kb = os.path.getsize(out_path) // 1024
        print(f"     -> {out_path}  ({kb} KB)")
    print("\nAll wizard backgrounds generated.")


if __name__ == "__main__":
    main()
