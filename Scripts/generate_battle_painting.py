"""
Generate high-res: Angelic hosts defeating the Abyssal army.
"""

import torch
from diffusers import AutoPipelineForText2Image, AutoPipelineForImage2Image
from PIL import Image, ImageEnhance, ImageFilter
import os
import gc

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Content\Textures\UI"

def main():
    print("=== Loading SD Turbo ===")

    pipe_t2i = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo",
        torch_dtype=torch.float16,
        variant="fp16",
    )
    pipe_t2i = pipe_t2i.to("cuda")
    pipe_t2i.enable_attention_slicing()
    pipe_i2i = AutoPipelineForImage2Image.from_pipe(pipe_t2i)

    prompt = (
        "Triumphant angelic host descending from golden heavens, "
        "crushing a dark demonic abyssal army below, "
        "winged celestial warriors in radiant white and gold armor, "
        "holy swords and divine light rays piercing through darkness, "
        "defeated demons falling and fleeing in terror, "
        "epic modern fantasy concept art, cinematic, "
        "vibrant bright colors, professional digital painting, "
        "artstation masterpiece, 4k ultra detailed"
    )

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print("=== Generating 6 high-res variations ===")
    for i in range(6):
        print(f"  {i+1}/6 — base...")
        base = pipe_t2i(
            prompt=prompt,
            num_inference_steps=4,
            guidance_scale=0.0,
            width=512,
            height=512,
        ).images[0]

        print(f"  {i+1}/6 — detail pass...")
        base_2x = base.resize((1024, 1024), Image.LANCZOS)
        hires = pipe_i2i(
            prompt=prompt,
            image=base_2x,
            num_inference_steps=4,
            guidance_scale=0.0,
            strength=0.35,
            width=1024,
            height=1024,
        ).images[0]

        hires = hires.resize((1920, 1080), Image.LANCZOS)
        hires = hires.filter(ImageFilter.SHARPEN)
        hires = ImageEnhance.Sharpness(hires).enhance(1.3)
        hires = ImageEnhance.Color(hires).enhance(1.15)
        hires = ImageEnhance.Contrast(hires).enhance(1.08)

        path = os.path.join(OUTPUT_DIR, f"T_MenuBackground_v{i+1}.png")
        hires.save(path, "PNG", optimize=True)
        print(f"  Saved: {path}")

    del pipe_t2i, pipe_i2i
    gc.collect()
    torch.cuda.empty_cache()
    print("=== Done ===")

if __name__ == "__main__":
    main()
