"""Generate portraits for the bespoke summoned creatures with SDXL on the dev
server. Painterly fantasy bestiary style, consistent set. Output in ~/summons/
as <SpecID>.png so the UE import maps each to /Game/UI/Units/<SpecID>.
"""
import os
import torch
from diffusers import StableDiffusionXLPipeline

MODEL_PATH = os.path.expanduser("~/models/production/sdxl")
OUT_DIR = os.path.expanduser("~/summons")
os.makedirs(OUT_DIR, exist_ok=True)

W, H = 1024, 768  # 4:3 fits the unit-card portrait area
STEPS = 36
CFG = 7.5

NEG = (
    "ugly, deformed, low quality, low resolution, blurry, jpeg artifacts, "
    "watermark, signature, text, letters, words, logo, frame, border, modern, "
    "photo, 3d render, cgi, multiple panels, out of frame, cropped"
)

STYLE = (
    "fantasy creature bestiary portrait, single subject centered, dramatic "
    "painterly oil illustration, dark atmospheric background, volumetric light, "
    "Master of Magic creature art style, highly detailed, rich colour, "
    "ominous and heroic, no text, no border"
)

# (SpecID, prompt-fragment, seed)
CREATURES = [
    ("Summon_Skeleton", "a pack of animated skeletal warriors wielding rusted swords and "
     "battered shields, glowing green eye sockets, rising from grave mist", 7001),
    ("Summon_PhantomWarrior", "a translucent ghostly armoured warrior, ethereal pale-blue "
     "spectral body, glowing eyes, drifting above the ground, non-corporeal", 7002),
    ("Summon_WarBear", "a massive armoured grizzly war bear rearing and roaring, iron "
     "plating and spiked harness, savage and powerful", 7003),
    ("Summon_Hellhound", "a snarling demonic hellhound with burning ember eyes, smoking "
     "flaming maw, charred black hide, infernal", 7004),
    ("Summon_GuardianSpirit", "a radiant celestial guardian spirit of pure light, humanoid "
     "form woven from glowing energy, serene and protective", 7005),
    ("Summon_Gargoyle", "a winged stone gargoyle with bat-like wings perched and menacing, "
     "cracked grey granite skin, glowing eyes", 7006),
    ("Summon_Wraith", "a shadowy hooded wraith, tattered spectral robes, cold blue spectral "
     "glow, skeletal clawed hand reaching, non-corporeal terror", 7007),
    ("Summon_Angel", "a majestic winged armoured angel wielding a flaming sword, golden "
     "holy light, feathered wings spread, divine and resplendent", 7008),
    ("Summon_Drake", "a colossal dragon-like great drake breathing a torrent of fire, "
     "vast leathery wings spread wide, scaled and ancient, terrifying", 7009),
]


def main():
    print(f"Loading SDXL from {MODEL_PATH} ...")
    pipe = StableDiffusionXLPipeline.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16, use_safetensors=True, variant="fp16",
    ).to("cuda")
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    print(f"Pipeline ready. Generating {len(CREATURES)} summon portraits.\n")

    for spec, fragment, seed in CREATURES:
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
