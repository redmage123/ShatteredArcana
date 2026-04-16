"""
Generate leather-bound spell book icons for each realm.
Two states per realm: filled (bright leather) and empty (dim outline).
"""
import torch
from diffusers import AutoPipelineForText2Image
from PIL import Image, ImageEnhance
import os, gc

OUTPUT = r"C:\Users\Braun\repos\ShatteredArcana\Art\GameAssets\processed\ui\books"

REALMS = [
    ("life",    "white and gold leather book, holy symbol, radiant"),
    ("death",   "dark purple leather book, skull emblem, shadowy"),
    ("chaos",   "red and orange leather book, flame symbol, fiery"),
    ("nature",  "green leather book, tree emblem, vine decorations"),
    ("sorcery", "blue leather book, crystal emblem, arcane runes"),
    ("arcane",  "golden leather book, eye symbol, geometric patterns"),
    ("binding", "dark red leather book, chain emblem, iron clasp"),
    ("spirit",  "violet leather book, ghost emblem, ethereal glow"),
    ("glamour", "pink leather book, mirror emblem, sparkle decorations"),
]

def main():
    print("=== Loading SD Turbo ===")
    pipe = AutoPipelineForText2Image.from_pretrained(
        "stabilityai/sd-turbo", torch_dtype=torch.float16, variant="fp16")
    pipe = pipe.to("cuda")
    pipe.enable_attention_slicing()

    os.makedirs(OUTPUT, exist_ok=True)
    base = "single leather bound magic spell book standing upright, thick book with visible page edges, ornate cover, dark background, game icon art, clean isolated object, detailed"

    for realm, desc in REALMS:
        path = os.path.join(OUTPUT, f"book_{realm}.png")
        if os.path.exists(path):
            print(f"  Skip: {realm}")
            continue

        print(f"  Generating: {realm}...")
        prompt = f"{desc}, {base}"
        img = pipe(prompt=prompt, num_inference_steps=4, guidance_scale=0.0,
                   width=512, height=512).images[0]

        # Crop to book shape (roughly center third)
        w, h = img.size
        img = img.crop((w//4, h//8, 3*w//4, 7*h//8))
        img = img.resize((64, 96), Image.LANCZOS)
        img = ImageEnhance.Color(img).enhance(1.3)
        img = ImageEnhance.Contrast(img).enhance(1.1)
        img.save(path, "PNG")

    # Also generate an empty book slot
    empty_path = os.path.join(OUTPUT, "book_empty.png")
    if not os.path.exists(empty_path):
        print("  Generating: empty slot...")
        img = pipe(prompt="empty book shaped outline, thin grey border, very dark, minimal, game UI placeholder",
                   num_inference_steps=4, guidance_scale=0.0, width=512, height=512).images[0]
        w, h = img.size
        img = img.crop((w//4, h//8, 3*w//4, 7*h//8))
        img = img.resize((64, 96), Image.LANCZOS)
        img = ImageEnhance.Brightness(img).enhance(0.3)
        img.save(empty_path, "PNG")

    del pipe; gc.collect(); torch.cuda.empty_cache()
    count = len([f for f in os.listdir(OUTPUT) if f.endswith('.png')])
    print(f"\n=== Done — {count} book icons ===")

if __name__ == "__main__":
    main()
