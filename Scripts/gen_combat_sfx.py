"""Synthesise 8 combat SFX procedurally with numpy. Faster, more controllable,
and no model-load dance. Each clip is ~1-2 seconds at 44.1kHz mono.

Drops files in ~/combat_sfx/<name>.wav.
"""
import os
import numpy as np
import scipy.io.wavfile

OUT_DIR = os.path.expanduser("~/combat_sfx")
os.makedirs(OUT_DIR, exist_ok=True)
SR = 44100


def env(samples, attack, decay, hold=0.0):
    """ADSR-ish envelope: attack rise, optional hold, exp decay."""
    n = samples
    out = np.zeros(n, dtype=np.float32)
    a = int(attack * n)
    h = int(hold * n)
    d = max(1, n - a - h)
    if a > 0:
        out[:a] = np.linspace(0, 1, a, dtype=np.float32)
    out[a:a + h] = 1.0
    out[a + h:] = np.exp(-np.linspace(0, 4, d, dtype=np.float32))
    return out


def noise(n, lowpass=None, seed=0):
    rng = np.random.default_rng(seed)
    x = rng.standard_normal(n).astype(np.float32)
    if lowpass:
        # Simple 1-pole IIR low-pass.
        a = float(lowpass)
        y = np.zeros_like(x)
        prev = 0.0
        for i in range(n):
            prev = prev + a * (x[i] - prev)
            y[i] = prev
        x = y
    return x


def sine(freq, n, sr=SR):
    t = np.arange(n, dtype=np.float32) / sr
    if np.isscalar(freq):
        return np.sin(2 * np.pi * freq * t).astype(np.float32)
    # frequency sweep
    return np.sin(2 * np.pi * np.cumsum(freq) / sr).astype(np.float32)


def save(name, wav):
    # Normalise + write int16 PCM.
    peak = max(1e-9, float(np.max(np.abs(wav))))
    wav = wav / peak * 0.95
    wav_i16 = (wav * 32767).astype(np.int16)
    path = os.path.join(OUT_DIR, f"{name}.wav")
    scipy.io.wavfile.write(path, SR, wav_i16)
    print(f"  -> {path} ({os.path.getsize(path)//1024} KB)")


def footsteps_march():
    # Four boot-thuds over ~1.2s; each is short low-noise + low-freq sine.
    dur = int(1.2 * SR)
    out = np.zeros(dur, dtype=np.float32)
    for k, t0 in enumerate([0.05, 0.35, 0.65, 0.95]):
        n = int(0.18 * SR)
        e = env(n, 0.05, 0.95, 0.0)
        body = noise(n, lowpass=0.04, seed=10 + k) * 0.6 + sine(70, n) * 0.35
        i0 = int(t0 * SR)
        i1 = min(dur, i0 + n)
        out[i0:i1] += (body * e)[:i1 - i0]
    return out


def horse_gallop():
    # 8 rapid hoofbeats in a 2-1-2-1 cluster pattern over ~1.6s.
    dur = int(1.6 * SR)
    out = np.zeros(dur, dtype=np.float32)
    pattern = [0.05, 0.12, 0.30, 0.42, 0.55, 0.62, 0.78, 0.92,
               1.05, 1.12, 1.28, 1.42]
    for k, t0 in enumerate(pattern):
        n = int(0.08 * SR)
        e = env(n, 0.02, 0.98)
        body = noise(n, lowpass=0.06, seed=30 + k) * 0.7 \
             + sine(110 - (k % 2) * 30, n) * 0.3
        i0 = int(t0 * SR)
        i1 = min(dur, i0 + n)
        out[i0:i1] += (body * e)[:i1 - i0]
    return out


def wing_flap():
    # 3 deep whooshes over ~1.4s.
    dur = int(1.4 * SR)
    out = np.zeros(dur, dtype=np.float32)
    for k, t0 in enumerate([0.05, 0.55, 1.05]):
        n = int(0.35 * SR)
        # Whoosh: low-pass noise enveloped by a rise-and-fall.
        body = noise(n, lowpass=0.02, seed=50 + k) * 0.9
        e = (np.sin(np.pi * np.linspace(0, 1, n, dtype=np.float32))) ** 2
        i0 = int(t0 * SR)
        i1 = min(dur, i0 + n)
        out[i0:i1] += (body * e)[:i1 - i0]
    return out


def sword_clash():
    # Bright metallic chord: stack of high partials with fast decay,
    # plus a noise crash transient.
    n = int(1.0 * SR)
    partials = [880, 1320, 1980, 2640, 3520]
    body = sum(sine(f, n) * (1.0 / (i + 1)) for i, f in enumerate(partials))
    body += noise(n, lowpass=0.5, seed=70) * 0.8
    e = env(n, 0.005, 0.995)
    return body * e


def arrow_whoosh():
    # Frequency-swept low-pass noise from high to low.
    n = int(0.6 * SR)
    f = np.linspace(2000, 200, n, dtype=np.float32)
    body = sine(f, n) * 0.4 + noise(n, lowpass=0.35, seed=80) * 0.9
    e = env(n, 0.05, 0.95)
    return body * e


def arrow_thud():
    # Short impact: low sine bell + transient noise.
    n = int(0.25 * SR)
    body = sine(180, n) * np.exp(-np.linspace(0, 6, n, dtype=np.float32))
    body += noise(n, lowpass=0.08, seed=90) * np.exp(-np.linspace(0, 9, n, dtype=np.float32))
    return body


def death_grunt():
    # Descending breathy sigh: vowel-like resonance + noise + falling pitch.
    n = int(0.9 * SR)
    f = np.linspace(220, 100, n, dtype=np.float32)
    voice = sine(f, n) * 0.6 + sine(f * 2, n) * 0.3 + sine(f * 3, n) * 0.15
    breath = noise(n, lowpass=0.15, seed=100) * 0.5
    e = env(n, 0.08, 0.92)
    return (voice + breath) * e


def shield_block():
    # Heavy wooden thud: short low sine + thumpy low-noise.
    n = int(0.35 * SR)
    body = sine(90, n) * np.exp(-np.linspace(0, 5, n, dtype=np.float32))
    body += noise(n, lowpass=0.04, seed=110) * np.exp(-np.linspace(0, 8, n, dtype=np.float32)) * 0.8
    return body


CLIPS = [
    ("footsteps_march", footsteps_march),
    ("horse_gallop",    horse_gallop),
    ("wing_flap",       wing_flap),
    ("sword_clash",     sword_clash),
    ("arrow_whoosh",    arrow_whoosh),
    ("arrow_thud",      arrow_thud),
    ("death_grunt",     death_grunt),
    ("shield_block",    shield_block),
]


def main():
    print(f"Synthesising {len(CLIPS)} combat SFX -> {OUT_DIR}")
    for name, fn in CLIPS:
        out = os.path.join(OUT_DIR, f"{name}.wav")
        if os.path.exists(out):
            print(f"[{name}] exists, skipping")
            continue
        print(f"[{name}]")
        wav = fn()
        save(name, wav)
    print("Done.")


if __name__ == "__main__":
    main()
