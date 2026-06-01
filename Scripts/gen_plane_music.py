"""Generate 8 plane-themed overworld music loops with MusicGen on the dev
server. Each track is ~30s of plane-flavoured ambient orchestral fantasy,
intended to underscore the overworld camera moving across that plane.

Output: ~/cityview_music/plane_<name>.wav at 32kHz mono.
The CoMAudioSubsystem PlayMusic path looks up /Game/Audio/Music/<key>.
"""
import os
import torch
import scipy.io.wavfile
from transformers import MusicgenForConditionalGeneration, AutoProcessor

MODEL_PATH = os.path.expanduser("~/models/production/musicgen")
OUT_DIR    = os.path.expanduser("~/plane_music")
os.makedirs(OUT_DIR, exist_ok=True)

# MusicGen prompts: plane-flavour + ambient orchestral fantasy style
PLANES = [
    ("aurelith",
     "warm uplifting orchestral fantasy theme, soft strings and harp, "
     "noble heroic melody, peaceful sunlit kingdom, slow tempo"),
    ("noctharion",
     "mysterious dark fantasy ambient, soft choir and low strings, "
     "moonlit shadowlands, brooding and ominous, slow drone"),
    ("verdantis",
     "vibrant lush jungle fantasy ambient, woodwinds and tribal drums, "
     "wild verdant wilderness, alive and primal, mid tempo"),
    ("infernyx",
     "menacing dark fantasy battle ambient, low brass and war drums, "
     "infernal hellscape, threatening and intense, slow ominous beat"),
    ("aethermist",
     "ethereal floating sky fantasy ambient, soft pads and crystal bells, "
     "cloud-realm wonder, dreamy and weightless, slow tempo"),
    ("abyssal",
     "deep mysterious underwater fantasy ambient, low resonant drone and "
     "echoing chimes, sunken oceanic realm, eerie and vast"),
    ("ethereal",
     "ghostly soft fantasy ambient, distant choir and shimmering pads, "
     "spirit-realm mist, otherworldly and gentle, very slow tempo"),
    ("feywild",
     "whimsical magical fantasy ambient, plucked harp and twinkling celesta, "
     "vibrant fey forest, mystical and playful, mid tempo"),
]

DURATION_SEC = 30

def main():
    print(f"Loading MusicGen from {MODEL_PATH} ...")
    model = MusicgenForConditionalGeneration.from_pretrained(
        MODEL_PATH, torch_dtype=torch.float16).to("cuda")
    processor = AutoProcessor.from_pretrained(MODEL_PATH)
    sample_rate = model.config.audio_encoder.sampling_rate
    # MusicGen tokens-per-second is ~50; one frame ≈ 0.02s
    max_new_tokens = int(DURATION_SEC * 50)
    print(f"sample_rate={sample_rate}, max_new_tokens={max_new_tokens}\n")

    for name, prompt in PLANES:
        out = os.path.join(OUT_DIR, f"plane_{name}.wav")
        if os.path.exists(out):
            print(f"[{name}] exists, skipping")
            continue
        print(f"[{name}] gen {DURATION_SEC}s")
        inputs = processor(text=[prompt], padding=True, return_tensors="pt").to("cuda")
        audio = model.generate(**inputs, max_new_tokens=max_new_tokens,
                               do_sample=True, guidance_scale=3.0)
        wav = audio[0, 0].detach().to(torch.float32).cpu().numpy()
        scipy.io.wavfile.write(out, sample_rate, wav)
        print(f"   -> {out} ({os.path.getsize(out)//1024} KB)")

    print(f"\nDone. {OUT_DIR}")


if __name__ == "__main__":
    main()
