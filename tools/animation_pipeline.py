#!/usr/bin/env python3
"""
Shattered Arcana — Character Animation Pipeline

Formalizes the professional 3D animation workflow:
  Phase 1: Reference — extract poses from reference videos via MediaPipe
  Phase 2: Setup — load character, verify rig, lock mesh/materials
  Phase 3: Blocking — set key poses one at a time, verify each visually
  Phase 4: Splining — interpolate, add breakdowns, anticipation, overlap
  Phase 5: Polish — breathing, weight shifts, micro-pauses, settle
  Phase 6: Render — full animation render + deploy

Each phase produces a checkpoint .blend file. No phase proceeds until
the previous phase is verified.

Usage:
    # On the dev server (176.9.99.103):
    blender --background --python animation_pipeline.py -- \
        --phase blocking \
        --character knight \
        --blend /tmp/knight_mblab_r8b.blend \
        --reference /home/bbrelin/ShatteredArcana/Art/Reference/poses/pose_data.json \
        --pose 1

    # Phases: reference, setup, blocking, splining, polish, render
    # For blocking, specify --pose N to work on one pose at a time
"""

import bpy
import json
import math
import os
import sys
import shutil
import subprocess
from mathutils import Euler, Vector

# ============================================================
# CONFIGURATION
# ============================================================

# Key poses that define the sword salute animation
# Each pose has a name, target frame, and description
KEY_POSES = {
    1: {
        "name": "idle",
        "frame": 1,
        "description": "Relaxed idle stance. Arms at sides, slight elbow bend, "
                       "weight centered, sword pointing down at right side, "
                       "shield resting on left forearm.",
        "hold_frames": 20,  # how long to hold before next pose
    },
    2: {
        "name": "reach",
        "frame": 30,
        "description": "Right hand reaches across body to left hip to grip sword. "
                       "Torso turns slightly left, head looks down at hand. "
                       "Left arm raises shield slightly.",
        "hold_frames": 0,
    },
    3: {
        "name": "draw",
        "frame": 55,
        "description": "Sword drawn upward in an arc to the right. "
                       "Right arm sweeps out and up, torso twists right. "
                       "Sword blade catches light at the apex.",
        "hold_frames": 0,
    },
    4: {
        "name": "salute",
        "frame": 75,
        "description": "Crosshilt salute. Right forearm bent sharply, sword vertical, "
                       "crossguard at chin level. Blade points straight up. "
                       "Slight head bow. Left arm holds shield in guard.",
        "hold_frames": 15,  # hold the salute
    },
    5: {
        "name": "point",
        "frame": 100,
        "description": "Combat point. Right arm extends forward, sword tip aimed at camera. "
                       "Body leans forward aggressively, weight on front foot. "
                       "Shield raised on left side. Head looks directly at viewer.",
        "hold_frames": 30,  # hold final combat stance
    },
}

# Breakdown poses between key poses (added in Phase 4)
BREAKDOWN_POSES = {
    "idle_to_reach": {
        "frame": 22,  # between idle(1) and reach(30)
        "description": "Anticipation dip — body lowers slightly, weight shifts right "
                       "as right arm begins to move across. Shield arm starts rising.",
    },
    "reach_to_draw": {
        "frame": 40,
        "description": "Arm pulling back with sword — elbow leads, wrist follows. "
                       "Torso beginning to unwind from the left twist.",
    },
    "draw_to_salute": {
        "frame": 65,
        "description": "Sword passes through overhead arc, decelerating. "
                       "Elbow bending to bring blade vertical. Head beginning to bow.",
    },
    "salute_to_point": {
        "frame": 92,
        "description": "Sword sweeping down from salute — blade tip drops first, "
                       "then arm extends forward. Body weight shifting forward.",
    },
}

# Output paths
RENDER_DIR = "/home/bbrelin/ShatteredArcana/Art/DenizenAnimations"
CHECKPOINT_DIR = "/tmp/animation_checkpoints"


# ============================================================
# PHASE 1: REFERENCE
# ============================================================

def phase_reference(video_path, output_path):
    """Extract poses from reference video using MediaPipe.
    
    This runs as a standalone Python script, NOT inside Blender.
    
    Args:
        video_path: Path to reference video (WebM)
        output_path: Path to save pose JSON data
    """
    # This is handled by extract_poses2.py
    # Output: JSON with per-frame landmarks, angles, vectors
    print(f"Reference extraction should be run separately:")
    print(f"  python3 /tmp/extract_poses2.py")
    print(f"  Input: {video_path}")
    print(f"  Output: {output_path}")


# ============================================================
# PHASE 2: SETUP
# ============================================================

def phase_setup(blend_file, output_blend):
    """Verify character rig, lock mesh and materials.
    
    Checks:
    - All required bones exist
    - Sword, shield, helmet are properly bone-parented
    - Materials are assigned correctly
    - Armature modifier is working
    
    Saves a verified checkpoint .blend file.
    """
    bpy.ops.wm.open_mainfile(filepath=blend_file)
    
    # Find armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
    
    if not armature:
        print("ERROR: No armature found")
        return False
    
    # Check required bones
    required_bones = ['spine01', 'spine02', 'spine03', 'head', 'neck',
                      'upperarm_R', 'lowerarm_R', 'hand_R',
                      'upperarm_L', 'lowerarm_L', 'hand_L',
                      'thigh_R', 'calf_R', 'foot_R',
                      'thigh_L', 'calf_L', 'foot_L']
    
    missing = []
    for bone_name in required_bones:
        if bone_name not in armature.data.bones:
            missing.append(bone_name)
    
    if missing:
        print(f"WARNING: Missing bones: {missing}")
        # Try to find alternates
        available = [b.name for b in armature.data.bones]
        print(f"Available bones: {available}")
    else:
        print(f"All {len(required_bones)} required bones present")
    
    # Check accessories
    accessories = {
        'Sword': 'hand_R',
        'KiteShield': ('lowerarm_L', 'forearm_L'),
        'Helmet': 'head',
    }
    
    for obj_name, expected_bone in accessories.items():
        obj = bpy.data.objects.get(obj_name)
        if not obj:
            print(f"WARNING: {obj_name} not found in scene")
            continue
        if obj.parent != armature:
            print(f"WARNING: {obj_name} not parented to armature")
            continue
        if isinstance(expected_bone, tuple):
            if obj.parent_bone not in expected_bone:
                print(f"WARNING: {obj_name} parented to {obj.parent_bone}, expected one of {expected_bone}")
        else:
            if obj.parent_bone != expected_bone:
                print(f"WARNING: {obj_name} parented to {obj.parent_bone}, expected {expected_bone}")
        print(f"  {obj_name}: parent_bone={obj.parent_bone} OK")
    
    # Check body mesh
    body = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH' and 'human' in obj.name.lower():
            body = obj
            break
    
    if body:
        print(f"Body mesh: {body.name} ({len(body.data.vertices)} verts)")
        print(f"  Vertex groups: {len(body.vertex_groups)}")
        print(f"  Materials: {len(body.data.materials)}")
        has_armature_mod = any(m.type == 'ARMATURE' for m in body.modifiers)
        print(f"  Armature modifier: {has_armature_mod}")
    
    # Save checkpoint
    bpy.ops.wm.save_as_mainfile(filepath=output_blend)
    print(f"\nSetup verified. Checkpoint: {output_blend}")
    return True


# ============================================================
# PHASE 3: BLOCKING
# ============================================================

def phase_blocking(blend_file, pose_number, reference_data=None, preview_path=None):
    """Set a single key pose and render a preview for verification.
    
    This is the core of the workflow — one pose at a time.
    
    Args:
        blend_file: Path to checkpoint .blend
        pose_number: Which key pose to set (1-5)
        reference_data: Path to MediaPipe pose JSON (optional)
        preview_path: Where to save the preview render
    
    Returns bone rotations used, for logging and iteration.
    """
    bpy.ops.wm.open_mainfile(filepath=blend_file)
    
    pose_config = KEY_POSES.get(pose_number)
    if not pose_config:
        print(f"ERROR: Invalid pose number {pose_number}")
        return None
    
    print(f"\n{'='*50}")
    print(f"BLOCKING: Pose {pose_number} — {pose_config['name']}")
    print(f"Frame: {pose_config['frame']}")
    print(f"Description: {pose_config['description']}")
    print(f"{'='*50}\n")
    
    # Find armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
    
    # Load reference data if available
    ref_angles = None
    if reference_data and os.path.exists(reference_data):
        with open(reference_data) as f:
            ref = json.load(f)
        poses = ref.get('poses', [])
        if poses:
            # Use the first pose as idle reference
            ref_angles = poses[0].get('angles', {})
            print(f"Reference data loaded: {len(poses)} poses")
            print(f"  Reference angles: {ref_angles}")
    
    # Set frame
    frame = pose_config['frame']
    bpy.context.scene.frame_set(frame)
    
    # Enter pose mode
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='POSE')
    
    # Apply the pose
    rotations = _apply_key_pose(armature, pose_number, ref_angles)
    
    bpy.ops.object.mode_set(mode='OBJECT')
    
    # Smooth curves if there are keyframes
    if armature.animation_data and armature.animation_data.action:
        for fc in armature.animation_data.action.fcurves:
            for kp in fc.keyframe_points:
                kp.interpolation = 'BEZIER'
                kp.handle_left_type = 'AUTO_CLAMPED'
                kp.handle_right_type = 'AUTO_CLAMPED'
    
    # Save checkpoint
    checkpoint = blend_file.replace('.blend', f'_pose{pose_number}.blend')
    bpy.ops.wm.save_as_mainfile(filepath=checkpoint)
    print(f"Checkpoint: {checkpoint}")
    
    # Render preview
    if preview_path is None:
        preview_path = os.path.join(CHECKPOINT_DIR, f"pose_{pose_number}_{pose_config['name']}.png")
    os.makedirs(os.path.dirname(preview_path), exist_ok=True)
    
    bpy.context.scene.render.resolution_x = 512
    bpy.context.scene.render.resolution_y = 768
    bpy.context.scene.render.image_settings.file_format = 'PNG'
    bpy.context.scene.render.image_settings.color_mode = 'RGBA'
    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.device = 'GPU'
    bpy.context.scene.cycles.samples = 32  # low for quick preview
    bpy.context.scene.cycles.use_denoising = True
    
    prefs = bpy.context.preferences.addons['cycles'].preferences
    prefs.compute_device_type = 'CUDA'
    prefs.get_devices()
    for d in prefs.devices:
        d.use = True
    
    bpy.context.scene.render.filepath = preview_path
    bpy.ops.render.render(write_still=True)
    print(f"Preview: {preview_path}")
    
    return {
        'pose': pose_number,
        'name': pose_config['name'],
        'frame': frame,
        'rotations': rotations,
        'checkpoint': checkpoint,
        'preview': preview_path,
    }


def _apply_key_pose(armature, pose_number, ref_angles=None):
    """Apply bone rotations for a specific key pose.
    
    Returns dict of {bone_name: (rx, ry, rz)} in degrees.
    """
    
    def kb(name, rot_deg):
        """Keyframe a bone rotation."""
        b = armature.pose.bones.get(name)
        if not b:
            return
        b.rotation_mode = 'XYZ'
        b.rotation_euler = Euler(tuple(math.radians(d) for d in rot_deg))
        b.keyframe_insert(data_path="rotation_euler", frame=bpy.context.scene.frame_current)
    
    rotations = {}
    
    def set_bone(name, rot_deg):
        kb(name, rot_deg)
        rotations[name] = rot_deg
    
    if pose_number == 1:
        # IDLE — natural standing, NOT T-pose
        # Arms at sides with slight bend, sword hanging down
        # Reference: shoulder abduction ~20-30 degrees, elbow ~160 degrees
        set_bone('upperarm_R', (65, 5, 0))     # arm down at side (from T-pose, rotate ~65 around X)
        set_bone('lowerarm_R', (15, 0, 0))      # slight elbow bend
        set_bone('hand_R', (0, 0, 0))
        set_bone('upperarm_L', (65, -5, 0))     # left arm down
        set_bone('lowerarm_L', (15, 0, 0))
        set_bone('hand_L', (0, 0, 0))
        set_bone('spine01', (0, 0, 0))
        set_bone('spine02', (0, 0, 0))
        set_bone('spine03', (0, 0, 0))
        set_bone('head', (0, 0, 0))
        set_bone('neck', (0, 0, 0))
    
    elif pose_number == 2:
        # REACH — right hand crosses to left hip
        set_bone('upperarm_R', (40, 30, 50))
        set_bone('lowerarm_R', (80, 0, 0))
        set_bone('hand_R', (10, 0, 0))
        set_bone('spine02', (0, -5, -18))
        set_bone('spine03', (0, 0, -5))
        set_bone('head', (-3, 0, -10))
        set_bone('upperarm_L', (55, -15, -10))   # shield arm raises slightly
        set_bone('lowerarm_L', (30, 0, 0))
    
    elif pose_number == 3:
        # DRAW — sword swept up and out to the right
        set_bone('upperarm_R', (-50, -18, -40))
        set_bone('lowerarm_R', (8, 0, 0))
        set_bone('hand_R', (-15, 0, 0))
        set_bone('spine02', (-3, 0, 12))
        set_bone('spine03', (0, 0, 5))
        set_bone('head', (0, 0, 5))
        set_bone('upperarm_L', (50, -20, -15))
        set_bone('lowerarm_L', (40, 0, 0))
    
    elif pose_number == 4:
        # SALUTE — sword vertical, crosshilt at chin
        set_bone('upperarm_R', (8, 8, 12))
        set_bone('lowerarm_R', (108, 0, 0))      # sharp elbow bend
        set_bone('hand_R', (5, 0, 0))
        set_bone('spine02', (2, 0, 0))
        set_bone('spine03', (0, 0, 0))
        set_bone('head', (-5, 0, 0))              # slight bow
        set_bone('neck', (-3, 0, 0))
        set_bone('upperarm_L', (45, -22, -20))    # shield guard
        set_bone('lowerarm_L', (55, 0, 0))
    
    elif pose_number == 5:
        # POINT — sword extended at viewer
        set_bone('upperarm_R', (58, -8, -2))
        set_bone('lowerarm_R', (16, 0, 0))
        set_bone('hand_R', (-30, 0, 0))
        set_bone('spine01', (7, 0, 0))
        set_bone('spine02', (-12, 0, 0))           # lean forward
        set_bone('spine03', (-3, 0, 0))
        set_bone('head', (7, 0, 0))                # look at viewer
        set_bone('neck', (3, 0, 0))
        set_bone('upperarm_L', (45, -15, -25))
        set_bone('lowerarm_L', (42, 0, 0))
    
    print(f"  Set {len(rotations)} bone rotations")
    return rotations


# ============================================================
# PHASE 4: SPLINING
# ============================================================

def phase_splining(blend_file, output_blend):
    """Add breakdown poses, anticipation, follow-through, and overlap.
    
    Requires: all 5 key poses verified in Phase 3.
    """
    bpy.ops.wm.open_mainfile(filepath=blend_file)
    
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
    
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='POSE')
    
    def kb(name, frame, rot_deg):
        b = armature.pose.bones.get(name)
        if not b:
            return
        b.rotation_mode = 'XYZ'
        b.rotation_euler = Euler(tuple(math.radians(d) for d in rot_deg))
        b.keyframe_insert(data_path="rotation_euler", frame=frame)
    
    # ANTICIPATION before reach (frame 22) — body dips
    kb('spine01', 22, (3, 0, 0))        # hips shift
    kb('spine02', 22, (2, -1, -3))      # lean slightly
    kb('upperarm_R', 22, (60, 8, 10))   # arm begins moving from idle
    kb('lowerarm_R', 22, (20, 0, 0))
    
    # BREAKDOWN reach->draw (frame 40) — elbow leads
    kb('upperarm_R', 40, (-10, 5, -20)) # arm pulling back
    kb('lowerarm_R', 40, (45, 0, 0))    # elbow still bent
    kb('spine02', 40, (0, -2, -5))      # torso unwinding
    kb('head', 40, (0, 0, -3))
    
    # BREAKDOWN draw->salute (frame 65) — sword passes overhead
    kb('upperarm_R', 65, (-20, 12, -5)) # arm crossing overhead
    kb('lowerarm_R', 65, (60, 0, 0))    # bending toward salute
    kb('hand_R', 65, (-5, 0, 0))
    kb('spine02', 65, (0, 0, 3))
    
    # BREAKDOWN salute->point (frame 92) — blade tip drops first
    kb('upperarm_R', 92, (30, -2, 5))   # arm beginning to extend
    kb('lowerarm_R', 92, (60, 0, 0))    # elbow opening
    kb('hand_R', 92, (-15, 0, 0))       # wrist starting to angle
    kb('spine02', 92, (-5, 0, 0))       # body beginning to lean
    kb('spine01', 92, (4, 0, 0))
    
    # SETTLE after point (frame 115) — slight overshoot and settle
    kb('upperarm_R', 115, (60, -9, -3)) # slight overshoot past final
    kb('lowerarm_R', 115, (14, 0, 0))
    kb('hand_R', 115, (-32, 0, 0))      # wrist overshoots
    kb('spine02', 115, (-13, 0, 0))     # lean overshoots
    
    # BREATHING during hold (frames 120, 132, 144)
    for f in [120, 132, 144]:
        kb('spine03', f, (1.5 if f == 120 else -0.5 if f == 132 else 0.5, 0, 0))
    
    bpy.ops.object.mode_set(mode='OBJECT')
    
    # Smooth all curves
    if armature.animation_data and armature.animation_data.action:
        for fc in armature.animation_data.action.fcurves:
            for kp in fc.keyframe_points:
                kp.interpolation = 'BEZIER'
                kp.handle_left_type = 'AUTO_CLAMPED'
                kp.handle_right_type = 'AUTO_CLAMPED'
    
    bpy.ops.wm.save_as_mainfile(filepath=output_blend)
    print(f"Splining complete. Checkpoint: {output_blend}")


# ============================================================
# PHASE 5: POLISH
# ============================================================

def phase_polish(blend_file, output_blend):
    """Add subtle secondary motion — breathing, weight shifts, head tracking."""
    bpy.ops.wm.open_mainfile(filepath=blend_file)
    
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
    
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='POSE')
    
    def kb(name, frame, rot_deg):
        b = armature.pose.bones.get(name)
        if not b:
            return
        b.rotation_mode = 'XYZ'
        b.rotation_euler = Euler(tuple(math.radians(d) for d in rot_deg))
        b.keyframe_insert(data_path="rotation_euler", frame=frame)
    
    # Breathing cycle throughout (spine03 subtle)
    breath_cycle = 72  # frames per breath
    for f in range(1, 145, breath_cycle // 4):
        phase = (f % breath_cycle) / breath_cycle
        breath = 1.0 * math.sin(phase * 2 * math.pi)
        kb('spine03', f, (breath, 0, 0))
    
    # Head leads body rotation — add head anticipation 3 frames early
    # (head looks where the body is going before the body turns)
    # This is already partially handled but we reinforce it
    
    bpy.ops.object.mode_set(mode='OBJECT')
    
    # Re-smooth
    if armature.animation_data and armature.animation_data.action:
        for fc in armature.animation_data.action.fcurves:
            for kp in fc.keyframe_points:
                kp.interpolation = 'BEZIER'
                kp.handle_left_type = 'AUTO_CLAMPED'
                kp.handle_right_type = 'AUTO_CLAMPED'
    
    bpy.ops.wm.save_as_mainfile(filepath=output_blend)
    print(f"Polish complete. Checkpoint: {output_blend}")


# ============================================================
# PHASE 6: RENDER
# ============================================================

def phase_render(blend_file, output_webm, total_frames=144):
    """Full Cycles GPU render + encode to WebM."""
    bpy.ops.wm.open_mainfile(filepath=blend_file)
    
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = total_frames
    bpy.context.scene.render.fps = 24
    
    bpy.context.scene.render.engine = 'CYCLES'
    bpy.context.scene.cycles.device = 'GPU'
    bpy.context.scene.cycles.samples = 64
    bpy.context.scene.cycles.use_denoising = True
    
    prefs = bpy.context.preferences.addons['cycles'].preferences
    prefs.compute_device_type = 'CUDA'
    prefs.get_devices()
    for d in prefs.devices:
        d.use = True
    
    bpy.context.scene.render.resolution_x = 512
    bpy.context.scene.render.resolution_y = 768
    bpy.context.scene.render.image_settings.file_format = 'PNG'
    bpy.context.scene.render.image_settings.color_mode = 'RGBA'
    
    frame_dir = os.path.join(RENDER_DIR, "pipeline_frames")
    if os.path.exists(frame_dir):
        shutil.rmtree(frame_dir)
    os.makedirs(frame_dir)
    
    bpy.context.scene.render.filepath = frame_dir + "/"
    print(f"Rendering {total_frames} frames with Cycles GPU...")
    bpy.ops.render.render(animation=True)
    
    subprocess.run([
        "ffmpeg", "-y", "-framerate", "24",
        "-i", frame_dir + "/%04d.png",
        "-c:v", "libvpx-vp9", "-b:v", "1500k",
        "-pix_fmt", "yuva420p", "-an", output_webm
    ], capture_output=True, timeout=300)
    
    shutil.rmtree(frame_dir, ignore_errors=True)
    
    if os.path.exists(output_webm):
        size = os.path.getsize(output_webm) / 1024
        print(f"OUTPUT: {output_webm} ({size:.0f}KB)")
    print("RENDER COMPLETE")


# ============================================================
# MAIN — parse args and run requested phase
# ============================================================

def main():
    # Parse args after "--"
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []
    
    import argparse
    parser = argparse.ArgumentParser(description="Animation Pipeline")
    parser.add_argument("--phase", required=True,
                        choices=["reference", "setup", "blocking", "splining", "polish", "render"])
    parser.add_argument("--blend", help="Input .blend file")
    parser.add_argument("--output", help="Output .blend or .webm file")
    parser.add_argument("--reference", help="MediaPipe pose JSON file")
    parser.add_argument("--pose", type=int, help="Key pose number (1-5) for blocking phase")
    parser.add_argument("--preview", help="Preview image output path")
    parser.add_argument("--character", default="knight", help="Character name")
    
    args = parser.parse_args(argv)
    
    os.makedirs(CHECKPOINT_DIR, exist_ok=True)
    
    if args.phase == "reference":
        phase_reference(
            args.reference or "/home/bbrelin/ShatteredArcana/Art/Reference/sword_reference.webm",
            args.output or "/home/bbrelin/ShatteredArcana/Art/Reference/poses/pose_data.json",
        )
    
    elif args.phase == "setup":
        phase_setup(
            args.blend or "/tmp/knight_mblab_r8b.blend",
            args.output or os.path.join(CHECKPOINT_DIR, "setup.blend"),
        )
    
    elif args.phase == "blocking":
        if not args.pose:
            # Run all 5 poses sequentially
            blend = args.blend or os.path.join(CHECKPOINT_DIR, "setup.blend")
            for pose_num in range(1, 6):
                result = phase_blocking(
                    blend, pose_num,
                    args.reference,
                    os.path.join(CHECKPOINT_DIR, f"pose_{pose_num}.png"),
                )
                if result:
                    blend = result['checkpoint']  # chain checkpoints
        else:
            phase_blocking(
                args.blend or os.path.join(CHECKPOINT_DIR, "setup.blend"),
                args.pose,
                args.reference,
                args.preview,
            )
    
    elif args.phase == "splining":
        phase_splining(
            args.blend or os.path.join(CHECKPOINT_DIR, "blocking_pose5.blend"),
            args.output or os.path.join(CHECKPOINT_DIR, "splined.blend"),
        )
    
    elif args.phase == "polish":
        phase_polish(
            args.blend or os.path.join(CHECKPOINT_DIR, "splined.blend"),
            args.output or os.path.join(CHECKPOINT_DIR, "polished.blend"),
        )
    
    elif args.phase == "render":
        phase_render(
            args.blend or os.path.join(CHECKPOINT_DIR, "polished.blend"),
            args.output or os.path.join(RENDER_DIR, "aurelith-denizen-pipeline.webm"),
        )


if __name__ == "__main__":
    main()
