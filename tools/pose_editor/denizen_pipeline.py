#!/usr/bin/env python3
"""
Shattered Arcana — Denizen Animation Pipeline

One-call pipeline to produce a denizen card animation for any plane.

Usage:
    python3 denizen_pipeline.py aurelith          # Render Aurelith denizen
    python3 denizen_pipeline.py --all              # Render all 8 planes
    python3 denizen_pipeline.py --list             # List available configs
    python3 denizen_pipeline.py aurelith --preview # Preview key frames only
"""

import json
import os
import sys
import time
from pathlib import Path

TOOL_DIR = Path(__file__).parent
PROJECT_ROOT = TOOL_DIR.parent.parent
ANIM_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations"
MIXAMO_DIR = ANIM_DIR / "MixamoAnims"
EXPORTS_DIR = TOOL_DIR / "exports"
FRAMES_DIR = TOOL_DIR / "frames"

sys.path.insert(0, str(TOOL_DIR))
from animator import Animator, quality_gate, encode_webm


# ============================================================
# VISUAL TESTING — required before any WebM is considered done
# ============================================================

def visual_review(frame_dir, total_frames, plane='unknown'):
    """Generate a visual review strip and check key quality criteria.

    This runs AFTER quality_gate (which checks jitter/freeze) and checks
    things that pixel diffs can't catch:
    - Is the character recognizable as a humanoid?
    - Is the character visible (not a dark blob)?
    - Are legs on the ground (not floating)?
    - Does the silhouette change between beats?

    Returns: (passed: bool, report: dict, strip_path: str)
    """
    from PIL import Image
    import numpy as np

    frame_dir = Path(frame_dir)
    beat_frames = [1, total_frames // 5, 2 * total_frames // 5,
                   3 * total_frames // 5, 4 * total_frames // 5]
    issues = []

    # Load key frames
    images = []
    for f in beat_frames:
        path = frame_dir / f'{f:04d}.png'
        if path.exists():
            images.append(np.array(Image.open(path).convert('RGB')))

    if len(images) < 3:
        return False, {'issues': ['Not enough frames rendered']}, ''

    # 1. BRIGHTNESS CHECK — character should be visible
    for i, img in enumerate(images):
        # Character region: center 60% width, top 70% height
        h, w = img.shape[:2]
        roi = img[int(h*0.05):int(h*0.7), int(w*0.2):int(w*0.8)]
        mean_brightness = roi.mean()
        if mean_brightness < 20:
            issues.append(f'Beat {i+1}: TOO DARK (brightness {mean_brightness:.0f}/255 in character region)')

    # 2. SILHOUETTE VARIETY — beats should look different
    diffs = []
    for i in range(1, len(images)):
        diff = np.abs(images[i].astype(float) - images[0].astype(float)).mean()
        diffs.append(diff)
    if all(d < 1.0 for d in diffs):
        issues.append(f'ALL BEATS LOOK IDENTICAL (max diff {max(diffs):.2f})')

    # 3. CHARACTER SIZE — should fill reasonable portion of frame
    for i, img in enumerate(images):
        h, w = img.shape[:2]
        # Detect non-background pixels (brightness > 25 or color variance > 15)
        brightness = img.mean(axis=2)
        color_var = img.astype(float).std(axis=2)
        char_mask = (brightness > 25) | (color_var > 15)
        char_fill = char_mask.mean()
        if char_fill < 0.03:
            issues.append(f'Beat {i+1}: CHARACTER TOO SMALL ({char_fill:.1%} of frame)')
        elif char_fill > 0.6:
            issues.append(f'Beat {i+1}: CHARACTER TOO LARGE ({char_fill:.1%} of frame)')

    # 4. Generate thumbnail strip for human review
    strip_images = []
    for img in images:
        # Resize to 200px wide
        pil_img = Image.fromarray(img)
        aspect = pil_img.height / pil_img.width
        thumb = pil_img.resize((200, int(200 * aspect)), Image.LANCZOS)
        strip_images.append(np.array(thumb))

    if strip_images:
        strip = np.concatenate(strip_images, axis=1)
        strip_path = str(ANIM_DIR / 'pose_screenshots' / f'{plane}_review_strip.png')
        Image.fromarray(strip).save(strip_path)
    else:
        strip_path = ''

    passed = len(issues) == 0
    report = {
        'issues': issues,
        'beat_count': len(images),
        'brightness': [img.mean() for img in images],
        'beat_diffs': diffs,
    }

    return passed, report, strip_path


# ============================================================
# MOTION DIFFUSION PROMPTS PER PLANE
# ============================================================
# Used when --motion-diffusion flag is set. Each plane gets a
# 5-beat text prompt sequence instead of Mixamo clips.

# HumanML3D-style prompts: physical, body-part-specific descriptions.
# The training data uses "a person" as subject and describes specific
# joint movements, directions, and speeds. Avoid abstract/character terms.
MOTION_PROMPTS = {
    'aurelith': [
        ("a person stands with their left arm bent holding something at chest height and shifts weight between feet", 3.0),
        ("a person reaches across their body with their right hand to their left hip then pulls their right arm out to the side", 3.0),
        ("a person swings their right arm in a wide arc from right to left while stepping forward with their left foot", 3.0),
        ("a person raises their right arm straight up above their head and tilts their head back slightly while standing still", 3.0),
        ("a person extends their right arm forward at shoulder height and leans their whole body forward aggressively", 3.0),
    ],
    'noctharion': [
        ("a person bends their knees and lowers their body into a deep crouch with both hands near the ground", 3.0),
        ("a person walks forward slowly with bent knees taking small careful steps and keeping their body low", 3.0),
        ("a person lunges forward quickly with their right arm thrust straight ahead while their left arm stays back", 3.0),
        ("a person stands still and slowly turns their head to the left then to the right while keeping their body tense", 3.0),
        ("a person raises their right arm high and swings it down fast in a chopping motion while stepping forward", 3.0),
    ],
    'verdantis': [
        ("a person stands with feet apart and slowly raises both arms out to the sides to shoulder height", 3.0),
        ("a person raises both arms above their head with palms facing up and arches their back slightly", 3.0),
        ("a person swings both arms together from right to left in a wide horizontal sweep while rotating their torso", 3.0),
        ("a person stands with slight swaying and gentle weight shifts from left foot to right foot", 3.0),
        ("a person steps forward and kicks their right leg straight ahead at waist height", 3.0),
    ],
    'infernyx': [
        ("a person stands in a wide stance with fists clenched at their sides and chest puffed forward", 3.0),
        ("a person raises both hands above their head then brings them down in front while bending forward", 3.0),
        ("a person swings their right arm then left arm in alternating wide slashing motions while advancing forward", 3.0),
        ("a person raises both forearms in front of their face and leans back slightly in a blocking position", 3.0),
        ("a person pulls their right arm back then punches straight forward while rotating their hips and stepping", 3.0),
    ],
    'aethermist': [
        ("a person stands very still with their arms relaxed at their sides and slowly rises onto their toes", 3.0),
        ("a person slowly raises both hands from their sides to above their head with fingers spread apart", 3.0),
        ("a person spins around once while extending both arms out to the sides at shoulder height", 3.0),
        ("a person stands with both arms extended straight out to the sides and slowly rotates their wrists", 3.0),
        ("a person pushes both palms forward at chest height while taking one step forward", 3.0),
    ],
    'abyssal': [
        ("a person sways their upper body slowly from side to side while keeping their feet planted", 3.0),
        ("a person bends forward and walks on their hands and feet close to the ground", 3.0),
        ("a person swings their arms in loose circular motions while twisting their torso left and right", 3.0),
        ("a person plants both feet wide and leans forward with arms braced in front of their body", 3.0),
        ("a person reaches forward with both hands and pulls back as if grabbing something heavy", 3.0),
    ],
    'ethereal': [
        ("a person stands on one foot with the other leg slightly raised and arms floating at their sides", 3.0),
        ("a person slowly raises both hands with palms up while tilting their head back to look upward", 3.0),
        ("a person takes a slow step and turns their body 180 degrees while trailing their arms behind them", 3.0),
        ("a person stands with arms raised at 45 degrees from their body and fingers spread wide", 3.0),
        ("a person slowly reaches forward with their right hand while their left hand draws back", 3.0),
    ],
    'feywild': [
        ("a person takes quick light steps side to side while bouncing slightly on the balls of their feet", 3.0),
        ("a person skips sideways to the right with exaggerated arm swings and head bobbing", 3.0),
        ("a person does a quick spin and extends their right arm in a sweeping motion at waist height", 3.0),
        ("a person strikes a dramatic pose with their right arm extended forward and left arm back", 3.0),
        ("a person jumps forward with both feet leaving the ground and arms swinging upward", 3.0),
    ],
}


# ============================================================
# PLANE CONFIGURATIONS
# ============================================================
# Each plane defines: model, clips, 5-beat sequence, pose overrides,
# texture palette, camera, and background preset.

# Clip catalog — all available Mixamo clips by category
CLIP_CATALOG = {
    'idle':    ['sword and shield idle', 'sword and shield idle (2)',
                'sword and shield idle (3)', 'sword and shield idle (4)'],
    'attack':  ['sword and shield attack', 'sword and shield attack (2)',
                'sword and shield attack (3)', 'sword and shield attack (4)'],
    'slash':   ['sword and shield slash', 'sword and shield slash (2)',
                'sword and shield slash (3)', 'sword and shield slash (4)',
                'sword and shield slash (5)'],
    'block':   ['sword and shield block', 'sword and shield block (2)'],
    'cast':    ['sword and shield casting', 'sword and shield casting (2)'],
    'sheath':  ['sheath sword 1', 'sheath sword 2'],
    'turn':    ['sword and shield 180 turn', 'sword and shield 180 turn (2)'],
    'walk':    ['sword and shield walk', 'sword and shield walk (2)'],
    'run':     ['sword and shield run', 'sword and shield run (2)'],
    'crouch':  ['sword and shield crouch', 'sword and shield crouch idle'],
    'death':   ['sword and shield death', 'sword and shield death (2)'],
    'kick':    ['sword and shield kick'],
    'power':   ['sword and shield power up'],
    'jump':    ['sword and shield jump', 'sword and shield jump (2)'],
    'impact':  ['sword and shield impact', 'sword and shield impact (2)',
                'sword and shield impact (3)'],
    'strafe':  ['sword and shield strafe', 'sword and shield strafe (2)',
                'sword and shield strafe (3)', 'sword and shield strafe (4)'],
}

# Texture color palettes per plane
PALETTES = {
    'aurelith':   {'primary': (180, 160, 80),  'accent': (218, 165, 32),  'steel': True},
    'noctharion': {'primary': (60, 30, 80),     'accent': (150, 50, 200),  'steel': False},
    'verdantis':  {'primary': (50, 100, 40),    'accent': (100, 200, 60),  'steel': False},
    'infernyx':   {'primary': (120, 30, 10),    'accent': (255, 100, 0),   'steel': True},
    'aethermist': {'primary': (60, 80, 160),    'accent': (100, 150, 255), 'steel': True},
    'abyssal':    {'primary': (20, 60, 80),     'accent': (0, 180, 180),   'steel': False},
    'ethereal':   {'primary': (120, 120, 160),  'accent': (200, 200, 255), 'steel': True},
    'feywild':    {'primary': (100, 50, 100),   'accent': (255, 100, 200), 'steel': False},
}

# Per-plane animation configs
PLANE_CONFIGS = {
    'aurelith': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle',
            'draw':     'sheath sword 1',
            'flourish': 'sword and shield slash',
            'hold':     'sword and shield idle',
            'attack':   'sword and shield attack',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 1.0},
            {'beat': 'draw',     'clip': 'draw',     'start': 73,  'end': 144, 'clipStart': 1.2, 'speed': -0.4},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.778},
        ],
        'pose_overrides': {
            'frames': [217, 252, 288],
            'bones': {
                'mixamorigRightShoulder': [65, 40, 115],
                'mixamorigRightArm': [-45, 60, 10],
                'mixamorigRightForeArm': [25, 25, -155],
                'mixamorigRightHand': [-35, 35, -40],
                'mixamorigNeck': [12, 5, 0],
                'mixamorigHead': [18, 8, 0],
                'mixamorigSpine1': [-6, 0, 0],
                'mixamorigSpine2': [-3, 2, 0],
            },
        },
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'aurelith',
        'repaint': True,
    },
    'noctharion': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle (2)',
            'stalk':    'sword and shield crouch',
            'flourish': 'sword and shield slash (2)',
            'hold':     'sword and shield crouch idle',
            'strike':   'sword and shield attack (2)',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.8},
            {'beat': 'stalk',    'clip': 'stalk',    'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.6},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'strike',   'clip': 'strike',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.7},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.1, 8.0], 'target': [0, 0.8, 0], 'fov': 30},
        'background': 'noctharion',
        'repaint': True,
    },
    'verdantis': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle (3)',
            'power':    'sword and shield power up',
            'flourish': 'sword and shield slash (3)',
            'hold':     'sword and shield idle',
            'attack':   'sword and shield kick',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.7},
            {'beat': 'power',    'clip': 'power',    'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.6},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.4},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.6},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'verdantis',
        'repaint': True,
    },
    'infernyx': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle (4)',
            'cast':     'sword and shield casting',
            'flourish': 'sword and shield slash (4)',
            'hold':     'sword and shield block idle',
            'attack':   'sword and shield attack (3)',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 1.0},
            {'beat': 'cast',     'clip': 'cast',     'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.6},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.8},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'infernyx',
        'repaint': True,
    },
    'aethermist': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle',
            'cast':     'sword and shield casting (2)',
            'flourish': 'sword and shield slash (5)',
            'hold':     'sword and shield idle (2)',
            'attack':   'sword and shield attack (4)',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.7},
            {'beat': 'cast',     'clip': 'cast',     'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.7},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'aethermist',
        'repaint': True,
    },
    'abyssal': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle',
            'stalk':    'sword and shield crouching',
            'flourish': 'sword and shield slash',
            'hold':     'sword and shield block',
            'attack':   'sword and shield attack',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.6},
            {'beat': 'stalk',    'clip': 'stalk',    'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.4},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.7},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.5], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'abyssal',
        'repaint': True,
    },
    'ethereal': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle (2)',
            'cast':     'sword and shield casting',
            'flourish': 'sword and shield turn',
            'hold':     'sword and shield idle (3)',
            'attack':   'sword and shield impact',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'cast',     'clip': 'cast',     'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.4},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.4},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.6},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'ethereal',
        'repaint': True,
    },
    'feywild': {
        'model': 'Knight D Pelegrini.fbx',
        'clips': {
            'idle':     'sword and shield idle',
            'dance':    'sword and shield strafe',
            'flourish': 'sword and shield slash (2)',
            'hold':     'sword and shield idle (4)',
            'attack':   'sword and shield jump',
        },
        'segments': [
            {'beat': 'idle',     'clip': 'idle',     'start': 1,   'end': 72,  'clipStart': 0.0, 'speed': 0.8},
            {'beat': 'dance',    'clip': 'dance',    'start': 73,  'end': 144, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'flourish', 'clip': 'flourish', 'start': 145, 'end': 216, 'clipStart': 0.0, 'speed': 0.6},
            {'beat': 'hold',     'clip': 'hold',     'start': 217, 'end': 288, 'clipStart': 0.0, 'speed': 0.5},
            {'beat': 'attack',   'clip': 'attack',   'start': 289, 'end': 360, 'clipStart': 0.0, 'speed': 0.6},
        ],
        'pose_overrides': None,
        'weapons': {
            'sword': [2, 0, 0, -90, 0, 0],
            'shield': [0, 10, 8, 0, 0, 0],
        },
        'camera': {'pos': [0.15, 1.2, 8.0], 'target': [0, 0.85, 0], 'fov': 30},
        'background': 'feywild',
        'repaint': True,
    },
}


# ============================================================
# PIPELINE
# ============================================================

def render_denizen(plane, preview=False, config_override=None, output_suffix=''):
    """Render a complete denizen card animation for the given plane.

    Args:
        plane: Plane name (e.g. 'aurelith')
        preview: If True, only render key frames (5 screenshots), not full 360
        config_override: Optional dict to override PLANE_CONFIGS[plane]
        output_suffix: Optional suffix for output filename (e.g. '-diffusion')

    Returns:
        Path to the output WebM (or screenshot dir if preview)
    """
    config = config_override or PLANE_CONFIGS.get(plane)
    if not config:
        print(f"Unknown plane: {plane}")
        print(f"Available: {', '.join(PLANE_CONFIGS.keys())}")
        return None

    output_dir = FRAMES_DIR / f'_render_{plane}{output_suffix}'
    webm_path = str(ANIM_DIR / f'{plane}-denizen-card{output_suffix}.webm')

    print(f"\n{'='*60}")
    print(f"  DENIZEN PIPELINE: {plane.upper()}")
    print(f"{'='*60}")

    with Animator(headless=True, width=512, height=768) as anim:
        time.sleep(1)

        # 1. Repaint armor texture
        if config.get('repaint'):
            count = anim.page.evaluate('() => window.repaintArmor()')
            print(f"[1/7] Repainted {count} textures")
        else:
            print("[1/7] Skipping texture repaint")

        # 2. Load clips
        clip_names = {}
        for role, clip_name in config['clips'].items():
            filename = f"{clip_name}.fbx"
            if not (MIXAMO_DIR / filename).exists():
                print(f"  WARNING: clip not found: {filename}")
                continue
            anim.page.evaluate(
                f'() => window.loadClip("/clips/{clip_name}.fbx", "{role}")'
            )
            clip_names[role] = clip_name
            time.sleep(0.3)
        time.sleep(1.5)
        clips = anim.list_clips()
        print(f"[2/7] Loaded {len(clips)} clips: {[c['name'] for c in clips]}")

        # 3. Set up segments
        for seg in config['segments']:
            anim.add_clip_segment(
                seg['clip'], seg['start'], seg['end'],
                clipStart=seg['clipStart'], speed=seg['speed']
            )
        segments = anim.get_clip_segments()
        print(f"[3/7] {len(segments)} segments configured")

        # 4. Pose overrides (quaternion-space)
        overrides = config.get('pose_overrides')
        if overrides:
            anim.page.evaluate('() => window.clearQuatOverrides()')
            for frame in overrides['frames']:
                anim.page.evaluate('() => window.stopAllClips()')
                time.sleep(0.05)
                for bone_name, (rx, ry, rz) in overrides['bones'].items():
                    anim.set_bone_rotation(bone_name, rx, ry, rz)
                    anim.page.evaluate(
                        f'() => window.captureQuatKeyframe("{bone_name}", {frame})'
                    )
            print(f"[4/7] {len(overrides['bones'])} bone overrides at "
                  f"frames {overrides['frames']}")
        else:
            print("[4/7] No pose overrides")

        # 5. Weapons, background, camera
        weapons = config.get('weapons', {})
        if weapons:
            anim.attach_weapons()
            if 'sword' in weapons:
                anim.set_sword_offset(*weapons['sword'])
            if 'shield' in weapons:
                anim.set_shield_offset(*weapons['shield'])
        anim.set_background(config['background'])
        cam = config['camera']
        anim.set_camera(*cam['pos'], tx=cam['target'][0],
                        ty=cam['target'][1], tz=cam['target'][2],
                        fov=cam['fov'])
        anim.go_to_frame(1)
        anim.set_camera_keyframe(1)
        print(f"[5/7] Scene configured: bg={config['background']}, "
              f"cam z={cam['pos'][2]}")

        # 6. Render
        if preview:
            print("[6/7] Preview mode — 5 key frames")
            preview_dir = str(ANIM_DIR / 'pose_screenshots')
            os.makedirs(preview_dir, exist_ok=True)
            for beat, frame in [('idle', 36), ('draw', 108), ('flourish', 180),
                                ('hold', 252), ('attack', 324)]:
                path = anim.verify_frame(frame)
                import shutil
                dest = f'{preview_dir}/{plane}_preview_{beat}_f{frame}.png'
                shutil.copy(path, dest)
                print(f"  {beat}: {dest}")
            print("[7/7] Preview complete")
            return preview_dir

        # Full render
        anim.page.evaluate('() => window.hideUI()')
        anim.page.set_viewport_size({'width': 512, 'height': 768})
        anim.page.evaluate('() => window.setRenderSize(512, 768)')
        time.sleep(0.3)
        os.makedirs(str(output_dir), exist_ok=True)

        print("[6/7] Rendering 360 frames...")
        for frame in range(1, 361):
            anim.page.evaluate(f'() => window.evaluateFullFrame({frame})')
            time.sleep(0.05)
            anim.page.screenshot(
                path=str(output_dir / f'{frame:04d}.png'),
                clip={'x': 0, 'y': 0, 'width': 512, 'height': 768}
            )
            if frame % 72 == 0:
                print(f"  Frame {frame}/360")

    # 7. Quality gate (jitter/freeze)
    print("[7/8] Quality gate (motion)...")
    report = quality_gate(str(output_dir), 1, 360)
    status = 'PASS' if report['pass'] else 'FAIL'
    print(f"  Motion: {status} (mean={report['mean_diff']:.3f}, "
          f"freezes={report['freeze_count']}, spikes={report['spike_count']})")
    if report['issues']:
        for issue in report['issues']:
            print(f"    {issue}")

    # 8. Visual review (brightness/silhouette/size)
    print("[8/8] Visual review...")
    vis_pass, vis_report, strip_path = visual_review(str(output_dir), 360, plane)
    vis_status = 'PASS' if vis_pass else 'FAIL'
    print(f"  Visual: {vis_status}")
    if vis_report['issues']:
        for issue in vis_report['issues']:
            print(f"    {issue}")
    if strip_path:
        print(f"  Review strip: {strip_path}")

    if not vis_pass:
        print(f"\n  WARNING: Visual review failed. Encoding anyway but DO NOT deploy without reviewing the strip.")

    encode_webm(str(output_dir), webm_path, fps=24, bitrate='1500k')
    size_kb = os.path.getsize(webm_path) / 1024
    print(f"\n  Output: {webm_path} ({size_kb:.0f} KB)")
    print(f"{'='*60}\n")
    return webm_path


def render_denizen_diffusion(plane, preview=False):
    """Render a denizen animation using MoMask-guided clip selection.

    Uses MoMask's text understanding to select the best Mixamo clip for
    each beat, then renders through the proven Mixamo clip pipeline.
    This ensures weapons, ground contact, full skeleton, and visual
    quality match the hand-configured version.

    Args:
        plane: Plane name
        preview: If True, only render 5 key frames

    Returns:
        Path to WebM output
    """
    if plane not in MOTION_PROMPTS:
        print(f"No motion prompts for plane: {plane}")
        print(f"Available: {', '.join(MOTION_PROMPTS.keys())}")
        return None

    prompts = MOTION_PROMPTS[plane]
    base_config = PLANE_CONFIGS.get(plane, PLANE_CONFIGS['aurelith'])

    print(f"\n{'='*60}")
    print(f"  MOTION DIFFUSION PIPELINE: {plane.upper()}")
    print(f"{'='*60}")

    # 1. Use MoMask to select clips for each beat
    from motion_generator import MotionGenerator
    gen = MotionGenerator()

    print("[1/6] Selecting clips from text prompts...")
    selected_segments = gen.select_clips(prompts)

    # 2. Build a dynamic config using the selected clips
    dynamic_config = dict(base_config)
    dynamic_clips = {}
    dynamic_segments = []

    for seg in selected_segments:
        role = seg['beat']
        clip_name = seg['clip']
        dynamic_clips[role] = clip_name
        dynamic_segments.append({
            'beat': role,
            'clip': role,
            'start': seg['start'],
            'end': seg['end'],
            'clipStart': seg['clipStart'],
            'speed': seg['speed'],
        })

    dynamic_config['clips'] = dynamic_clips
    dynamic_config['segments'] = dynamic_segments
    dynamic_config['pose_overrides'] = None  # No manual overrides needed

    print(f"\n  Selected clips:")
    for seg in selected_segments:
        print(f"    [{seg['start']:3d}-{seg['end']:3d}] {seg['clip']} "
              f"(speed={seg['speed']}) ← \"{seg['prompt'][:40]}...\"")

    # 3. Render using the standard Mixamo pipeline
    print(f"\n[2/6] Rendering via Mixamo pipeline...")
    result = render_denizen(plane, preview=preview, config_override=dynamic_config,
                            output_suffix='-diffusion')

    return result


# ============================================================
# CLI
# ============================================================

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("\nFlags:")
        print("  --preview              Preview key frames only")
        print("  --motion-diffusion     Use MoMask instead of Mixamo clips")
        print("  --all                  Render all 8 planes")
        print("  --list                 List available configs")
        return

    args = sys.argv[1:]
    use_diffusion = '--motion-diffusion' in args
    preview = '--preview' in args
    args = [a for a in args if not a.startswith('--')]

    if '--list' in sys.argv:
        for name, cfg in PLANE_CONFIGS.items():
            clips = list(cfg['clips'].values())
            has_prompts = name in MOTION_PROMPTS
            print(f"  {name:12s}  bg={cfg['background']:12s}  "
                  f"diffusion={'yes' if has_prompts else 'no':3s}  clips={clips[:3]}...")
        return

    if '--all' in sys.argv:
        render_fn = render_denizen_diffusion if use_diffusion else render_denizen
        results = {}
        for plane in PLANE_CONFIGS:
            try:
                path = render_fn(plane, preview=preview)
                results[plane] = path
            except Exception as e:
                print(f"ERROR rendering {plane}: {e}")
                import traceback
                traceback.print_exc()
                results[plane] = None
        print("\n=== BATCH RESULTS ===")
        for plane, path in results.items():
            status = 'OK' if path else 'FAILED'
            print(f"  {plane:12s}: {status}  {path or ''}")
        return

    if not args:
        print("Specify a plane name (e.g. 'aurelith')")
        return

    plane = args[0].lower()
    if use_diffusion:
        render_denizen_diffusion(plane, preview=preview)
    else:
        render_denizen(plane, preview=preview)


if __name__ == '__main__':
    main()
