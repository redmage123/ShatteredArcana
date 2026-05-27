"""Generate the CoM-style spell book background with SDXL on the dev server.

An open leather-bound tome seen top-down with two blank aged-parchment pages,
so the UI can overlay the spell list on the left page and spell details on the
right page (with page-turn arrows). Landscape. Output in ~/spellbook/.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/spellbook")
os.makedirs(OUT_DIR, exist_ok=True)

# Landscape, ~3:2, SDXL-friendly.
W, H = 1536, 1024
STEPS = 38
CFG = 7.5

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, modern, photo, 3d render, cgi, text, letters, words, "
    "writing, gibberish text, hands, fingers, person, cropped, tilted, perspective"
)

STYLE = (
    "ancient open spellbook lying flat seen directly from above, two large blank "
    "aged parchment pages, ornate gold-tooled dark-leather binding and central "
    "spine, decorative gold filigree corners and arcane border motifs on the "
    "pages, faint mystical watermark, warm candlelight, rich fantasy oil "
    "painting, painterly, symmetrical centered composition, empty pages ready "
    "for handwritten spells, museum quality, intricate detail"
)

VARIANTS = [
    ("spellbook_bg", 6101),
    ("spellbook_bg_alt", 6102),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print("Pipeline ready.\n")

    for slug, seed in VARIANTS:
        out_path = os.path.join(OUT_DIR, f"{slug}.png")
        if os.path.exists(out_path):
            print(f"[{slug}] exists, skipping")
            continue
        print(f"[{slug}] seed={seed} {W}x{H} steps={STEPS}")
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(
            prompt=STYLE, negative_prompt=NEG, width=W, height=H,
            num_inference_steps=STEPS, guidance_scale=CFG, generator=gen,
        )
        result.images[0].save(out_path)
        print(f"   -> {out_path} ({os.path.getsize(out_path)//1024} KB)")
    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
