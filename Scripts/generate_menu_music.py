"""
Generate high-fidelity V6 menu theme: war drums building to foreboding brass crescendo.
Uses MusicGen Medium for better quality, generates in segments to avoid artifacts.
"""

import torch
from transformers import AutoProcessor, MusicgenForConditionalGeneration
import scipy.io.wavfile
import numpy as np
import os
import gc

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Content\Audio\Music\Menu"

PROMPT = (
    "War drums building slowly, deep brass stabs, "
    "minor key string tremolo, choir rising, "
    "building from quiet tension to epic power, "
    "dark orchestral crescendo, foreboding and grand"
)

def crossfade(audio1, audio2, overlap_samples):
    """Crossfade two audio arrays with linear fade."""
    fade_out = np.linspace(1.0, 0.0, overlap_samples)
    fade_in = np.linspace(0.0, 1.0, overlap_samples)

    # Apply fades to overlap region
    audio1_end = audio1[-overlap_samples:] * fade_out
    audio2_start = audio2[:overlap_samples] * fade_in

    # Combine: audio1 (without overlap) + crossfaded region + audio2 (without overlap)
    result = np.concatenate([
        audio1[:-overlap_samples],
        audio1_end + audio2_start,
        audio2[overlap_samples:]
    ])
    return result

def generate_segment(model, processor, prompt, tokens=800, temp=0.95):
    """Generate a single audio segment."""
    inputs = processor(text=[prompt], padding=True, return_tensors="pt").to("cuda")
    with torch.no_grad():
        audio = model.generate(
            **inputs, max_new_tokens=tokens,
            do_sample=True, temperature=temp,
        )
    return audio[0, 0].cpu().numpy()

def main():
    print("=== Loading MusicGen Medium ===")

    # Try Medium first, fall back to Small if OOM
    model_name = "facebook/musicgen-medium"
    try:
        processor = AutoProcessor.from_pretrained(model_name)
        model = MusicgenForConditionalGeneration.from_pretrained(
            model_name, torch_dtype=torch.float16
        )
        model = model.to("cuda")
        model.eval()
        print("  Using MusicGen Medium (1.5B) — high fidelity")
    except Exception as e:
        print(f"  Medium failed ({e}), falling back to Small")
        model_name = "facebook/musicgen-small"
        processor = AutoProcessor.from_pretrained(model_name)
        model = MusicgenForConditionalGeneration.from_pretrained(model_name)
        model = model.to("cuda")
        model.eval()

    model.config.audio_encoder.sampling_rate  # 32000
    sr = 32000

    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Generate 3 variations of V6
    prompts_v6 = [
        # V6a: emphasis on building tension
        "Slow deep war drums, quiet tension building, "
        "low brass drones, dark string tremolo, "
        "gradually rising choir voices, minor key, "
        "crescendo to powerful dark orchestral climax",

        # V6b: emphasis on the foreboding quality
        "Deep timpani and bass drum rhythm, foreboding low brass, "
        "dark cello ostinato, building intensity, "
        "ominous choir chanting, brass fanfare emerging, "
        "epic dark fantasy orchestral crescendo",

        # V6c: emphasis on the grand payoff
        "Military snare and timpani building, "
        "deep trombone and French horn, "
        "strings rising in minor key, mixed choir, "
        "dramatic powerful orchestral finale, "
        "dark heroic fantasy theme",
    ]

    for vi, prompt in enumerate(prompts_v6):
        print(f"\n=== V6 variation {vi+1}/3 ===")

        # Generate two 15-second segments and crossfade
        print("  Generating segment 1 (opening)...")
        seg1 = generate_segment(model, processor, prompt, tokens=750, temp=0.95)

        print("  Generating segment 2 (climax)...")
        seg2 = generate_segment(model, processor, prompt, tokens=750, temp=0.9)

        # Crossfade with 2 second overlap
        overlap = sr * 2
        full = crossfade(seg1, seg2, overlap)

        # Normalize
        full = full / np.max(np.abs(full)) * 0.92

        # Resample from 32kHz to 44.1kHz for better playback compatibility
        from scipy.signal import resample
        target_sr = 44100
        num_samples_44k = int(len(full) * target_sr / sr)
        full_44k = resample(full, num_samples_44k).astype(np.float64)
        full_44k = full_44k / np.max(np.abs(full_44k)) * 0.92

        # Save as 16-bit WAV at 44.1kHz
        audio_16bit = (full_44k * 32767).astype(np.int16)
        path = os.path.join(OUTPUT_DIR, f"menu_theme_v6_{chr(97+vi)}.wav")
        scipy.io.wavfile.write(path, target_sr, audio_16bit)
        duration = len(audio_16bit) / target_sr
        print(f"  Saved: {os.path.basename(path)} ({duration:.1f}s at {target_sr}Hz)")

    del model, processor
    gc.collect()
    torch.cuda.empty_cache()
    print("\n=== Done ===")

if __name__ == "__main__":
    main()
