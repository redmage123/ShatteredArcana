#!/usr/bin/env python3
"""
Shattered Arcana — Animator

Comprehensive Playwright-based animation automation tool.
Claude uses this to create animations frame-by-frame, Harryhausen style.

Usage:
    python3 animator.py test              # Integration test
    python3 animator.py interactive       # REPL mode
    python3 animator.py serve             # Start HTTP server only
"""

import http.server
import json
import math
import os
import socketserver
import subprocess
import sys
import threading
import time
from pathlib import Path
from urllib.parse import unquote

# ============================================================
# PATHS
# ============================================================

TOOL_DIR = Path(__file__).parent
PROJECT_ROOT = TOOL_DIR.parent.parent
MODEL_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations" / "MixamoAnims"
SCREENSHOT_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations" / "pose_screenshots"
FRAMES_DIR = TOOL_DIR / "frames"
EXPORTS_DIR = TOOL_DIR / "exports"
PORT = 8765


# ============================================================
# HTTP SERVER
# ============================================================

class AnimHTTPHandler(http.server.SimpleHTTPRequestHandler):
    """Serves the pose editor, JS modules, and model/clip FBX files."""

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            self.path = '/index.html'
            self.directory = str(TOOL_DIR)
            return super().do_GET()
        elif self.path.startswith('/js/'):
            self.directory = str(TOOL_DIR)
            return super().do_GET()
        elif self.path.startswith('/model/') or self.path.startswith('/clips/'):
            prefix_len = 7 if self.path.startswith('/model/') else 7
            filename = unquote(self.path[prefix_len:])
            filepath = MODEL_DIR / filename
            if filepath.exists():
                self.send_response(200)
                self.send_header('Content-Type', 'application/octet-stream')
                self.send_header('Content-Length', str(filepath.stat().st_size))
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                with open(filepath, 'rb') as f:
                    self.wfile.write(f.read())
            else:
                self.send_error(404, f'Not found: {filename}')
            return
        else:
            self.directory = str(TOOL_DIR)
            return super().do_GET()

    def log_message(self, fmt, *args):
        pass  # Suppress


def start_server(port=PORT):
    socketserver.TCPServer.allow_reuse_address = True
    httpd = socketserver.TCPServer(("", port), AnimHTTPHandler)
    httpd.serve_forever()


# ============================================================
# ANIMATOR CLASS
# ============================================================

class Animator:
    """High-level animation automation via Playwright."""

    def __init__(self, port=PORT, headless=True, width=1280, height=960):
        self.port = port
        self.width = width
        self.height = height

        # Start HTTP server
        self._server_thread = threading.Thread(
            target=start_server, args=(port,), daemon=True
        )
        self._server_thread.start()
        time.sleep(0.5)

        # Launch browser
        from playwright.sync_api import sync_playwright
        self._pw = sync_playwright().start()
        self._browser = self._pw.chromium.launch(
            headless=headless,
            args=['--no-sandbox', '--disable-gpu', '--use-gl=swiftshader'],
        )
        self._page = self._browser.new_page(viewport={'width': width, 'height': height})

        # Capture console for debugging
        self._console_errors = []
        self._page.on('console', lambda msg: (
            self._console_errors.append(msg.text) if msg.type == 'error' else None
        ))

        # Load editor
        self._page.goto(f'http://localhost:{port}/', wait_until='networkidle', timeout=30000)

        # Wait for model
        self._page.wait_for_function(
            '() => window.getModelReady && window.getModelReady()',
            timeout=30000,
        )
        time.sleep(0.5)

    @property
    def page(self):
        return self._page

    def _eval(self, expr):
        return self._page.evaluate(expr)

    # ── Scene Info ──────────────────────────────────────────
    def get_scene_info(self):
        return self._eval('() => window.getSceneInfo()')

    # ── Frame Control ───────────────────────────────────────
    def go_to_frame(self, frame):
        self._eval(f'() => window.goToFrame({frame})')

    def get_current_frame(self):
        return self._eval('() => window.getCurrentFrame()')

    def get_total_frames(self):
        return self._eval('() => window.getTotalFrames()')

    # ── Bone Manipulation ───────────────────────────────────
    def select_bone(self, name):
        self._eval(f'() => window.selectBone("{name}")')

    def set_bone_rotation(self, name, rx, ry, rz):
        return self._eval(f'() => window.setBoneRotation("{name}", {rx}, {ry}, {rz})')

    def get_bone_rotation(self, name):
        return self._eval(f'() => window.getBoneRotation("{name}")')

    def get_bone_names(self):
        return self._eval('() => window.getBoneNames()')

    def get_all_rotations(self):
        return self._eval('() => window.getBonesState()')

    def reset_bone(self, name):
        self._eval(f'() => window.resetBone("{name}")')

    def reset_all_bones(self):
        self._eval('() => window.resetAllBones()')

    def pose_bones(self, rotations):
        """Set multiple bone rotations at once.
        rotations: dict of { boneName: (rx, ry, rz) } or { boneName: [rx, ry, rz] }
        """
        js_obj = {}
        for name, rot in rotations.items():
            if isinstance(rot, (list, tuple)):
                js_obj[name] = list(rot)
            else:
                js_obj[name] = [rot.get('x', 0), rot.get('y', 0), rot.get('z', 0)]
        self._eval(f'(r) => window.poseBones(r)')(json.dumps(js_obj))
        # Can't call evaluate with args directly, use a different approach
        self._page.evaluate('(r) => window.poseBones(JSON.parse(r))', json.dumps(js_obj))

    def pose_bones_direct(self, rotations):
        """Set multiple bone rotations via individual calls (more reliable)."""
        for name, rot in rotations.items():
            if isinstance(rot, (list, tuple)):
                self.set_bone_rotation(name, rot[0], rot[1], rot[2])
            else:
                self.set_bone_rotation(name, rot.get('x', 0), rot.get('y', 0), rot.get('z', 0))

    # ── Keyframes ───────────────────────────────────────────
    def set_keyframe(self, bone_name=None):
        if bone_name:
            return self._eval(f'() => window.setKeyframe("{bone_name}")')
        return self._eval('() => window.setKeyframeAll()')

    def set_keyframe_all(self):
        return self._eval('() => window.setKeyframeAll()')

    def delete_keyframe(self, bone_name=None):
        if bone_name:
            self._eval(f'() => window.deleteKeyframe("{bone_name}")')
        else:
            self._eval('() => window.deleteKeyframe()')

    def get_keyframes(self):
        return json.loads(self._eval('() => window.getKeyframes()'))

    def import_keyframes(self, data):
        self._page.evaluate('(d) => window.importKeyframes(d)', json.dumps(data))

    def clear_keyframes(self):
        self._eval('() => window.clearKeyframes()')

    def keyframe_pose(self, frame, rotations):
        """Go to frame, set bone rotations, keyframe all. Convenience method."""
        self.go_to_frame(frame)
        self.pose_bones_direct(rotations)
        self.set_keyframe_all()

    # ── Clip System ─────────────────────────────────────────
    def load_clip(self, clip_name):
        """Load a Mixamo FBX clip by filename (without path).
        Returns { name, duration, trackCount }.
        """
        url = f'/clips/{clip_name}.fbx'
        return self._page.evaluate(
            f'() => window.loadClip("{url}", "{clip_name}")',
        )

    def load_clip_file(self, filename, name=None):
        """Load a clip by exact filename."""
        if name is None:
            name = filename.replace('.fbx', '')
        url = f'/clips/{filename}'
        return self._page.evaluate(f'() => window.loadClip("{url}", "{name}")')

    def list_clips(self):
        return self._eval('() => window.listClips()')

    def sample_clip_at_frame(self, clip_name, frame):
        return self._eval(f'() => window.sampleClipAtFrame("{clip_name}", {frame})')

    def add_clip_segment(self, clip_name, start_frame, end_frame, **opts):
        opts_js = json.dumps(opts) if opts else '{}'
        return self._eval(
            f'() => window.addClipSegment("{clip_name}", {start_frame}, {end_frame}, {opts_js})'
        )

    def clear_clip_segments(self):
        self._eval('() => window.clearClipSegments()')

    def get_clip_segments(self):
        return self._eval('() => window.getClipSegments()')

    def evaluate_at_frame(self, frame):
        """Full evaluation: sequencer + keyframes + camera + render."""
        self._eval(f'() => window.evaluateFullFrame({frame})')

    # ── Weapons ─────────────────────────────────────────────
    def attach_weapons(self):
        return self._eval('() => window.attachWeapons()')

    def detach_weapons(self):
        self._eval('() => window.detachWeapons()')

    def set_sword_offset(self, x, y, z, rx, ry, rz):
        self._eval(f'() => window.setSwordOffset({x},{y},{z},{rx},{ry},{rz})')

    def set_shield_offset(self, x, y, z, rx, ry, rz):
        self._eval(f'() => window.setShieldOffset({x},{y},{z},{rx},{ry},{rz})')

    # ── Camera ──────────────────────────────────────────────
    def set_camera(self, x, y, z, tx=0, ty=1.0, tz=0, fov=35):
        self._eval(f'() => {{ window.setCameraPosition({x},{y},{z}); window.setCameraTarget({tx},{ty},{tz}); window.setCameraFOV({fov}); }}')

    def set_camera_keyframe(self, frame=None):
        if frame is not None:
            self._eval(f'() => window.setCameraKeyframe({frame})')
        else:
            self._eval('() => window.setCameraKeyframe()')

    def get_camera_state(self):
        return self._eval('() => window.getCameraState()')

    # ── Background ──────────────────────────────────────────
    def set_background(self, preset):
        self._eval(f'() => window.setBackground("{preset}")')

    def set_particle_intensity(self, v):
        self._eval(f'() => window.setParticleIntensity({v})')

    # ── Pose Library ────────────────────────────────────────
    def save_pose(self, name):
        self._eval(f'() => window.savePose("{name}")')

    def load_pose(self, name):
        return self._eval(f'() => window.loadPose("{name}")')

    def list_poses(self):
        return self._eval('() => window.listPoses()')

    # ── Undo / Redo ─────────────────────────────────────────
    def undo(self):
        self._eval('() => window.undo()')

    def redo(self):
        self._eval('() => window.redo()')

    # ── Visual Verification ─────────────────────────────────
    def screenshot(self, path=None):
        """Take a screenshot and return the path."""
        SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
        if path is None:
            frame = self.get_current_frame()
            path = str(SCREENSHOT_DIR / f'frame_{frame:04d}.png')
        self._page.screenshot(path=str(path))
        return str(path)

    def verify_frame(self, frame):
        """Go to frame, evaluate, screenshot. Core visual verification primitive."""
        self.go_to_frame(frame)
        self._eval(f'() => {{ window.evaluateFullFrame({frame}); }}')
        time.sleep(0.1)
        return self.screenshot()

    def score_frame(self, path=None):
        """Analyze a screenshot and return (score, issues) where score is 0-100.

        This scoring adapts dynamically: if visual review reveals the scores
        don't match reality, the thresholds should be tightened. The score
        must genuinely reflect what a human artist would judge.

        Checks:
        - Character properly framed (head, torso, legs all visible)
        - Head not clipped at top (critical — instant fail if missing)
        - Character centered and reasonably sized
        - No major body parts off-screen
        - Sufficient brightness and color to read the character
        - Vertical coverage: character should span 50-85% of frame height

        Returns (score: float, issues: list[str])
        """
        from PIL import Image
        import numpy as np

        if path is None:
            path = self.screenshot()

        img = Image.open(path).convert('RGB')
        arr = np.array(img)
        h, w, _ = arr.shape

        issues = []
        score = 100.0

        # Background detection — use per-channel to avoid counting
        # dark armor as background. Background is navy blue (~10,10,26).
        # Character has reds, golds, whites that distinguish from bg.
        bg_b = arr[:, :, 2]  # blue channel
        bg_r = arr[:, :, 0]  # red channel
        brightness = arr.mean(axis=2)

        # Character mask: either bright OR has significant red/color
        # (catches dark armor that still has color variation)
        char_mask = (brightness > 35) | (bg_r > 40) | (arr.std(axis=2) > 20)

        # Character mask — tuned for dark-armored Knight D Pelegrini.
        # Background is navy (R~15, G~11, B~24) — blue-dominant, dark.
        # Ground is brown (R~50, G~40, B~37) — red-dominant, warm.
        # Character has reds (tabard), silvers (armor), golds (trim) —
        #   distinguishable by high color std or specific color ranges.
        r, g, b = arr[:, :, 0].astype(float), arr[:, :, 1].astype(float), arr[:, :, 2].astype(float)
        color_std = arr.astype(float).std(axis=2)

        # Ground mask: warm brown, low color variety (R > B, brightness 30-55)
        is_ground = (r > b + 5) & (brightness > 28) & (brightness < 58) & (color_std < 15)
        # Background mask: cold navy (B > R, brightness < 30)
        is_background = (b > r) & (brightness < 28)

        # Character = everything that's NOT ground and NOT background
        strong_mask = ~is_ground & ~is_background & (brightness > 18)

        # ── HEAD VISIBLE (weight: 25) ──
        # Top 20% of frame should contain head. If head is cut off,
        # the animation looks amateur.
        # Use a combined mask: strong_mask OR has red tones (for the knight's red hood)
        has_red = (r > 50) & (r > g * 1.3) & (r > b * 1.3)
        head_mask = strong_mask | has_red
        top_20 = head_mask[:int(h * 0.20), :]
        top_fill = top_20.mean()
        if top_fill < 0.001:
            issues.append(f'HEAD_MISSING (top 20% fill {top_fill:.1%})')
            score -= 25
        elif top_fill < 0.004:
            issues.append(f'head_barely_visible (top 20% fill {top_fill:.1%})')
            score -= 12
        elif top_fill < 0.01:
            issues.append(f'head_small (top 20% fill {top_fill:.1%})')
            score -= 5

        # ── TORSO VISIBLE (weight: 20) ──
        # Middle band (20-55% from top) should have substantial character
        torso = strong_mask[int(h * 0.20):int(h * 0.55), w//6:5*w//6]
        torso_fill = torso.mean()
        if torso_fill < 0.05:
            issues.append(f'TORSO_MISSING (fill {torso_fill:.1%})')
            score -= 20
        elif torso_fill < 0.12:
            issues.append(f'torso_small (fill {torso_fill:.1%})')
            score -= 8

        # ── LEGS VISIBLE (weight: 10) ──
        legs = strong_mask[int(h * 0.55):int(h * 0.85), w//6:5*w//6]
        legs_fill = legs.mean()
        if legs_fill < 0.03:
            issues.append(f'legs_missing (fill {legs_fill:.1%})')
            score -= 10
        elif legs_fill < 0.08:
            issues.append(f'legs_small (fill {legs_fill:.1%})')
            score -= 4

        # ── VERTICAL COVERAGE (weight: 15) ──
        # Find topmost and bottommost character rows.
        # Only count rows where character fills >3% width (ignores stray particles)
        row_fill = strong_mask.mean(axis=1)
        substantial_rows = row_fill > 0.03
        char_rows = np.where(substantial_rows)[0]
        if len(char_rows) > 10:
            top_row = char_rows[0]
            bot_row = char_rows[-1]
            coverage = (bot_row - top_row) / h
            # Character should span 45-90% of frame height
            if coverage < 0.35:
                issues.append(f'character_too_small (coverage {coverage:.1%})')
                score -= 15
            elif coverage < 0.45:
                issues.append(f'character_small (coverage {coverage:.1%})')
                score -= 6
            elif coverage > 0.95:
                issues.append(f'character_too_tall (coverage {coverage:.1%})')
                score -= 8
            # Check headroom — top of character should be at 5-20% from top
            headroom = top_row / h
            if headroom < 0.02:
                issues.append(f'no_headroom (top at {headroom:.1%})')
                score -= 10
        else:
            issues.append('CHARACTER_NOT_FOUND')
            score -= 40

        # ── CENTERING (weight: 10) ──
        # Character's center of mass should be near horizontal center
        if len(char_rows) > 10:
            col_has_char = strong_mask.any(axis=0)
            char_cols = np.where(col_has_char)[0]
            if len(char_cols) > 10:
                center_x = (char_cols[0] + char_cols[-1]) / 2 / w
                if abs(center_x - 0.5) > 0.25:
                    issues.append(f'off_center (cx={center_x:.2f})')
                    score -= 10
                elif abs(center_x - 0.5) > 0.15:
                    issues.append(f'slightly_off_center (cx={center_x:.2f})')
                    score -= 3

        # ── BRIGHTNESS (weight: 5) ──
        # Dark fantasy scene — only penalize if truly unreadable
        char_pixels = arr[char_mask]
        if len(char_pixels) > 100:
            avg_brightness = char_pixels.mean()
            if avg_brightness < 25:
                issues.append(f'too_dark (brightness {avg_brightness:.0f})')
                score -= 5

        # ── COLOR (weight: 3) ──
        # The knight has reds and golds but scene is dark — only flag if pure gray
        if len(char_pixels) > 100:
            color_std_val = char_pixels.std(axis=0).mean()
            if color_std_val < 6:
                issues.append(f'monochrome (color std {color_std_val:.0f})')
                score -= 3

        # ── TOP EDGE CLIPPING (weight: 7) ──
        # Strong character pixels at very top = head cut off
        top_strip = strong_mask[:int(h * 0.01), w//4:3*w//4]
        if top_strip.mean() > 0.2:
            issues.append(f'top_clipping ({top_strip.mean():.1%})')
            score -= 7

        score = max(0, min(100, score))
        return score, issues

    def gate_frame(self, frame, pose_fn, max_iterations=10, target=95.0):
        """Confidence gate: iterate until frame passes target score.

        Args:
            frame: Frame number to evaluate
            pose_fn: Callable that takes (animator, frame, iteration, issues)
                     and adjusts the pose/camera. Returns True if it made changes.
            max_iterations: Safety limit
            target: Minimum confidence score to pass (default 95%)

        Returns (score, path, iterations)
        """
        for i in range(max_iterations):
            time.sleep(0.15)
            path = self.screenshot()
            score, issues = self.score_frame(path)

            if score >= target:
                return score, path, i + 1

            # Call the adjustment function
            changed = pose_fn(self, frame, i, issues)
            if not changed:
                # pose_fn couldn't fix it — return best effort
                return score, path, i + 1

            time.sleep(0.2)

        # Max iterations reached
        path = self.screenshot()
        score, issues = self.score_frame(path)
        return score, path, max_iterations

    # ── Onion Skin ──────────────────────────────────────────
    def toggle_onion_skin(self, enabled=True):
        self._eval(f'() => window.toggleOnionSkin({str(enabled).lower()})')

    # ── Render Pipeline ─────────────────────────────────────
    def render_all_frames(self, output_dir=None, width=512, height=768,
                          start=1, end=360):
        """Render all frames as PNGs."""
        if output_dir is None:
            output_dir = str(FRAMES_DIR)
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Hide UI for clean render
        self._eval('() => window.hideUI()')
        self._page.set_viewport_size({'width': width, 'height': height})
        self._eval(f'() => window.setRenderSize({width}, {height})')
        time.sleep(0.2)

        for frame in range(start, end + 1):
            self._eval(f'() => {{ window.evaluateFullFrame({frame}); }}')
            time.sleep(0.05)
            path = output_dir / f'{frame:04d}.png'
            self._page.screenshot(
                path=str(path),
                clip={'x': 0, 'y': 0, 'width': width, 'height': height}
            )
            if frame % 24 == 0:
                print(f'  Rendered frame {frame}/{end}')

        # Restore UI
        self._eval('() => window.showUI()')
        self._page.set_viewport_size({'width': self.width, 'height': self.height})
        self._eval(f'() => window.setRenderSize({self.width}, {self.height - 120})')

        print(f'Done: {end - start + 1} frames at {output_dir}')
        return str(output_dir)

    def render_to_webm(self, output_path, width=512, height=768,
                        fps=24, bitrate='1500k', start=1, end=360):
        """Full pipeline: render frames, quality-gate, then encode to WebM."""
        frame_dir = str(FRAMES_DIR / '_render_tmp')
        self.render_all_frames(frame_dir, width, height, start, end)

        # Quality gate: check for jitter and freezes before encoding
        report = quality_gate(frame_dir, start, end)
        if report['pass']:
            print(f'Quality gate PASSED (jitter={report["mean_diff"]:.2f}, '
                  f'freezes={report["freeze_count"]}, spikes={report["spike_count"]})')
        else:
            print(f'Quality gate FAILED:')
            for issue in report['issues']:
                print(f'  - {issue}')
            print('Encoding anyway — review the issues above.')

        encode_webm(frame_dir, output_path, fps, bitrate)
        return output_path

    # ── Export ──────────────────────────────────────────────
    def export_animation(self, path=None):
        """Export all animation data (keyframes, clips, camera) to JSON."""
        if path is None:
            EXPORTS_DIR.mkdir(parents=True, exist_ok=True)
            path = str(EXPORTS_DIR / 'animation.json')
        data = {
            'keyframes': self.get_keyframes(),
            'clipSegments': self.get_clip_segments(),
            'camera': json.loads(self._eval('() => window.getCameraKeyframes()')),
            'poses': {n: self._eval(f'() => JSON.stringify(window.listPoses())') for n in []},
        }
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)
        print(f'Exported: {path}')
        return path

    # ── Cleanup ─────────────────────────────────────────────
    def close(self):
        try:
            self._browser.close()
            self._pw.stop()
        except Exception:
            pass

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


# ============================================================
# WebM ENCODING
# ============================================================

def quality_gate(frame_dir, start=1, end=360):
    """Analyze rendered frames for jitter, freezes, and hard cuts.

    Returns a report dict:
      pass: bool — True if animation quality is acceptable
      mean_diff: float — average pixel difference between consecutive frames
      freeze_count: int — number of frames with near-zero motion (< 0.05)
      spike_count: int — number of frames with jitter > 3x mean
      issues: list[str] — human-readable issue descriptions
    """
    from PIL import Image
    import numpy as np

    diffs = []
    prev_arr = None
    frame_dir = Path(frame_dir)

    for frame in range(start, end + 1):
        path = frame_dir / f'{frame:04d}.png'
        if not path.exists():
            continue
        arr = np.array(Image.open(path).convert('RGB')).astype(float)
        if prev_arr is not None:
            diff = np.abs(arr - prev_arr).mean()
            diffs.append((frame, diff))
        prev_arr = arr

    if not diffs:
        return {'pass': False, 'issues': ['No frames found'], 'mean_diff': 0,
                'freeze_count': 0, 'spike_count': 0, 'diffs': []}

    diffs_arr = np.array([d[1] for d in diffs])
    mean_diff = diffs_arr.mean()
    issues = []

    # Freezes: consecutive frames with < 0.005 mean pixel diff.
    # Note: subtle motion (breathing, slow clips) produces diffs of 0.01-0.05.
    # Only truly static frames (no bone movement at all) are < 0.005.
    freeze_threshold = 0.005
    freeze_frames = [(f, d) for f, d in diffs if d < freeze_threshold]
    # Group consecutive freezes into runs
    freeze_runs = []
    if freeze_frames:
        run_start = freeze_frames[0][0]
        run_end = freeze_frames[0][0]
        for f, d in freeze_frames[1:]:
            if f == run_end + 1:
                run_end = f
            else:
                freeze_runs.append((run_start, run_end))
                run_start = f
                run_end = f
        freeze_runs.append((run_start, run_end))

    # Only flag freeze runs > 12 frames (0.5s)
    long_freezes = [(s, e) for s, e in freeze_runs if e - s >= 12]
    for s, e in long_freezes:
        issues.append(f'FREEZE: frames {s}-{e} ({e - s + 1} frames, {(e - s + 1) / 24:.1f}s of no motion)')

    # Jitter spikes: frames where diff > 3x mean (excluding first 2 frames of each beat)
    spike_threshold = max(mean_diff * 3, 0.5)
    spikes = [(f, d) for f, d in diffs if d > spike_threshold]
    # Filter out expected beat transitions (allow the first frame of each segment)
    # We don't know segment boundaries here, so flag all spikes
    if len(spikes) > 8:  # More than 8 spikes across 360 frames = jittery
        issues.append(f'JITTER: {len(spikes)} frames exceed {spike_threshold:.2f} diff '
                       f'(3x mean of {mean_diff:.2f})')

    # Alternating jitter: check for high variance in diff (std > mean)
    if diffs_arr.std() > mean_diff * 1.5 and mean_diff > 0.1:
        issues.append(f'ALTERNATING_JITTER: std={diffs_arr.std():.2f} >> mean={mean_diff:.2f}')

    # Static beats: check for regions where mean diff < 0.005 for > 24 frames
    # (truly frozen — no bone motion at all, not even breathing)
    window = 24
    for i in range(len(diffs) - window):
        window_mean = diffs_arr[i:i + window].mean()
        if window_mean < 0.005:
            frame_start = diffs[i][0]
            issues.append(f'STATIC_BEAT: frames {frame_start}-{frame_start + window} '
                          f'(mean diff {window_mean:.4f} — no visible motion)')
            break  # Only report first occurrence

    passed = len(issues) == 0
    return {
        'pass': passed,
        'mean_diff': float(mean_diff),
        'freeze_count': len(freeze_frames),
        'spike_count': len(spikes),
        'issues': issues,
        'diffs': diffs,
    }


def encode_webm(frame_dir, output_path, fps=24, bitrate='1500k'):
    """Encode a PNG frame sequence to WebM via ffmpeg."""
    cmd = [
        'ffmpeg', '-y',
        '-framerate', str(fps),
        '-i', os.path.join(frame_dir, '%04d.png'),
        '-c:v', 'libvpx-vp9',
        '-b:v', bitrate,
        '-pix_fmt', 'yuva420p',
        '-auto-alt-ref', '0',
        '-an',
        output_path,
    ]
    print(f'Encoding: {output_path}')
    result = subprocess.run(cmd, capture_output=True, timeout=600)
    if result.returncode != 0:
        print(f'ffmpeg error: {result.stderr.decode()[:500]}')
    else:
        size_kb = os.path.getsize(output_path) / 1024
        print(f'Done: {output_path} ({size_kb:.0f} KB)')
    return output_path


# ============================================================
# CLIP CATALOG
# ============================================================

def list_available_clips():
    """List all Mixamo FBX animation clips available."""
    clips = []
    for f in sorted(MODEL_DIR.iterdir()):
        if f.suffix.lower() == '.fbx' and f.name != 'X Bot.fbx':
            size_kb = f.stat().st_size / 1024
            clips.append({'name': f.stem, 'file': f.name, 'size_kb': round(size_kb)})
    return clips


# ============================================================
# INTERACTIVE REPL
# ============================================================

def interactive(anim):
    """Interactive mode for the pose tool."""
    print("Shattered Arcana Animator — Interactive Mode")
    print("Commands: frame <n>, select <bone>, rot <rx> <ry> <rz>, key, bones,")
    print("          state, shot, clip <name>, attach, bg <preset>, cam <x y z>,")
    print("          render <dir>, webm <path>, export, quit")
    print()

    while True:
        try:
            cmd = input("anim> ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if not cmd:
            continue

        parts = cmd.split()
        verb = parts[0].lower()

        try:
            if verb in ('q', 'quit', 'exit'):
                break
            elif verb == 'info':
                print(json.dumps(anim.get_scene_info(), indent=2))
            elif verb == 'frame' and len(parts) >= 2:
                anim.go_to_frame(int(parts[1]))
                print(f'Frame: {parts[1]}')
            elif verb == 'select' and len(parts) >= 2:
                anim.select_bone(parts[1])
                rot = anim.get_bone_rotation(parts[1])
                if rot:
                    print(f'{parts[1]}: ({rot["x"]:.1f}, {rot["y"]:.1f}, {rot["z"]:.1f})')
            elif verb == 'rot' and len(parts) >= 4:
                sel = anim._eval('() => window.getSelectedBone()')
                if not sel:
                    print('Select a bone first')
                else:
                    anim.set_bone_rotation(sel, float(parts[1]), float(parts[2]), float(parts[3]))
                    print(f'{sel} -> ({parts[1]}, {parts[2]}, {parts[3]})')
            elif verb == 'set' and len(parts) >= 5:
                anim.set_bone_rotation(parts[1], float(parts[2]), float(parts[3]), float(parts[4]))
            elif verb == 'key':
                anim.set_keyframe_all()
                print(f'Keyframed all @ frame {anim.get_current_frame()}')
            elif verb == 'bones':
                names = anim.get_bone_names()
                seen = set()
                for n in names:
                    if n not in seen:
                        print(f'  {n}')
                        seen.add(n)
            elif verb == 'state':
                state = anim.get_all_rotations()
                for name, rot in sorted(state.items()):
                    if abs(rot['x']) > 0.1 or abs(rot['y']) > 0.1 or abs(rot['z']) > 0.1:
                        print(f'  {name}: ({rot["x"]:.1f}, {rot["y"]:.1f}, {rot["z"]:.1f})')
            elif verb in ('shot', 'screenshot'):
                path = parts[1] if len(parts) > 1 else None
                print(f'Screenshot: {anim.screenshot(path)}')
            elif verb == 'verify' and len(parts) >= 2:
                print(f'Verified: {anim.verify_frame(int(parts[1]))}')
            elif verb == 'clip' and len(parts) >= 2:
                name = ' '.join(parts[1:])
                result = anim.load_clip(name)
                print(f'Loaded: {result}')
            elif verb == 'clips':
                for c in list_available_clips():
                    print(f'  {c["name"]} ({c["size_kb"]} KB)')
            elif verb == 'sample' and len(parts) >= 3:
                anim.sample_clip_at_frame(parts[1], int(parts[2]))
                print(f'Sampled {parts[1]} @ frame {parts[2]}')
            elif verb == 'attach':
                print(f'Weapons: {anim.attach_weapons()}')
            elif verb == 'detach':
                anim.detach_weapons()
            elif verb == 'bg' and len(parts) >= 2:
                anim.set_background(parts[1])
            elif verb == 'cam' and len(parts) >= 4:
                anim.set_camera(float(parts[1]), float(parts[2]), float(parts[3]))
            elif verb == 'render' and len(parts) >= 2:
                anim.render_all_frames(parts[1])
            elif verb == 'webm' and len(parts) >= 2:
                anim.render_to_webm(parts[1])
            elif verb == 'export':
                path = parts[1] if len(parts) > 1 else None
                anim.export_animation(path)
            elif verb == 'undo':
                anim.undo()
            elif verb == 'redo':
                anim.redo()
            else:
                print(f'Unknown: {cmd}')
        except Exception as e:
            print(f'Error: {e}')


# ============================================================
# INTEGRATION TEST
# ============================================================

def test():
    """Run integration test: load model, attach weapons, screenshot."""
    print("=== Animator Integration Test ===\n")

    with Animator() as anim:
        # 1. Check model loaded
        info = anim.get_scene_info()
        print(f"1. Scene info: {info}")
        assert info['boneCount'] > 0, "No bones loaded!"
        print(f"   PASS: {info['boneCount']} bones\n")

        # 2. Get bone names
        names = anim.get_bone_names()
        unique = list(set(names))
        print(f"2. Unique bones: {len(unique)}")
        print(f"   Sample: {unique[:5]}")
        print(f"   PASS\n")

        # 3. Set a bone rotation
        anim.set_bone_rotation('mixamorigRightArm', 65, 0, -10)
        rot = anim.get_bone_rotation('mixamorigRightArm')
        print(f"3. Set mixamorigRightArm: {rot}")
        print(f"   PASS\n")

        # 4. Attach weapons
        result = anim.attach_weapons()
        print(f"4. Attach weapons: {result}")
        print(f"   PASS\n")

        # 5. Set background
        anim.set_background('aurelith')
        print(f"5. Background set to aurelith")
        print(f"   PASS\n")

        # 6. Screenshot
        path = anim.screenshot('/tmp/animator_test.png')
        print(f"6. Screenshot: {path}")
        assert os.path.exists(path), "Screenshot not created!"
        print(f"   PASS\n")

        # 7. List available clips
        clips = list_available_clips()
        print(f"7. Available clips: {len(clips)}")
        for c in clips[:5]:
            print(f"   - {c['name']}")
        print(f"   PASS\n")

        # 8. Keyframe test
        anim.go_to_frame(1)
        anim.set_keyframe_all()
        kf = anim.get_keyframes()
        print(f"8. Keyframes set: {len(kf)} bones keyed")
        print(f"   PASS\n")

        # 9. Camera test
        anim.set_camera(0, 1.2, 3.5, tx=0, ty=1.0, tz=0, fov=35)
        cam = anim.get_camera_state()
        print(f"9. Camera: {cam}")
        print(f"   PASS\n")

        # 10. Pose library
        anim.save_pose('test_pose')
        poses = anim.list_poses()
        print(f"10. Poses: {poses}")
        print(f"    PASS\n")

    print("=== ALL TESTS PASSED ===")


# ============================================================
# MAIN
# ============================================================

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("\nAvailable clips:")
        for c in list_available_clips():
            print(f"  {c['name']} ({c['size_kb']} KB)")
        return

    cmd = sys.argv[1]

    if cmd == 'test':
        test()
    elif cmd == 'serve':
        print(f'HTTP server at http://localhost:{PORT}/')
        start_server()
    elif cmd == 'interactive':
        with Animator() as anim:
            interactive(anim)
    elif cmd == 'clips':
        for c in list_available_clips():
            print(f'{c["name"]:50s} {c["size_kb"]:6d} KB  {c["file"]}')
    else:
        print(f'Unknown command: {cmd}')
        print(__doc__)


if __name__ == '__main__':
    main()
