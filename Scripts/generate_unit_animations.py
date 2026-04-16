"""
Generate unit movement animation sprite sheets.
Each unit type gets an 8-frame horizontal sprite sheet for movement.
Types: infantry march, cavalry gallop, flying hover, naval sailing.
Also generates per-race unit idle sprites.
"""
import torch
from diffusers import AutoPipelineForText2Image
from PIL import Image, ImageEnhance
import os, gc

OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\unit_animations"

MOVEMENT_TYPES = [
    # (name, prompt for each frame phase)
    ("infantry_march", "medieval fantasy foot soldier marching, side view, armor and sword, walking animation"),
    ("cavalry_gallop", "armored knight on horseback galloping, side view, lance and shield, horse running"),
    ("flying_hover", "winged fantasy creature flying, side view, wings spread, hovering animation"),
    ("naval_sailing", "fantasy war galley sailing on water, side view, sails and oars, moving forward"),
    ("siege_move", "large wooden siege tower being pushed, side view, wheels rolling, soldiers pushing"),
    ("settler_walk", "medieval settler with cart and supplies walking, side view, traveling"),
    ("scout_run", "fast scout running, side view, light armor, bow on back, quick movement"),
    ("dragon_fly", "large dragon flying, side view, wings flapping, breathing fire, majestic"),
    ("undead_shamble", "skeleton warrior shambling forward, side view, ragged armor, lurching gait"),
    ("mage_float", "wizard floating above ground, side view, robes flowing, magical energy trail"),
]

RACE_UNITS = [
    ("human_swordsman", "human medieval swordsman standing ready, front view, steel armor, sword and shield"),
    ("elf_archer", "elven archer standing ready, front view, elegant armor, longbow drawn"),
    ("dwarf_hammerer", "dwarven warrior standing ready, front view, heavy plate, war hammer"),
    ("orc_berserker", "orc berserker standing ready, front view, crude armor, two axes"),
    ("dark_elf_assassin", "dark elf assassin standing ready, front view, black leather, twin daggers"),
    ("demon_soldier", "demon warrior standing ready, front view, hellforged armor, flaming sword"),
    ("undead_skeleton", "skeleton warrior standing ready, front view, rusted armor, ancient sword"),
    ("troll_warrior", "troll warrior standing ready, front view, hide armor, massive club"),
    ("celestial_guardian", "angelic celestial warrior standing ready, front view, golden armor, holy sword"),
    ("dragon_knight", "dragonborn knight standing ready, front view, scaled armor, great sword"),
]

def gen_sheet(pipe, base_prompt, output_path, frames=8, frame_size=128):
    """Generate an 8-frame animation sprite sheet."""
    if os.path.exists(output_path):
        return

    sheet = Image.new("RGBA", (frame_size * frames, frame_size), (0, 0, 0, 0))
    phases = ["starting", "stepping forward", "mid stride", "full stride",
              "stepping forward", "mid stride", "returning", "at rest"]

    for f in range(frames):
        prompt = f"{base_prompt}, {phases[f]}, frame {f+1}, pixel art style game sprite, dark background"
        img = pipe(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                   width=512, height=512).images[0]
        img = img.resize((frame_size, frame_size), Image.LANCZOS)
        img = ImageEnhance.Color(img).enhance(1.3)

        # Make dark background transparent
        img = img.convert("RGBA")
        pixels = img.load()
        for y in range(frame_size):
            for x in range(frame_size):
                r, g, b, a = pixels[x, y]
                brightness = (r + g + b) / 3
                if brightness < 25:
                    pixels[x, y] = (r, g, b, 0)
                elif brightness < 50:
                    pixels[x, y] = (r, g, b, int(brightness * 5))

        sheet.paste(img, (f * frame_size, 0), img)

    sheet.save(output_path, "PNG")

def gen_single(pipe, prompt, output_path, size=128):
    """Generate a single unit sprite."""
    if os.path.exists(output_path):
        return
    full_prompt = f"{prompt}, game unit sprite, detailed, dark background, fantasy RPG"
    img = pipe(prompt=full_prompt, num_inference_steps=4, guidance_scale=0.0,
               width=512, height=512).images[0]
    img = img.resize((size, size), Image.LANCZOS)
    img = ImageEnhance.Color(img).enhance(1.3)
    img.save(output_path, "PNG")

def main():
    print("=== Loading SD Turbo ===")
    pipe = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe = pipe.to("cuda")
    pipe.enable_attention_slicing()

    os.makedirs(os.path.join(OUTPUT, "movement"), exist_ok=True)
    os.makedirs(os.path.join(OUTPUT, "units"), exist_ok=True)

    # Movement animations (8-frame sprite sheets)
    print("\n=== Movement Animations ===")
    for name, prompt in MOVEMENT_TYPES:
        print(f"  {name}...")
        gen_sheet(pipe, prompt, os.path.join(OUTPUT, "movement", f"{name}_sheet.png"))

    # Race unit idle sprites
    print("\n=== Race Unit Sprites ===")
    for name, prompt in RACE_UNITS:
        print(f"  {name}...")
        gen_single(pipe, prompt, os.path.join(OUTPUT, "units", f"{name}.png"))

    del pipe; gc.collect(); torch.cuda.empty_cache()

    move_count = len([f for f in os.listdir(os.path.join(OUTPUT, "movement")) if f.endswith('.png')])
    unit_count = len([f for f in os.listdir(os.path.join(OUTPUT, "units")) if f.endswith('.png')])
    print(f"\n=== Done — {move_count} movement sheets + {unit_count} unit sprites ===")

if __name__ == "__main__":
    main()
