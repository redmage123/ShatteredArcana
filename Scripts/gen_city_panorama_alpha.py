"""Run rembg over ~/cityview/raw/*.png to produce RGBA sprites in
~/cityview/sprites/. Run this on the dev server after gen_city_panorama.py.
"""
import os
import sys
from rembg import remove
from PIL import Image

RAW_DIR = os.path.expanduser("~/cityview/raw")
OUT_DIR = os.path.expanduser("~/cityview/sprites")
os.makedirs(OUT_DIR, exist_ok=True)

def main():
    pngs = sorted(f for f in os.listdir(RAW_DIR) if f.endswith(".png"))
    print(f"Processing {len(pngs)} sprite raws -> RGBA ...")
    for f in pngs:
        out = os.path.join(OUT_DIR, f)
        if os.path.exists(out):
            print(f"[{f}] exists, skipping")
            continue
        src = Image.open(os.path.join(RAW_DIR, f)).convert("RGBA")
        rgba = remove(src)
        rgba.save(out)
        print(f"[{f}] -> RGBA ({os.path.getsize(out)//1024} KB)")
    print(f"\nDone. {OUT_DIR}")

if __name__ == "__main__":
    sys.exit(main())
