"""Generate bespoke sprite sheets for 12 iconic spells. Each gets its own
keyframe (SDXL 768x768) and an 8-frame burst-and-fade strip post-processed
with PIL, matching CoMSpellVFXSubsystem's existing frame timing.

Sheet path convention: /Game/Textures/SpellVFX/iconic/<slug>_sheet
The override table in CoMMagicSubsystem maps SpellID -> "iconic.<slug>".
"""
import os
import torch
from PIL import Image
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR    = os.path.expanduser("~/spellvfx/iconic_keys")
SHEET_DIR  = os.path.expanduser("~/spellvfx/iconic_sheets")
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(SHEET_DIR, exist_ok=True)

FRAME = 512
FRAMES = 8

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame, characters, "
    "people, animals, buildings"
)

STYLE = (
    "centered magical phenomenon, dramatic painterly oil illustration, "
    "Master of Magic spell effect, abstract energy, pure black background, "
    "no text, no border, highly detailed, vivid colour"
)

# (slug, prompt fragment).  Slug maps 1:1 to "iconic.<slug>" effect ID.
SPELLS = [
    ("spell_of_mastery",
     "a cosmic vortex of pure golden-white light with concentric rings of "
     "floating arcane runes spiralling toward a brilliant central singularity, "
     "sense of universe-altering culmination"),
    ("just_cause",
     "a radiant pillar of pure golden divine justice descending from above, "
     "scales of judgment overlaid in luminous light, holy aura"),
    ("eternal_night",
     "a vast swirling pool of black shadow swallowing pinprick stars, "
     "ghostly mist tendrils with cold violet edge"),
    ("armageddon",
     "a sky raining fire and meteors over a cracked landscape, towering "
     "ash storm wall, blood-red sky with falling stars, world-ending dread"),
    ("call_the_void",
     "a jagged abyssal rift torn open in space with violet-black void energy "
     "and shattering crystalline shards spilling from a swirling event horizon"),
    ("gaia_force",
     "a colossal emerald world-tree silhouette with golden life-force motes "
     "streaming through the air, lush vines and leaves whirling outward"),
    ("heavenly_light",
     "a single bright golden sunbeam descending from a parted cloudburst, "
     "shimmering motes of holy light, warm radiant glow"),
    ("wall_of_fire",
     "a tall curving wall of roaring orange-red flame with embers and sparks, "
     "intense firestorm wall, smoke rising from the base"),
    ("suppress_magic",
     "a dampening web of dim cobalt-blue veil energy with broken arcane sigils "
     "fading into static, smothering magical light"),
    ("flying_fortress",
     "a vast stone-and-crystal floating citadel silhouette wreathed in storm "
     "clouds and lightning, blue-violet magical updraft"),
    ("volcano",
     "an erupting volcano with billowing pyroclastic plume, glowing lava "
     "fountains and red-hot ash, dramatic black smoke column"),
    ("great_tree",
     "an enormous mythic world-tree with golden glowing canopy, vines and "
     "wildflowers swirling outward, vibrant deep green and gold magical aura"),
]


def make_sheet(src_img: Image.Image, sheet_path: str):
    """Build an 8-frame burst-and-fade horizontal strip from a single keyframe."""
    src = src_img.convert("RGBA").resize((FRAME, FRAME), Image.LANCZOS)
    sheet = Image.new("RGBA", (FRAME * FRAMES, FRAME), (0, 0, 0, 0))
    for i in range(FRAMES):
        t = i / (FRAMES - 1)
        scale = 0.55 + 0.7 * t
        alpha = max(0, min(255, int(255 * (1.0 - abs(t - 0.35) * 1.45))))
        sz = max(2, int(FRAME * scale))
        scaled = src.resize((sz, sz), Image.LANCZOS)
        a = scaled.split()[3].point(lambda p: int(p * alpha / 255))
        scaled.putalpha(a)
        ox = i * FRAME + (FRAME - sz) // 2
        oy = (FRAME - sz) // 2
        sheet.alpha_composite(scaled, (ox, oy))
    sheet.save(sheet_path)


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)

    for slug, frag in SPELLS:
        key = os.path.join(OUT_DIR, f"{slug}.png")
        sheet = os.path.join(SHEET_DIR, f"{slug}_sheet.png")
        if os.path.exists(sheet):
            print(f"[{slug}] exists, skipping")
            continue
        if not os.path.exists(key):
            prompt = f"{frag}, {STYLE}"
            seed = 40000 + (hash(slug) & 0xFFFF)
            print(f"[{slug}] gen key seed={seed}")
            gen = torch.Generator("cuda").manual_seed(seed)
            res = pipe(prompt=prompt, negative_prompt=NEG, width=768, height=768,
                       num_inference_steps=32, guidance_scale=7.0, generator=gen)
            res.images[0].save(key)
        print(f"[{slug}] strip -> {sheet}")
        make_sheet(Image.open(key), sheet)

    print(f"\nDone. {SHEET_DIR}")


if __name__ == "__main__":
    main()
