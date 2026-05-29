"""Generate portraits for the 4 race-gated engineer units with SDXL on the dev
server. Same painterly fantasy bestiary style as the summon portraits so the
unit-card art reads consistently across unit types.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/engineers")
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 1024, 768   # 4:3 to fit the unit-card portrait area
STEPS = 36
CFG = 7.5

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, cropped, multiple panels, out of frame"
)

STYLE = (
    "fantasy unit portrait, single subject centered, dramatic painterly oil "
    "illustration, dark atmospheric background, volumetric light, Master of "
    "Magic unit-roster art style, highly detailed, rich colour, no text, "
    "no border"
)

ENGINEERS = [
    ("Dwarves_Engineer",
     "a stout dwarven engineer with thick braided red beard, in worn leather "
     "and bronze-studded armour, carrying a heavy ironwork hammer and chisel, "
     "covered in soot from a forge, standing in a torchlit underground hall", 8001),
    ("HighMen_Engineer",
     "a human engineer in practical leather jerkin and chain accents, holding "
     "a brass theodolite and a parchment scroll of plans, focused expression, "
     "stone keep battlements in the background, golden hour light", 8002),
    ("Beastmen_Engineer",
     "a hulking horned beastman engineer in studded leather with a heavy "
     "stonework mallet, shaping a massive block at a torchlit ramp, snorting "
     "with effort, dust and sparks in the air", 8003),
    ("Lizardmen_Engineer",
     "a scaled lizardman engineer in copper-plated armour with stonework tools "
     "carved from green jade, working masonry beside a steaming swamp pool, "
     "low torches and humid mist", 8004),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print(f"Pipeline ready. Generating {len(ENGINEERS)} engineer portraits.\n")

    for spec, fragment, seed in ENGINEERS:
        out_path = os.path.join(OUT_DIR, f"{spec}.png")
        if os.path.exists(out_path):
            print(f"[{spec}] exists, skipping")
            continue
        prompt = f"{fragment}, {STYLE}"
        print(f"[{spec}] seed={seed} {W}x{H}")
        gen = torch.Generator("cuda").manual_seed(seed)
        result = pipe(prompt=prompt, negative_prompt=NEG, width=W, height=H,
                      num_inference_steps=STEPS, guidance_scale=CFG, generator=gen)
        result.images[0].save(out_path)
        print(f"   -> {out_path} ({os.path.getsize(out_path)//1024} KB)")
    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
