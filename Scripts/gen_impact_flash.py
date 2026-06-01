"""Generate 9 per-realm impact_flash sprite sheets at /Game/Textures/SpellVFX/<realm>/impact_flash_sheet.

The CoMSpellVFXSubsystem registers one impact_flash per realm with 8 frames at
0.1s each — the standard burst-and-fade impact hit fired alongside every cast.
We approximate this as a single keyframe per realm (a radial energy burst in
the realm's colour) post-processed into an 8-frame strip on the dev server,
then imported as a single sprite sheet texture.
"""
import os
import torch
from PIL import Image
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR    = os.path.expanduser("~/spellvfx/impact_flash")
SHEET_DIR  = os.path.expanduser("~/spellvfx/sheets")
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(SHEET_DIR, exist_ok=True)

FRAME = 512   # per-frame square
FRAMES = 8

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame, characters, "
    "people, animals, buildings, weapons"
)

STYLE = (
    "centered radial magical energy burst, dramatic painterly oil illustration, "
    "Master of Magic spell impact effect, glowing radial rays, swirling energy, "
    "abstract magical phenomenon, pure black background, no text, no border, "
    "highly detailed, vivid colour"
)

REALMS = [
    ("life",    "warm radiant golden-white holy light burst with rays of pure sunlight"),
    ("death",   "necrotic green-and-black shadow burst with swirling ghostly tendrils"),
    ("chaos",   "explosive orange-red firestorm burst with embers and crackling flame"),
    ("nature",  "vibrant emerald-green vine burst with whirling leaves and earth dust"),
    ("sorcery", "icy electric-blue lightning burst with arcing bolts and crystalline shards"),
    ("arcane",  "violet-and-silver arcane sigil burst with mystical glyphs spiralling outward"),
    ("binding", "ash-grey-and-bronze chain burst with iron-link tendrils whirling outward"),
    ("spirit",  "pale silver-white ethereal mist burst with ghostly luminous wisps"),
    ("glamour", "iridescent pink-and-teal prismatic burst with shimmering mirage shards"),
]


def make_sheet(src_img: Image.Image, sheet_path: str):
    """Build an 8-frame horizontal strip from a single keyframe: a radial
    expand-and-fade animation that reads correctly when played at 10fps."""
    src = src_img.convert("RGBA").resize((FRAME, FRAME), Image.LANCZOS)
    sheet = Image.new("RGBA", (FRAME * FRAMES, FRAME), (0, 0, 0, 0))
    for i in range(FRAMES):
        t = i / (FRAMES - 1)  # 0..1
        # Burst curve: scale rises from 0.55 -> 1.25, alpha rises then fades.
        scale = 0.55 + 0.7 * t
        alpha = int(255 * (1.0 - abs(t - 0.35) * 1.45))
        alpha = max(0, min(255, alpha))
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

    for realm, frag in REALMS:
        key = os.path.join(OUT_DIR, f"{realm}.png")
        sheet = os.path.join(SHEET_DIR, f"{realm}_impact_flash_sheet.png")
        if os.path.exists(sheet):
            print(f"[{realm}] sheet exists, skipping")
            continue
        if not os.path.exists(key):
            prompt = f"{frag}, {STYLE}"
            seed = 30000 + (hash(realm) & 0xFFFF)
            print(f"[{realm}] gen key seed={seed}")
            gen = torch.Generator("cuda").manual_seed(seed)
            res = pipe(prompt=prompt, negative_prompt=NEG, width=768, height=768,
                       num_inference_steps=30, guidance_scale=7.0, generator=gen)
            res.images[0].save(key)
        print(f"[{realm}] build 8-frame sheet -> {sheet}")
        make_sheet(Image.open(key), sheet)

    print(f"\nDone. {SHEET_DIR}")


if __name__ == "__main__":
    main()
