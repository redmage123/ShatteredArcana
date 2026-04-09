#!/usr/bin/env python3
"""
Generate placeholder audio assets for Shattered Arcana.

Uses numpy waveform synthesis to create WAV files for:
- Music (menu, overworld, combat, victory/defeat)
- UI SFX (clicks, chimes, notifications)
- Combat SFX (swords, arrows, shields)
- Magic SFX (fire, ice, lightning, etc.)
- Ambient loops (plane-themed atmospheres)
- Creature SFX (dragon, undead, monster)
- Naval SFX (waves, cannon, ship)

All files: 44100 Hz, 16-bit PCM, mono.
"""

import os
import numpy as np
from scipy.io import wavfile

SAMPLE_RATE = 44100
BASE_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'Content', 'Audio')
BASE_DIR = os.path.abspath(BASE_DIR)

# ---------------------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------------------

def nsamples(duration):
    """Canonical sample count for a given duration."""
    return int(round(SAMPLE_RATE * duration))


def save_wav(rel_path, samples):
    """Save float samples [-1, 1] as 16-bit PCM WAV."""
    path = os.path.join(BASE_DIR, rel_path)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    samples = np.clip(samples, -1.0, 1.0)
    data = (samples * 32767).astype(np.int16)
    wavfile.write(path, SAMPLE_RATE, data)
    return path


def mul_safe(a, b):
    """Element-wise multiply arrays that may differ in length by a sample or two."""
    n = min(len(a), len(b))
    return a[:n] * b[:n]


def add_safe(a, b):
    """Element-wise add arrays that may differ in length."""
    n = min(len(a), len(b))
    return a[:n] + b[:n]


def sine(freq, duration, phase=0.0):
    n = nsamples(duration)
    t = np.linspace(0, duration, n, endpoint=False)
    return np.sin(2 * np.pi * freq * t + phase)


def silence(duration):
    return np.zeros(nsamples(duration))


def white_noise(duration):
    return np.random.uniform(-1, 1, nsamples(duration))


def pink_noise(duration):
    """Approximate pink noise via spectral shaping."""
    n = nsamples(duration)
    wn = np.random.randn(n)
    # Simple 1/f filter using running averages
    b = [0.049922035, -0.095993537, 0.050612699, -0.004709510]
    a = [1.0, -2.494956002, 2.017265875, -0.522189400]
    from scipy.signal import lfilter
    pn = lfilter(b, a, wn)
    mx = np.max(np.abs(pn))
    if mx > 0:
        pn /= mx
    return pn


def adsr(duration, attack=0.01, decay=0.05, sustain_level=0.7, release=0.05):
    """Generate ADSR envelope."""
    n = nsamples(duration)
    env = np.zeros(n)
    a_n = int(attack * SAMPLE_RATE)
    d_n = int(decay * SAMPLE_RATE)
    r_n = int(release * SAMPLE_RATE)
    s_n = n - a_n - d_n - r_n
    if s_n < 0:
        # Scale proportionally
        total = attack + decay + release
        a_n = int((attack / total) * n * 0.9)
        d_n = int((decay / total) * n * 0.9)
        r_n = n - a_n - d_n
        s_n = 0
        sustain_level = 0.5

    idx = 0
    if a_n > 0:
        env[idx:idx + a_n] = np.linspace(0, 1, a_n)
        idx += a_n
    if d_n > 0:
        env[idx:idx + d_n] = np.linspace(1, sustain_level, d_n)
        idx += d_n
    if s_n > 0:
        env[idx:idx + s_n] = sustain_level
        idx += s_n
    if r_n > 0:
        env[idx:idx + r_n] = np.linspace(sustain_level, 0, r_n)
    return env


def exp_decay(duration, decay_rate=3.0):
    t = np.linspace(0, duration, nsamples(duration), endpoint=False)
    return np.exp(-decay_rate * t)


def simple_reverb(signal, decay=0.3, delay_ms=50):
    """Simple reverb via delayed copies."""
    delay_samples = int(delay_ms * SAMPLE_RATE / 1000)
    result = signal.copy()
    for i in range(1, 5):
        d = delay_samples * i
        gain = decay ** i
        if d < len(result):
            result[d:] += gain * signal[:len(signal) - d]
    mx = np.max(np.abs(result))
    if mx > 0:
        result /= mx
    return result


def freq_from_note(note_name, octave):
    """Convert note name + octave to frequency."""
    notes = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3,
             'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8,
             'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
    n = notes[note_name] - 9 + (octave - 4) * 12
    return 440.0 * (2.0 ** (n / 12.0))


def chord_wave(note_names, octave, duration, amp=0.3):
    """Generate a chord from note names."""
    result = np.zeros(nsamples(duration))
    for note in note_names:
        f = freq_from_note(note, octave)
        result += sine(f, duration) * amp
        # Add a soft overtone
        result += sine(f * 2, duration) * amp * 0.15
    return result / max(1, len(note_names))


def sweep(f_start, f_end, duration):
    """Frequency sweep (linear)."""
    t = np.linspace(0, duration, nsamples(duration), endpoint=False)
    phase = 2 * np.pi * (f_start * t + (f_end - f_start) * t * t / (2 * duration))
    return np.sin(phase)


def bandpass_noise(duration, center_freq, bandwidth):
    """Approximate bandpass filtered noise."""
    from scipy.signal import butter, lfilter
    n = white_noise(duration)
    low = max(20, center_freq - bandwidth / 2)
    high = min(SAMPLE_RATE / 2 - 1, center_freq + bandwidth / 2)
    nyq = SAMPLE_RATE / 2
    b, a = butter(4, [low / nyq, high / nyq], btype='band')
    filtered = lfilter(b, a, n)
    mx = np.max(np.abs(filtered))
    if mx > 0:
        filtered /= mx
    return filtered


def normalize(signal, level=0.9):
    mx = np.max(np.abs(signal))
    if mx > 0:
        return signal * (level / mx)
    return signal


def make_loopable(signal, crossfade_samples=4410):
    """Crossfade the end into the beginning for seamless looping."""
    cf = min(crossfade_samples, len(signal) // 4)
    fade_in = np.linspace(0, 1, cf)
    fade_out = np.linspace(1, 0, cf)
    signal[:cf] = signal[:cf] * fade_in + signal[-cf:] * fade_out
    signal[-cf:] = signal[:cf]  # mirror the crossfaded beginning
    return signal


# ---------------------------------------------------------------------------
# Music generators
# ---------------------------------------------------------------------------

def gen_menu_theme():
    """Gentle arpeggiated chords: C major -> Am -> F -> G, 45 seconds."""
    duration = 45.0
    chord_dur = 45.0 / 8  # 2 repeats of the 4-chord progression
    chords = [
        (['C', 'E', 'G'], 4),
        (['A', 'C', 'E'], 4),
        (['F', 'A', 'C'], 4),
        (['G', 'B', 'D'], 4),
    ]
    result = np.zeros(nsamples(duration))
    for repeat in range(2):
        for ci, (notes, octave) in enumerate(chords):
            offset = int((repeat * 4 + ci) * chord_dur * SAMPLE_RATE)
            # Arpeggiate
            note_dur = chord_dur / len(notes)
            for ni, note in enumerate(notes):
                f = freq_from_note(note, octave)
                n_offset = offset + int(ni * note_dur * SAMPLE_RATE)
                tone = sine(f, note_dur) * adsr(note_dur, 0.05, 0.1, 0.6, 0.3)
                tone += sine(f * 2, note_dur) * 0.1 * adsr(note_dur, 0.05, 0.1, 0.4, 0.3)
                end = min(n_offset + len(tone), len(result))
                result[n_offset:end] += tone[:end - n_offset] * 0.5
            # Pad chord underneath
            pad = chord_wave(notes, octave, chord_dur, 0.15)
            pad *= adsr(chord_dur, 0.2, 0.3, 0.5, 0.5)
            end = min(offset + len(pad), len(result))
            result[offset:end] += pad[:end - offset]
    result = make_loopable(normalize(simple_reverb(result, 0.3, 80)))
    save_wav('Music/Menu/menu_theme.wav', result)


def gen_overworld_theme():
    """Brighter major key melody, 45 seconds."""
    duration = 45.0
    # Simple melody in C major
    melody_notes = [
        ('C', 5, 0.4), ('E', 5, 0.4), ('G', 5, 0.4), ('A', 5, 0.6),
        ('G', 5, 0.4), ('E', 5, 0.4), ('F', 5, 0.6), ('E', 5, 0.4),
        ('D', 5, 0.4), ('C', 5, 0.8),
        ('D', 5, 0.4), ('E', 5, 0.4), ('F', 5, 0.4), ('G', 5, 0.6),
        ('A', 5, 0.4), ('G', 5, 0.4), ('F', 5, 0.4), ('E', 5, 0.6),
        ('D', 5, 0.4), ('C', 5, 0.8),
    ]
    result = np.zeros(nsamples(duration))
    # Background pad
    pad_chords = [(['C', 'E', 'G'], 3), (['F', 'A', 'C'], 3),
                  (['G', 'B', 'D'], 3), (['C', 'E', 'G'], 3)]
    pad_dur = duration / len(pad_chords)
    for i, (notes, oct) in enumerate(pad_chords):
        offset = int(i * pad_dur * SAMPLE_RATE)
        pad = chord_wave(notes, oct, pad_dur, 0.12)
        pad *= adsr(pad_dur, 0.5, 0.5, 0.6, 1.0)
        end = min(offset + len(pad), len(result))
        result[offset:end] += pad[:end - offset]

    # Melody
    pos = 0.0
    total_melody_dur = sum(d for _, _, d in melody_notes)
    repeats = int(duration / total_melody_dur) + 1
    for _ in range(repeats):
        for note, oct, dur in melody_notes:
            if pos >= duration:
                break
            f = freq_from_note(note, oct)
            actual_dur = min(dur, duration - pos)
            tone = sine(f, actual_dur) * adsr(actual_dur, 0.02, 0.05, 0.7, 0.1) * 0.4
            tone += sine(f * 2, actual_dur) * 0.05 * adsr(actual_dur, 0.02, 0.05, 0.5, 0.1)
            offset = int(pos * SAMPLE_RATE)
            end = min(offset + len(tone), len(result))
            result[offset:end] += tone[:end - offset]
            pos += dur

    result = make_loopable(normalize(simple_reverb(result, 0.25, 60)))
    save_wav('Music/Overworld/overworld_theme.wav', result)


def gen_combat_light():
    """Faster tempo, minor key, percussion-like beats, 40 seconds."""
    duration = 40.0
    n = nsamples(duration)
    result = np.zeros(n)
    bpm = 140
    beat_dur = 60.0 / bpm

    # Percussion: kick-like on beats
    t_pos = 0.0
    beat_i = 0
    while t_pos < duration:
        offset = int(t_pos * SAMPLE_RATE)
        # Kick on 1 and 3
        if beat_i % 4 in (0, 2):
            kick = sine(60, 0.1) * exp_decay(0.1, 15) * 0.5
            end = min(offset + len(kick), n)
            result[offset:end] += kick[:end - offset]
        # Hi-hat on every beat
        hh = white_noise(0.03) * exp_decay(0.03, 30) * 0.2
        end = min(offset + len(hh), n)
        result[offset:end] += hh[:end - offset]
        t_pos += beat_dur
        beat_i += 1

    # Minor key bass line: Am
    bass_notes = [('A', 2), ('C', 3), ('E', 3), ('A', 2)]
    bass_dur = beat_dur * 4
    pos = 0.0
    i = 0
    while pos < duration:
        note, oct = bass_notes[i % len(bass_notes)]
        f = freq_from_note(note, oct)
        bd = min(bass_dur, duration - pos)
        tone = sine(f, bd) * adsr(bd, 0.01, 0.1, 0.6, 0.2) * 0.3
        offset = int(pos * SAMPLE_RATE)
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]
        pos += bass_dur
        i += 1

    result = make_loopable(normalize(result))
    save_wav('Music/Combat/combat_light.wav', result)


def gen_combat_heavy():
    """Intense driving rhythm, dissonant, 40 seconds."""
    duration = 40.0
    n = nsamples(duration)
    result = np.zeros(n)
    bpm = 160
    beat_dur = 60.0 / bpm

    # Heavy percussion
    t_pos = 0.0
    beat_i = 0
    while t_pos < duration:
        offset = int(t_pos * SAMPLE_RATE)
        # Kick every beat
        kick = sine(50, 0.15) * exp_decay(0.15, 12) * 0.6
        end = min(offset + len(kick), n)
        result[offset:end] += kick[:end - offset]
        # Snare on 2 and 4
        if beat_i % 4 in (1, 3):
            snare = white_noise(0.08) * exp_decay(0.08, 15) * 0.4
            end = min(offset + len(snare), n)
            result[offset:end] += snare[:end - offset]
        t_pos += beat_dur
        beat_i += 1

    # Dissonant power chords (tritone)
    chords_seq = [
        (['E', 'Bb'], 2), (['F', 'B'], 2), (['E', 'Bb'], 2), (['D', 'Ab'], 2)
    ]
    chord_dur = duration / len(chords_seq)
    for i, (notes, oct) in enumerate(chords_seq):
        offset = int(i * chord_dur * SAMPLE_RATE)
        cw = chord_wave(notes, oct, chord_dur, 0.25)
        # Add distortion-like saturation
        cw = np.tanh(cw * 3) * 0.4
        cw *= adsr(chord_dur, 0.1, 0.2, 0.7, 0.5)
        end = min(offset + len(cw), n)
        result[offset:end] += cw[:end - offset]

    result = make_loopable(normalize(result))
    save_wav('Music/Combat/combat_heavy.wav', result)


def gen_combat_epic():
    """Full orchestral simulation, layered harmonics, 45 seconds."""
    duration = 45.0
    n = nsamples(duration)
    result = np.zeros(n)
    bpm = 150
    beat_dur = 60.0 / bpm

    # Timpani-like percussion
    t_pos = 0.0
    beat_i = 0
    while t_pos < duration:
        offset = int(t_pos * SAMPLE_RATE)
        if beat_i % 8 in (0, 3, 6):
            timp = sine(80, 0.3) * exp_decay(0.3, 8) * 0.5
            timp += sine(40, 0.3) * exp_decay(0.3, 6) * 0.3
            end = min(offset + len(timp), n)
            result[offset:end] += timp[:end - offset]
        # Cymbal crashes on bar starts
        if beat_i % 16 == 0:
            crash = white_noise(0.8) * exp_decay(0.8, 4) * 0.2
            end = min(offset + len(crash), n)
            result[offset:end] += crash[:end - offset]
        t_pos += beat_dur
        beat_i += 1

    # Brass-like chords with rich harmonics
    epic_chords = [
        (['D', 'F', 'A'], 3), (['Bb', 'D', 'F'], 3),
        (['C', 'E', 'G'], 3), (['D', 'F', 'A'], 3),
    ]
    chord_dur = duration / len(epic_chords)
    for i, (notes, oct) in enumerate(epic_chords):
        offset = int(i * chord_dur * SAMPLE_RATE)
        cw = np.zeros(nsamples(chord_dur))
        for note in notes:
            f = freq_from_note(note, oct)
            for harm in range(1, 6):
                amp = 0.2 / harm
                cw += sine(f * harm, chord_dur) * amp
        cw *= adsr(chord_dur, 0.3, 0.3, 0.7, 0.5)
        end = min(offset + len(cw), n)
        result[offset:end] += cw[:end - offset]

    # String-like high melody
    melody = [('D', 5), ('F', 5), ('A', 5), ('G', 5), ('F', 5), ('E', 5),
              ('D', 5), ('C', 5), ('D', 5)]
    mel_dur = duration / len(melody)
    for i, (note, oct) in enumerate(melody):
        offset = int(i * mel_dur * SAMPLE_RATE)
        f = freq_from_note(note, oct)
        tone = sine(f, mel_dur) * 0.2
        tone += sine(f * 2, mel_dur) * 0.05
        tone *= adsr(mel_dur, 0.1, 0.2, 0.6, 0.3)
        # Vibrato
        t = np.linspace(0, mel_dur, nsamples(mel_dur), endpoint=False)
        vibrato = 1.0 + 0.01 * np.sin(2 * np.pi * 5 * t)
        tone *= vibrato
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]

    result = make_loopable(normalize(simple_reverb(result, 0.35, 100)))
    save_wav('Music/Combat/combat_epic.wav', result)


def gen_victory_fanfare():
    """Short triumphant ascending arpeggio, 5 seconds."""
    duration = 5.0
    n = nsamples(duration)
    result = np.zeros(n)
    notes = [('C', 4), ('E', 4), ('G', 4), ('C', 5), ('E', 5), ('G', 5), ('C', 6)]
    note_dur = 0.3
    for i, (note, oct) in enumerate(notes):
        offset = int(i * note_dur * SAMPLE_RATE)
        f = freq_from_note(note, oct)
        d = duration - i * note_dur
        if d <= 0:
            break
        tone = sine(f, d) * exp_decay(d, 2.0) * 0.3
        tone += sine(f * 2, d) * exp_decay(d, 3.0) * 0.1
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]

    # Final chord ring
    final_chord = chord_wave(['C', 'E', 'G'], 5, 2.5, 0.3)
    final_chord *= exp_decay(2.5, 1.5)
    offset = int(2.5 * SAMPLE_RATE)
    end = min(offset + len(final_chord), n)
    result[offset:end] += final_chord[:end - offset]

    result = normalize(simple_reverb(result, 0.3, 70))
    save_wav('Music/Victory/victory_fanfare.wav', result)


def gen_defeat_stinger():
    """Descending minor chord, 3 seconds."""
    duration = 3.0
    n = nsamples(duration)
    result = np.zeros(n)
    # Descending Cm chord
    notes = [('G', 4), ('Eb', 4), ('C', 4), ('G', 3)]
    note_dur = 0.5
    for i, (note, oct) in enumerate(notes):
        offset = int(i * note_dur * SAMPLE_RATE)
        f = freq_from_note(note, oct)
        d = duration - i * note_dur
        if d <= 0:
            break
        tone = sine(f, d) * exp_decay(d, 2.5) * 0.35
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]

    result = normalize(simple_reverb(result, 0.3, 80))
    save_wav('Music/Victory/defeat_stinger.wav', result)


# ---------------------------------------------------------------------------
# UI SFX generators
# ---------------------------------------------------------------------------

def gen_btn_click():
    d = 0.1
    noise_burst = white_noise(0.02) * exp_decay(0.02, 40) * 0.5
    click = sine(800, d) * exp_decay(d, 30) * 0.4
    result = np.zeros(nsamples(d))
    result[:len(noise_burst)] += noise_burst
    result[:len(click)] += click
    save_wav('SFX/UI/btn_click.wav', normalize(result))


def gen_btn_hover():
    d = 0.08
    result = sine(1200, d) * exp_decay(d, 25) * 0.25
    save_wav('SFX/UI/btn_hover.wav', normalize(result))


def gen_notification():
    d = 0.4
    n = nsamples(d)
    result = np.zeros(n)
    # Two ascending notes
    t1 = sine(880, 0.15) * adsr(0.15, 0.01, 0.02, 0.7, 0.05) * 0.5
    t2 = sine(1100, 0.2) * adsr(0.2, 0.01, 0.02, 0.7, 0.1) * 0.5
    result[:len(t1)] += t1
    offset = int(0.15 * SAMPLE_RATE)
    result[offset:offset + len(t2)] += t2[:min(len(t2), n - offset)]
    save_wav('SFX/UI/notification.wav', normalize(simple_reverb(result, 0.2, 40)))


def gen_error():
    d = 0.3
    result = sine(150, d) * adsr(d, 0.01, 0.05, 0.6, 0.1) * 0.5
    result += sine(155, d) * adsr(d, 0.01, 0.05, 0.6, 0.1) * 0.3  # beating
    save_wav('SFX/UI/error.wav', normalize(result))


def gen_turn_start():
    d = 0.5
    n = nsamples(d)
    result = np.zeros(n)
    bell = sine(1500, d) * exp_decay(d, 5) * 0.4
    bell += sine(3000, d) * exp_decay(d, 7) * 0.15
    bell += sine(4500, d) * exp_decay(d, 9) * 0.08
    result[:len(bell)] = bell
    save_wav('SFX/UI/turn_start.wav', normalize(simple_reverb(result, 0.3, 50)))


def gen_turn_end():
    d = 0.4
    result = chord_wave(['C', 'E', 'G'], 5, d, 0.3)
    result *= adsr(d, 0.02, 0.1, 0.5, 0.2)
    save_wav('SFX/UI/turn_end.wav', normalize(simple_reverb(result, 0.2, 40)))


def gen_build_complete():
    d = 0.5
    n = nsamples(d)
    result = np.zeros(n)
    # Hammer tap
    tap = white_noise(0.03) * exp_decay(0.03, 40) * 0.5
    tap2 = sine(300, 0.05) * exp_decay(0.05, 20) * 0.3
    # Combine: pad shorter to longer
    if len(tap) < len(tap2):
        tap2[:len(tap)] += tap
        tap = tap2
    else:
        tap[:len(tap2)] += tap2
    result[:len(tap)] += tap
    # Chime
    chime = sine(1200, 0.3) * exp_decay(0.3, 6) * 0.4
    offset = int(0.1 * SAMPLE_RATE)
    end = min(offset + len(chime), n)
    result[offset:end] += chime[:end - offset]
    save_wav('SFX/UI/build_complete.wav', normalize(result))


def gen_research_complete():
    d = 0.5
    n = nsamples(d)
    result = np.zeros(n)
    # Ascending sparkle
    for i, freq in enumerate([800, 1200, 1600, 2000, 2400]):
        offset = int(i * 0.08 * SAMPLE_RATE)
        tone = sine(freq, 0.15) * exp_decay(0.15, 8) * 0.3
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]
    save_wav('SFX/UI/research_complete.wav', normalize(simple_reverb(result, 0.25, 35)))


def gen_spell_learned():
    d = 0.6
    n = nsamples(d)
    result = np.zeros(n)
    # Filtered noise shimmer
    shimmer = bandpass_noise(d, 3000, 2000) * adsr(d, 0.05, 0.1, 0.4, 0.3) * 0.2
    result[:len(shimmer)] += shimmer
    # Sine sweep upward
    sw = sweep(400, 2000, d) * adsr(d, 0.05, 0.1, 0.5, 0.2) * 0.3
    result[:len(sw)] += sw
    save_wav('SFX/UI/spell_learned.wav', normalize(simple_reverb(result, 0.3, 50)))


def gen_gold_gained():
    d = 0.3
    n = nsamples(d)
    result = np.zeros(n)
    # Metallic ping
    ping = sine(2500, d) * exp_decay(d, 8) * 0.4
    ping += sine(5000, d) * exp_decay(d, 12) * 0.15
    # Coin clink noise
    clink = bandpass_noise(0.05, 4000, 3000) * exp_decay(0.05, 30) * 0.3
    result[:len(clink)] += clink
    result[:len(ping)] += ping
    save_wav('SFX/UI/gold_gained.wav', normalize(result))


def gen_menu_open():
    d = 0.25
    result = sweep(200, 1500, d) * adsr(d, 0.01, 0.05, 0.5, 0.1) * 0.4
    # Add noise whoosh
    noise = bandpass_noise(d, 2000, 3000) * adsr(d, 0.01, 0.05, 0.3, 0.1) * 0.2
    result += noise[:len(result)]
    save_wav('SFX/UI/menu_open.wav', normalize(result))


def gen_menu_close():
    d = 0.25
    result = sweep(1500, 200, d) * adsr(d, 0.01, 0.05, 0.5, 0.1) * 0.4
    noise = bandpass_noise(d, 2000, 3000) * adsr(d, 0.01, 0.05, 0.3, 0.1) * 0.2
    result += noise[:len(result)]
    save_wav('SFX/UI/menu_close.wav', normalize(result))


# ---------------------------------------------------------------------------
# Combat SFX generators
# ---------------------------------------------------------------------------

def gen_sword_clash():
    d = 0.5
    n = nsamples(d)
    result = np.zeros(n)
    # Metallic impact
    impact = white_noise(0.02) * exp_decay(0.02, 50) * 0.6
    result[:len(impact)] += impact
    # Metallic ring
    ring = sine(2000, d) * exp_decay(d, 6) * 0.3
    ring += sine(3500, d) * exp_decay(d, 8) * 0.15
    ring += sine(5200, d) * exp_decay(d, 10) * 0.08
    result[:len(ring)] += ring
    save_wav('SFX/Combat/sword_clash.wav', normalize(result))


def gen_sword_swing():
    d = 0.3
    result = sweep(400, 100, d) * adsr(d, 0.02, 0.05, 0.3, 0.15) * 0.3
    noise = bandpass_noise(d, 800, 600) * adsr(d, 0.02, 0.08, 0.2, 0.1) * 0.3
    result += noise[:len(result)]
    save_wav('SFX/Combat/sword_swing.wav', normalize(result))


def gen_arrow_loose():
    d = 0.4
    n = nsamples(d)
    result = np.zeros(n)
    # Twang
    twang = sine(400, 0.15) * exp_decay(0.15, 12) * 0.4
    twang2 = sine(800, 0.1) * exp_decay(0.1, 15) * 0.2
    twang[:len(twang2)] += twang2
    result[:len(twang)] += twang
    # Whoosh
    whoosh = bandpass_noise(0.3, 1000, 800) * adsr(0.3, 0.05, 0.05, 0.3, 0.15) * 0.25
    offset = int(0.08 * SAMPLE_RATE)
    end = min(offset + len(whoosh), n)
    result[offset:end] += whoosh[:end - offset]
    save_wav('SFX/Combat/arrow_loose.wav', normalize(result))


def gen_arrow_impact():
    d = 0.25
    n = nsamples(d)
    result = np.zeros(n)
    # Thud
    thud = sine(100, 0.1) * exp_decay(0.1, 15) * 0.5
    thud_noise = white_noise(0.03) * exp_decay(0.03, 30) * 0.3
    thud[:len(thud_noise)] += thud_noise
    result[:len(thud)] += thud
    save_wav('SFX/Combat/arrow_impact.wav', normalize(result))


def gen_shield_block():
    d = 0.4
    n = nsamples(d)
    result = np.zeros(n)
    # Heavy metallic thud
    thud = sine(80, 0.2) * exp_decay(0.2, 8) * 0.5
    impact = white_noise(0.03) * exp_decay(0.03, 35) * 0.5
    ring = sine(1200, d) * exp_decay(d, 5) * 0.2
    result[:len(thud)] += thud
    result[:len(impact)] += impact
    result[:len(ring)] += ring
    save_wav('SFX/Combat/shield_block.wav', normalize(result))


def gen_unit_hit():
    d = 0.3
    n = nsamples(d)
    result = np.zeros(n)
    # Impact
    impact = sine(120, 0.1) * exp_decay(0.1, 12) * 0.5
    impact_noise = white_noise(0.04) * exp_decay(0.04, 25) * 0.4
    impact[:len(impact_noise)] += impact_noise
    result[:len(impact)] += impact
    # Grunt-like noise burst
    grunt = bandpass_noise(0.15, 300, 200) * adsr(0.15, 0.01, 0.03, 0.4, 0.08) * 0.3
    offset = int(0.05 * SAMPLE_RATE)
    end = min(offset + len(grunt), n)
    result[offset:end] += grunt[:end - offset]
    save_wav('SFX/Combat/unit_hit.wav', normalize(result))


def gen_unit_death():
    d = 0.8
    n = nsamples(d)
    result = np.zeros(n)
    # Descending tone
    desc = sweep(400, 80, 0.6) * adsr(0.6, 0.02, 0.1, 0.5, 0.3) * 0.4
    result[:len(desc)] += desc
    # Impact at start
    impact = white_noise(0.04) * exp_decay(0.04, 25) * 0.3
    result[:len(impact)] += impact
    save_wav('SFX/Combat/unit_death.wav', normalize(result))


# ---------------------------------------------------------------------------
# Magic SFX generators
# ---------------------------------------------------------------------------

def gen_spell_fire():
    d = 1.0
    n = nsamples(d)
    result = np.zeros(n)
    # Crackling noise burst
    crackle = white_noise(d) * 0.3
    # Modulate with random bursts
    t = np.linspace(0, d, n, endpoint=False)
    mod = np.abs(np.sin(2 * np.pi * 20 * t + np.random.randn(n) * 0.5))
    crackle *= mod * adsr(d, 0.05, 0.1, 0.6, 0.3)
    result += crackle
    # Rising pitch
    rising = sweep(200, 800, d) * adsr(d, 0.1, 0.2, 0.5, 0.3) * 0.3
    result += rising
    save_wav('SFX/Magic/spell_fire.wav', normalize(result))


def gen_spell_ice():
    d = 1.0
    n = nsamples(d)
    result = np.zeros(n)
    # Crystalline high-frequency shimmer
    for freq in [3000, 4500, 6000, 7500]:
        tone = sine(freq, d) * exp_decay(d, 2) * 0.15
        result[:len(tone)] += tone
    # Add sparkle noise
    sparkle = bandpass_noise(d, 5000, 3000) * adsr(d, 0.05, 0.1, 0.3, 0.4) * 0.2
    result += sparkle
    save_wav('SFX/Magic/spell_ice.wav', normalize(simple_reverb(result, 0.3, 60)))


def gen_spell_lightning():
    d = 0.8
    n = nsamples(d)
    result = np.zeros(n)
    # Sharp crack
    crack = white_noise(0.02) * 0.8
    result[:len(crack)] += crack
    # Rumble decay
    rumble = bandpass_noise(d, 100, 80) * exp_decay(d, 3) * 0.4
    result[:len(rumble)] += rumble
    # Electric sizzle
    sizzle = bandpass_noise(0.5, 3000, 2000) * exp_decay(0.5, 5) * 0.2
    offset = int(0.02 * SAMPLE_RATE)
    end = min(offset + len(sizzle), n)
    result[offset:end] += sizzle[:end - offset]
    save_wav('SFX/Magic/spell_lightning.wav', normalize(result))


def gen_spell_death():
    d = 1.5
    n = nsamples(d)
    result = np.zeros(n)
    # Low rumbling drone
    drone = sine(60, d) * adsr(d, 0.2, 0.2, 0.7, 0.4) * 0.4
    drone += sine(63, d) * adsr(d, 0.2, 0.2, 0.7, 0.4) * 0.3  # beating
    result += drone
    # Eerie overtones
    t = np.linspace(0, d, n, endpoint=False)
    eerie = sine(300, d) * 0.15 * np.sin(2 * np.pi * 0.5 * t)  # tremolo
    result += eerie
    save_wav('SFX/Magic/spell_death.wav', normalize(result))


def gen_spell_nature():
    d = 1.0
    n = nsamples(d)
    result = np.zeros(n)
    # Gentle chime
    chime = sine(1000, d) * exp_decay(d, 3) * 0.3
    chime += sine(1500, d) * exp_decay(d, 4) * 0.15
    result += chime
    # Rustling noise
    rustle = bandpass_noise(d, 2000, 1500) * adsr(d, 0.1, 0.1, 0.3, 0.3) * 0.15
    result += rustle
    save_wav('SFX/Magic/spell_nature.wav', normalize(simple_reverb(result, 0.25, 45)))


def gen_spell_healing():
    d = 1.5
    n = nsamples(d)
    result = np.zeros(n)
    # Ascending pure tones
    notes = [('C', 5), ('E', 5), ('G', 5), ('C', 6)]
    note_dur = d / len(notes)
    for i, (note, oct) in enumerate(notes):
        offset = int(i * note_dur * SAMPLE_RATE)
        f = freq_from_note(note, oct)
        remaining = d - i * note_dur
        tone = sine(f, remaining) * exp_decay(remaining, 2) * 0.3
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]
    save_wav('SFX/Magic/spell_healing.wav', normalize(simple_reverb(result, 0.35, 60)))


def gen_spell_arcane():
    d = 1.5
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    # Mysterious warbling with phase shifting
    result = np.zeros(n)
    for freq in [300, 450, 600]:
        phase_mod = np.sin(2 * np.pi * 2 * t) * 3  # phase modulation
        tone = np.sin(2 * np.pi * freq * t + phase_mod) * 0.2
        tone *= adsr(d, 0.1, 0.2, 0.6, 0.4)
        result += tone
    save_wav('SFX/Magic/spell_arcane.wav', normalize(simple_reverb(result, 0.3, 70)))


# ---------------------------------------------------------------------------
# Ambient loop generators
# ---------------------------------------------------------------------------

def gen_ambient_aurelith():
    """Warm pad + distant bells, 20 seconds."""
    d = 20.0
    n = nsamples(d)
    result = np.zeros(n)
    # Warm pad (C major)
    pad = chord_wave(['C', 'E', 'G'], 3, d, 0.2)
    pad *= adsr(d, 1.0, 1.0, 0.6, 2.0)
    result += pad
    # Distant bells at random intervals
    np.random.seed(42)
    for _ in range(8):
        t_start = np.random.uniform(0, d - 2)
        freq = np.random.choice([1200, 1500, 1800, 2000])
        offset = int(t_start * SAMPLE_RATE)
        bell = sine(freq, 1.5) * exp_decay(1.5, 3) * 0.15
        bell2 = sine(freq * 2.5, 1.0) * exp_decay(1.0, 5) * 0.05
        bell[:len(bell2)] += bell2
        end = min(offset + len(bell), n)
        result[offset:end] += bell[:end - offset]
    result = make_loopable(normalize(simple_reverb(result, 0.4, 100)))
    save_wav('Ambient/Planes/Aurelith/ambient_aurelith.wav', result)


def gen_ambient_noctharion():
    """Dark drone + eerie whispers (filtered noise), 20 seconds."""
    d = 20.0
    n = nsamples(d)
    result = np.zeros(n)
    # Dark drone
    drone = sine(55, d) * 0.3 + sine(57, d) * 0.25  # beating
    drone *= adsr(d, 1.0, 1.0, 0.7, 2.0)
    result += drone
    # Eerie whispers
    np.random.seed(43)
    for _ in range(6):
        t_start = np.random.uniform(0, d - 3)
        offset = int(t_start * SAMPLE_RATE)
        whisper = bandpass_noise(2.0, 1500, 500) * adsr(2.0, 0.3, 0.3, 0.3, 0.5) * 0.1
        end = min(offset + len(whisper), n)
        result[offset:end] += whisper[:end - offset]
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Noctharion/ambient_noctharion.wav', result)


def gen_ambient_verdantis():
    """Nature sounds (layered noise bands simulating birds/wind), 25 seconds."""
    d = 25.0
    n = nsamples(d)
    result = np.zeros(n)
    # Wind
    wind = bandpass_noise(d, 400, 300) * 0.15
    t = np.linspace(0, d, n, endpoint=False)
    wind *= 0.5 + 0.5 * np.sin(2 * np.pi * 0.1 * t)  # slow modulation
    result += wind
    # Bird-like chirps
    np.random.seed(44)
    for _ in range(15):
        t_start = np.random.uniform(0, d - 0.5)
        freq = np.random.uniform(2000, 4000)
        offset = int(t_start * SAMPLE_RATE)
        chirp = sweep(freq, freq * 1.3, 0.1) * exp_decay(0.1, 15) * 0.15
        end = min(offset + len(chirp), n)
        result[offset:end] += chirp[:end - offset]
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Verdantis/ambient_verdantis.wav', result)


def gen_ambient_infernyx():
    """Low rumble + crackling, 20 seconds."""
    d = 20.0
    n = nsamples(d)
    result = np.zeros(n)
    # Low rumble
    rumble = bandpass_noise(d, 60, 40) * 0.3
    result += rumble
    # Crackling
    np.random.seed(45)
    crackle = white_noise(d) * 0.1
    # Sparse crackles
    mask = np.random.random(n) > 0.998
    crackle *= mask.astype(float)
    from scipy.signal import lfilter
    # Smooth slightly
    b = np.ones(100) / 100
    crackle = lfilter(b, [1.0], crackle)
    crackle = crackle / max(np.max(np.abs(crackle)), 1e-6) * 0.15
    result += crackle
    # Sub bass
    result += sine(40, d) * 0.2
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Infernyx/ambient_infernyx.wav', result)


def gen_ambient_aethermist():
    """Ethereal pad + wind, 20 seconds."""
    d = 20.0
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    result = np.zeros(n)
    # Ethereal pad with slow phase
    pad = np.sin(2 * np.pi * 220 * t + np.sin(2 * np.pi * 0.2 * t) * 2) * 0.2
    pad += np.sin(2 * np.pi * 330 * t + np.sin(2 * np.pi * 0.15 * t) * 2) * 0.15
    pad *= adsr(d, 2.0, 1.0, 0.6, 2.0)
    result += pad
    # Wind
    wind = bandpass_noise(d, 600, 400) * 0.1
    wind *= 0.5 + 0.5 * np.sin(2 * np.pi * 0.08 * t)
    result += wind
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Aethermist/ambient_aethermist.wav', result)


def gen_ambient_abyssal():
    """Deep disturbing drone + heartbeat-like pulse, 20 seconds."""
    d = 20.0
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    result = np.zeros(n)
    # Deep drone
    drone = sine(35, d) * 0.3 + sine(37, d) * 0.25
    drone *= adsr(d, 1.0, 1.0, 0.8, 2.0)
    result += drone
    # Heartbeat pulse (~70 bpm)
    heartbeat_freq = 70 / 60  # Hz
    pulse_env = np.abs(np.sin(2 * np.pi * heartbeat_freq * t)) ** 8
    pulse = sine(50, d) * pulse_env * 0.3
    result += pulse
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Abyssal/ambient_abyssal.wav', result)


def gen_ambient_ethereal():
    """Ghostly shimmer + phase effects, 20 seconds."""
    d = 20.0
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    result = np.zeros(n)
    # Ghostly shimmer
    for freq in [400, 600, 800]:
        phase = np.sin(2 * np.pi * 0.3 * t) * 4
        tone = np.sin(2 * np.pi * freq * t + phase) * 0.12
        result += tone
    result *= adsr(d, 1.5, 1.0, 0.6, 2.0)
    # High shimmer
    shimmer = bandpass_noise(d, 4000, 2000) * 0.05
    shimmer *= 0.5 + 0.5 * np.sin(2 * np.pi * 0.2 * t)
    result += shimmer
    result = make_loopable(normalize(result))
    save_wav('Ambient/Planes/Ethereal/ambient_ethereal.wav', result)


def gen_ambient_feywild():
    """Magical chimes + gentle melody, 25 seconds."""
    d = 25.0
    n = nsamples(d)
    result = np.zeros(n)
    # Gentle pad
    pad = chord_wave(['F', 'A', 'C'], 4, d, 0.12)
    pad *= adsr(d, 1.5, 1.0, 0.5, 2.0)
    result += pad
    # Random chimes
    np.random.seed(46)
    chime_freqs = [freq_from_note(nn, 5) for nn in ['C', 'E', 'F', 'G', 'A']]
    for _ in range(12):
        t_start = np.random.uniform(0, d - 1.5)
        freq = np.random.choice(chime_freqs)
        offset = int(t_start * SAMPLE_RATE)
        chime = sine(freq, 1.0) * exp_decay(1.0, 3) * 0.2
        end = min(offset + len(chime), n)
        result[offset:end] += chime[:end - offset]
    # Simple melody
    melody = [('C', 5), ('E', 5), ('F', 5), ('G', 5), ('A', 5), ('G', 5)]
    mel_dur = 2.0
    for i, (note, oct) in enumerate(melody):
        pos = 5.0 + i * mel_dur
        if pos + mel_dur > d:
            break
        offset = int(pos * SAMPLE_RATE)
        f = freq_from_note(note, oct)
        tone = sine(f, mel_dur) * adsr(mel_dur, 0.1, 0.2, 0.4, 0.5) * 0.15
        end = min(offset + len(tone), n)
        result[offset:end] += tone[:end - offset]
    result = make_loopable(normalize(simple_reverb(result, 0.35, 80)))
    save_wav('Ambient/Planes/Feywild/ambient_feywild.wav', result)


# ---------------------------------------------------------------------------
# Creature SFX generators
# ---------------------------------------------------------------------------

def gen_dragon_roar():
    d = 2.0
    n = nsamples(d)
    result = np.zeros(n)
    # Deep growl: layered low sine waves with distortion
    growl = sine(60, d) * 0.4 + sine(90, d) * 0.3 + sine(120, d) * 0.2
    growl *= adsr(d, 0.1, 0.2, 0.8, 0.5)
    growl = np.tanh(growl * 3) * 0.5  # distortion
    result += growl
    # Roar: rising then falling noise
    roar = bandpass_noise(d, 200, 150) * 0.3
    roar_env = np.zeros(n)
    rise = int(0.4 * n)
    peak = int(0.2 * n)
    fall = n - rise - peak
    roar_env[:rise] = np.linspace(0, 1, rise)
    roar_env[rise:rise + peak] = 1.0
    roar_env[rise + peak:] = np.linspace(1, 0, fall)
    result += roar * roar_env
    save_wav('SFX/Creatures/dragon_roar.wav', normalize(result))


def gen_undead_moan():
    d = 1.5
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    # Low filtered sweep
    freq_sweep = 100 + 80 * np.sin(2 * np.pi * 0.5 * t)
    result = np.sin(2 * np.pi * np.cumsum(freq_sweep) / SAMPLE_RATE) * 0.4
    result *= adsr(d, 0.2, 0.2, 0.6, 0.4)
    # Add breathiness
    breath = bandpass_noise(d, 300, 200) * adsr(d, 0.3, 0.2, 0.3, 0.4) * 0.15
    result += breath
    save_wav('SFX/Creatures/undead_moan.wav', normalize(result))


def gen_monster_growl():
    d = 1.0
    n = nsamples(d)
    result = np.zeros(n)
    # Aggressive low noise burst
    growl = bandpass_noise(d, 150, 100) * adsr(d, 0.05, 0.1, 0.7, 0.3) * 0.5
    result += growl
    # Add sub-bass tone
    result += sine(50, d) * adsr(d, 0.05, 0.1, 0.6, 0.3) * 0.3
    # Distortion
    result = np.tanh(result * 2.5)
    save_wav('SFX/Creatures/monster_growl.wav', normalize(result))


# ---------------------------------------------------------------------------
# Naval SFX generators
# ---------------------------------------------------------------------------

def gen_wave_crash():
    d = 2.0
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    # Filtered noise swell
    noise = white_noise(d)
    # Envelope: swell up then decay
    env = np.zeros(n)
    swell = int(0.4 * n)
    env[:swell] = np.linspace(0, 1, swell)
    env[swell:] = np.linspace(1, 0, n - swell) ** 2
    result = noise * env * 0.5
    # Low-pass via simple moving average
    from scipy.signal import lfilter
    kernel_size = 20
    b = np.ones(kernel_size) / kernel_size
    result = lfilter(b, [1.0], result)
    save_wav('SFX/Naval/wave_crash.wav', normalize(result))


def gen_cannon_fire():
    d = 1.5
    n = nsamples(d)
    result = np.zeros(n)
    # Explosive burst
    burst = white_noise(0.05) * 0.8
    result[:len(burst)] += burst
    # Low boom
    boom = sine(40, 0.5) * exp_decay(0.5, 5) * 0.5
    result[:len(boom)] += boom
    # Low decay rumble
    rumble = bandpass_noise(d, 80, 60) * exp_decay(d, 3) * 0.3
    result[:len(rumble)] += rumble
    save_wav('SFX/Naval/cannon_fire.wav', normalize(result))


def gen_ship_creak():
    d = 2.0
    n = nsamples(d)
    t = np.linspace(0, d, n, endpoint=False)
    # Slow pitch-bent sine
    freq = 200 + 100 * np.sin(2 * np.pi * 0.3 * t)
    result = np.sin(2 * np.pi * np.cumsum(freq) / SAMPLE_RATE) * 0.3
    result *= adsr(d, 0.2, 0.3, 0.5, 0.5)
    # Add some noise for texture
    result += bandpass_noise(d, 400, 200) * adsr(d, 0.2, 0.3, 0.2, 0.5) * 0.1
    save_wav('SFX/Naval/ship_creak.wav', normalize(result))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("Generating placeholder audio for Shattered Arcana...")
    print(f"Output directory: {BASE_DIR}")
    print()

    generators = [
        # Music
        ("Music/Menu/menu_theme.wav", gen_menu_theme),
        ("Music/Overworld/overworld_theme.wav", gen_overworld_theme),
        ("Music/Combat/combat_light.wav", gen_combat_light),
        ("Music/Combat/combat_heavy.wav", gen_combat_heavy),
        ("Music/Combat/combat_epic.wav", gen_combat_epic),
        ("Music/Victory/victory_fanfare.wav", gen_victory_fanfare),
        ("Music/Victory/defeat_stinger.wav", gen_defeat_stinger),
        # UI SFX
        ("SFX/UI/btn_click.wav", gen_btn_click),
        ("SFX/UI/btn_hover.wav", gen_btn_hover),
        ("SFX/UI/notification.wav", gen_notification),
        ("SFX/UI/error.wav", gen_error),
        ("SFX/UI/turn_start.wav", gen_turn_start),
        ("SFX/UI/turn_end.wav", gen_turn_end),
        ("SFX/UI/build_complete.wav", gen_build_complete),
        ("SFX/UI/research_complete.wav", gen_research_complete),
        ("SFX/UI/spell_learned.wav", gen_spell_learned),
        ("SFX/UI/gold_gained.wav", gen_gold_gained),
        ("SFX/UI/menu_open.wav", gen_menu_open),
        ("SFX/UI/menu_close.wav", gen_menu_close),
        # Combat SFX
        ("SFX/Combat/sword_clash.wav", gen_sword_clash),
        ("SFX/Combat/sword_swing.wav", gen_sword_swing),
        ("SFX/Combat/arrow_loose.wav", gen_arrow_loose),
        ("SFX/Combat/arrow_impact.wav", gen_arrow_impact),
        ("SFX/Combat/shield_block.wav", gen_shield_block),
        ("SFX/Combat/unit_hit.wav", gen_unit_hit),
        ("SFX/Combat/unit_death.wav", gen_unit_death),
        # Magic SFX
        ("SFX/Magic/spell_fire.wav", gen_spell_fire),
        ("SFX/Magic/spell_ice.wav", gen_spell_ice),
        ("SFX/Magic/spell_lightning.wav", gen_spell_lightning),
        ("SFX/Magic/spell_death.wav", gen_spell_death),
        ("SFX/Magic/spell_nature.wav", gen_spell_nature),
        ("SFX/Magic/spell_healing.wav", gen_spell_healing),
        ("SFX/Magic/spell_arcane.wav", gen_spell_arcane),
        # Ambient
        ("Ambient/Planes/Aurelith/ambient_aurelith.wav", gen_ambient_aurelith),
        ("Ambient/Planes/Noctharion/ambient_noctharion.wav", gen_ambient_noctharion),
        ("Ambient/Planes/Verdantis/ambient_verdantis.wav", gen_ambient_verdantis),
        ("Ambient/Planes/Infernyx/ambient_infernyx.wav", gen_ambient_infernyx),
        ("Ambient/Planes/Aethermist/ambient_aethermist.wav", gen_ambient_aethermist),
        ("Ambient/Planes/Abyssal/ambient_abyssal.wav", gen_ambient_abyssal),
        ("Ambient/Planes/Ethereal/ambient_ethereal.wav", gen_ambient_ethereal),
        ("Ambient/Planes/Feywild/ambient_feywild.wav", gen_ambient_feywild),
        # Creatures
        ("SFX/Creatures/dragon_roar.wav", gen_dragon_roar),
        ("SFX/Creatures/undead_moan.wav", gen_undead_moan),
        ("SFX/Creatures/monster_growl.wav", gen_monster_growl),
        # Naval
        ("SFX/Naval/wave_crash.wav", gen_wave_crash),
        ("SFX/Naval/cannon_fire.wav", gen_cannon_fire),
        ("SFX/Naval/ship_creak.wav", gen_ship_creak),
    ]

    total_size = 0
    for name, gen_func in generators:
        print(f"  Generating {name}...", end=" ")
        gen_func()
        path = os.path.join(BASE_DIR, name)
        size = os.path.getsize(path)
        total_size += size
        print(f"({size:,} bytes)")

    print()
    print(f"Total files generated: {len(generators)}")
    print(f"Total size: {total_size:,} bytes ({total_size / 1024 / 1024:.1f} MB)")


if __name__ == '__main__':
    main()
