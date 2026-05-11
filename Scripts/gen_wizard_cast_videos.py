"""Generate 14 wizard cast-animation clips with Stable Video Diffusion XT.

Input:  ~/wizard-bgs/wizard_NN.png  (the SDXL-generated combat backgrounds)
Output: ~/wizard-cast/wizard_NN.mp4 (25-frame H.264 videos at 14fps)

SVD-XT requires 1024x576 input. We downsize the 1920x1080 wizard backgrounds
to that aspect / size before generating.

Each clip = 25 frames × ~1-2 min on RTX 3090. 14 clips = ~20-30 min total.
"""
import os
import torch
from diffusers import StableVideoDiffusionPipeline
from diffusers.utils import export_to_video, load_image
from PIL import Image

MODEL_PATH = os.path.expanduser("~/models/production/svd")
INPUT_DIR  = os.path.expanduser("~/wizard-bgs")
OUTPUT_DIR = os.path.expanduser("~/wizard-cast")
os.makedirs(OUTPUT_DIR, exist_ok=True)

W, H = 1024, 576

print(f"Loading SVD-XT from {MODEL_PATH}...")
pipe = StableVideoDiffusionPipeline.from_pretrained(
    MODEL_PATH,
    torch_dtype=torch.float16,
    variant="fp16",
)
pipe = pipe.to("cuda")
pipe.enable_model_cpu_offload()
print("Pipeline ready.\n")

for i in range(1, 15):
    in_path  = os.path.join(INPUT_DIR,  f"wizard_{i:02d}.png")
    out_path = os.path.join(OUTPUT_DIR, f"wizard_{i:02d}.mp4")
    if os.path.exists(out_path):
        print(f"[{i:02d}] already exists, skipping")
        continue
    if not os.path.exists(in_path):
        print(f"[{i:02d}] missing input {in_path}")
        continue

    print(f"[{i:02d}] Generating from {in_path}")
    img = load_image(in_path).resize((W, H))

    gen = torch.Generator("cuda").manual_seed(4200 + i)
    frames = pipe(
        img,
        decode_chunk_size=2,
        generator=gen,
        motion_bucket_id=127,   # higher = more motion
        noise_aug_strength=0.02,
        num_frames=25,
        num_inference_steps=25,
    ).frames[0]

    export_to_video(frames, out_path, fps=14)
    sz_kb = os.path.getsize(out_path) // 1024
    print(f"     -> {out_path} ({sz_kb} KB)")

print("\nWizard cast videos done.")
