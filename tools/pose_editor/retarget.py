#!/usr/bin/env python3
"""
SMPL/HumanML3D → Mixamo skeleton retargeting.

Converts the 22-joint HumanML3D output from MoMask into Mixamo-compatible
bone rotations that the pose editor can consume directly.

HumanML3D joints (SMPL subset, 22 joints):
  0:  Pelvis         (root)
  1:  R_Hip          2: L_Hip          3: Spine1
  4:  R_Knee         5: L_Knee         6: Spine2
  7:  R_Ankle        8: L_Ankle        9: Spine3
  10: R_Foot         11: L_Foot        12: Neck
  13: R_Collar       14: L_Collar      15: Head
  16: R_Shoulder     17: L_Shoulder
  18: R_Elbow        19: L_Elbow
  20: R_Wrist        21: L_Wrist

Kinematic chains:
  [0,2,5,8,11]      Left leg
  [0,1,4,7,10]      Right leg
  [0,3,6,9,12,15]   Spine → Head
  [9,14,17,19,21]   Left arm
  [9,13,16,18,20]   Right arm

Mixamo skeleton (key bones):
  mixamorigHips, mixamorigSpine, mixamorigSpine1, mixamorigSpine2,
  mixamorigNeck, mixamorigHead,
  mixamorigLeftShoulder, mixamorigLeftArm, mixamorigLeftForeArm, mixamorigLeftHand,
  mixamorigRightShoulder, mixamorigRightArm, mixamorigRightForeArm, mixamorigRightHand,
  mixamorigLeftUpLeg, mixamorigLeftLeg, mixamorigLeftFoot, mixamorigLeftToeBase,
  mixamorigRightUpLeg, mixamorigRightLeg, mixamorigRightFoot, mixamorigRightToeBase
"""

import math
import numpy as np

# ============================================================
# SMPL → Mixamo joint mapping
# ============================================================

# HumanML3D joint index → Mixamo bone name
JOINT_TO_MIXAMO = {
    0:  'mixamorigHips',
    1:  'mixamorigRightUpLeg',
    2:  'mixamorigLeftUpLeg',
    3:  'mixamorigSpine',
    4:  'mixamorigRightLeg',
    5:  'mixamorigLeftLeg',
    6:  'mixamorigSpine1',
    7:  'mixamorigRightFoot',
    8:  'mixamorigLeftFoot',
    9:  'mixamorigSpine2',
    10: 'mixamorigRightToeBase',
    11: 'mixamorigLeftToeBase',
    12: 'mixamorigNeck',
    13: 'mixamorigRightShoulder',
    14: 'mixamorigLeftShoulder',
    15: 'mixamorigHead',
    16: 'mixamorigRightArm',
    17: 'mixamorigLeftArm',
    18: 'mixamorigRightForeArm',
    19: 'mixamorigLeftForeArm',
    20: 'mixamorigRightHand',
    21: 'mixamorigLeftHand',
}

# Parent indices for HumanML3D skeleton
PARENTS = [-1, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 12, 13, 14, 16, 17, 18, 19]


# ============================================================
# Quaternion math
# ============================================================

def quat_normalize(q):
    """Normalize quaternion [w,x,y,z]."""
    n = np.linalg.norm(q, axis=-1, keepdims=True)
    return q / np.maximum(n, 1e-8)


def quat_between_vectors(v1, v2):
    """Quaternion [w,x,y,z] that rotates unit vector v1 to unit vector v2."""
    v1 = v1 / np.linalg.norm(v1)
    v2 = v2 / np.linalg.norm(v2)
    cross = np.cross(v1, v2)
    dot = np.dot(v1, v2)

    if dot < -0.9999:
        # Nearly opposite — find perpendicular axis
        perp = np.array([1, 0, 0]) if abs(v1[0]) < 0.9 else np.array([0, 1, 0])
        axis = np.cross(v1, perp)
        axis = axis / np.linalg.norm(axis)
        return np.array([0.0, axis[0], axis[1], axis[2]])

    w = 1.0 + dot
    q = np.array([w, cross[0], cross[1], cross[2]])
    return q / np.linalg.norm(q)


def quat_multiply(q1, q2):
    """Multiply two quaternions [w,x,y,z]."""
    w1, x1, y1, z1 = q1
    w2, x2, y2, z2 = q2
    return np.array([
        w1*w2 - x1*x2 - y1*y2 - z1*z2,
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
    ])


def quat_inverse(q):
    """Inverse of a unit quaternion [w,x,y,z]."""
    return np.array([q[0], -q[1], -q[2], -q[3]])


def quat_to_euler_xyz(q):
    """Convert quaternion [w,x,y,z] to Euler XYZ (degrees)."""
    w, x, y, z = q
    # Roll (X)
    sinr = 2.0 * (w * x + y * z)
    cosr = 1.0 - 2.0 * (x * x + y * y)
    rx = math.atan2(sinr, cosr)
    # Pitch (Y)
    sinp = 2.0 * (w * y - z * x)
    sinp = max(-1.0, min(1.0, sinp))
    ry = math.asin(sinp)
    # Yaw (Z)
    siny = 2.0 * (w * z + x * y)
    cosy = 1.0 - 2.0 * (y * y + z * z)
    rz = math.atan2(siny, cosy)
    return (math.degrees(rx), math.degrees(ry), math.degrees(rz))


# ============================================================
# Joint positions → Bone rotations
# ============================================================

# Reference bone directions in T-pose (unit vectors from parent to child)
# Computed from a standard Mixamo T-pose skeleton.
_TPOSE_DIRS = None

def _compute_tpose_dirs(tpose_positions):
    """Compute reference bone directions from T-pose joint positions."""
    n_joints = len(PARENTS)
    dirs = np.zeros((n_joints, 3))
    for i in range(1, n_joints):
        p = PARENTS[i]
        d = tpose_positions[i] - tpose_positions[p]
        norm = np.linalg.norm(d)
        if norm > 1e-6:
            dirs[i] = d / norm
        else:
            dirs[i] = np.array([0, 1, 0])
    return dirs


def positions_to_rotations(joint_positions, tpose=None):
    """Convert per-frame joint positions (N, 22, 3) to per-frame bone rotations.

    Args:
        joint_positions: numpy array (num_frames, 22, 3) — world-space positions
        tpose: optional (22, 3) T-pose positions. If None, uses first frame.

    Returns:
        dict of { mixamo_bone_name: [(rx, ry, rz), ...] } with Euler XYZ in degrees,
        one tuple per frame.
    """
    global _TPOSE_DIRS

    n_frames, n_joints, _ = joint_positions.shape
    assert n_joints == 22, f"Expected 22 joints, got {n_joints}"

    if tpose is None:
        tpose = joint_positions[0]

    if _TPOSE_DIRS is None:
        _TPOSE_DIRS = _compute_tpose_dirs(tpose)

    result = {name: [] for name in JOINT_TO_MIXAMO.values()}

    for f in range(n_frames):
        positions = joint_positions[f]

        # Compute global rotations for each joint
        global_rots = [np.array([1, 0, 0, 0])] * n_joints  # identity quaternions

        for i in range(1, n_joints):
            p = PARENTS[i]
            # Current bone direction
            bone_dir = positions[i] - positions[p]
            bone_len = np.linalg.norm(bone_dir)
            if bone_len < 1e-6:
                continue
            bone_dir = bone_dir / bone_len

            # Rotation from T-pose direction to current direction
            tpose_dir = _TPOSE_DIRS[i]
            q = quat_between_vectors(tpose_dir, bone_dir)
            global_rots[i] = q

        # Convert global rotations to local (relative to parent)
        for i in range(n_joints):
            if i not in JOINT_TO_MIXAMO:
                continue
            bone_name = JOINT_TO_MIXAMO[i]

            if PARENTS[i] < 0:
                # Root — use global rotation
                local_q = global_rots[i]
            else:
                # Local = inverse(parent_global) * child_global
                parent_q = global_rots[PARENTS[i]]
                local_q = quat_multiply(quat_inverse(parent_q), global_rots[i])

            local_q = quat_normalize(local_q)
            euler = quat_to_euler_xyz(local_q)
            result[bone_name].append(euler)

    return result


def rotations_to_pose_editor_format(rotations):
    """Convert retargeted rotations to the format expected by the pose editor.

    Input:  { bone_name: [(rx, ry, rz), ...] }
    Output: [ { bone_name: (rx, ry, rz), ... }, ... ]  — one dict per frame
    """
    if not rotations:
        return []

    n_frames = len(next(iter(rotations.values())))
    frames = []
    for f in range(n_frames):
        frame_data = {}
        for bone_name, rots in rotations.items():
            if f < len(rots):
                frame_data[bone_name] = rots[f]
        frames.append(frame_data)
    return frames


# ============================================================
# Convenience
# ============================================================

def retarget(joint_positions, fps_in=20, fps_out=24, total_frames=360):
    """Full retarget pipeline: positions → Mixamo rotations, resampled.

    Args:
        joint_positions: (N, 22, 3) from MoMask
        fps_in: MoMask output fps (typically 20)
        fps_out: Target fps (24 for our pipeline)
        total_frames: Desired output frame count (360 = 15s at 24fps)

    Returns:
        List of dicts, one per frame: [{ bone: (rx, ry, rz), ... }, ...]
    """
    n_in = joint_positions.shape[0]

    # Resample to target frame count via linear interpolation
    if n_in != total_frames:
        from scipy.interpolate import interp1d
        t_in = np.linspace(0, 1, n_in)
        t_out = np.linspace(0, 1, total_frames)
        # Interpolate each joint's x,y,z
        resampled = np.zeros((total_frames, 22, 3))
        for j in range(22):
            for axis in range(3):
                f = interp1d(t_in, joint_positions[:, j, axis], kind='linear')
                resampled[:, j, axis] = f(t_out)
        joint_positions = resampled

    rotations = positions_to_rotations(joint_positions)
    return rotations_to_pose_editor_format(rotations)


def blend_beat_sequences(beat_frame_lists, blend_frames=6):
    """Crossfade-blend consecutive beat sequences at their boundaries.

    Takes a list of frame lists (one per beat), each being a list of
    { bone_name: (rx, ry, rz) } dicts. At each junction, the last
    `blend_frames` of beat K and the first `blend_frames` of beat K+1
    are slerp-blended using a smoothstep curve.

    Returns a single concatenated frame list with smooth transitions.
    """
    if not beat_frame_lists:
        return []
    if len(beat_frame_lists) == 1:
        return beat_frame_lists[0]

    def smoothstep(t):
        return t * t * (3 - 2 * t)

    def lerp_rotation(a, b, t):
        """Linear interpolation between two (rx, ry, rz) tuples."""
        return (
            a[0] + (b[0] - a[0]) * t,
            a[1] + (b[1] - a[1]) * t,
            a[2] + (b[2] - a[2]) * t,
        )

    result = list(beat_frame_lists[0])  # copy first beat

    for beat_idx in range(1, len(beat_frame_lists)):
        next_beat = beat_frame_lists[beat_idx]
        n_blend = min(blend_frames, len(result), len(next_beat))

        if n_blend > 0:
            # Blend the overlap zone
            for i in range(n_blend):
                t = smoothstep((i + 1) / (n_blend + 1))
                tail_idx = len(result) - n_blend + i
                head_frame = next_beat[i]
                tail_frame = result[tail_idx]

                blended = {}
                all_bones = set(tail_frame.keys()) | set(head_frame.keys())
                for bone in all_bones:
                    if bone in tail_frame and bone in head_frame:
                        blended[bone] = lerp_rotation(tail_frame[bone], head_frame[bone], t)
                    elif bone in head_frame:
                        blended[bone] = head_frame[bone]
                    else:
                        blended[bone] = tail_frame[bone]

                result[tail_idx] = blended

            # Append the rest of next_beat (after the blended overlap)
            result.extend(next_beat[n_blend:])
        else:
            result.extend(next_beat)

    return result


if __name__ == '__main__':
    # Quick self-test with synthetic T-pose
    print("Retarget self-test...")
    # Create a simple 10-frame motion: T-pose with slight arm raise
    tpose = np.zeros((22, 3))
    # Set up a basic skeleton
    tpose[0] = [0, 1, 0]     # Pelvis
    tpose[3] = [0, 1.2, 0]   # Spine1
    tpose[6] = [0, 1.4, 0]   # Spine2
    tpose[9] = [0, 1.6, 0]   # Spine3
    tpose[12] = [0, 1.7, 0]  # Neck
    tpose[15] = [0, 1.85, 0] # Head
    tpose[13] = [0.15, 1.6, 0]  # R_Collar
    tpose[14] = [-0.15, 1.6, 0] # L_Collar
    tpose[16] = [0.5, 1.6, 0]   # R_Shoulder
    tpose[17] = [-0.5, 1.6, 0]  # L_Shoulder
    tpose[18] = [0.8, 1.6, 0]   # R_Elbow
    tpose[19] = [-0.8, 1.6, 0]  # L_Elbow
    tpose[20] = [1.1, 1.6, 0]   # R_Wrist
    tpose[21] = [-1.1, 1.6, 0]  # L_Wrist
    tpose[1] = [0.1, 0.9, 0]    # R_Hip
    tpose[2] = [-0.1, 0.9, 0]   # L_Hip
    tpose[4] = [0.1, 0.5, 0]    # R_Knee
    tpose[5] = [-0.1, 0.5, 0]   # L_Knee
    tpose[7] = [0.1, 0.1, 0]    # R_Ankle
    tpose[8] = [-0.1, 0.1, 0]   # L_Ankle
    tpose[10] = [0.1, 0, 0.1]   # R_Foot
    tpose[11] = [-0.1, 0, 0.1]  # L_Foot

    # 10 frames: gradually raise right arm
    motion = np.tile(tpose, (10, 1, 1))
    for f in range(10):
        angle = f * 0.1 * math.pi  # 0 to ~0.9 radians
        motion[f, 16] = [0.5 * math.cos(angle), 1.6 + 0.5 * math.sin(angle), 0]
        motion[f, 18] = [0.8 * math.cos(angle), 1.6 + 0.8 * math.sin(angle), 0]
        motion[f, 20] = [1.1 * math.cos(angle), 1.6 + 1.1 * math.sin(angle), 0]

    frames = retarget(motion, fps_in=20, fps_out=24, total_frames=24)
    print(f"  Output: {len(frames)} frames, {len(frames[0])} bones per frame")
    print(f"  Sample RightArm frame 0: {frames[0].get('mixamorigRightArm', 'missing')}")
    print(f"  Sample RightArm frame 12: {frames[12].get('mixamorigRightArm', 'missing')}")
    print("  PASS" if len(frames) == 24 else "  FAIL")
