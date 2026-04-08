#!/usr/bin/env python3
"""
Shattered Arcana — Harryhausen Pose Tool

A Playwright-based interactive pose editor for frame-by-frame animation.
Works like Ray Harryhausen's stop-motion: move to a frame, pose the bones
by selecting and dragging them, verify visually, lock the keyframe, advance.

Usage:
    python3 pose_tool.py serve          # Start the HTTP server + open browser
    python3 pose_tool.py screenshot     # Take a screenshot of current state
    python3 pose_tool.py pose <frame>   # Go to frame, take screenshot
    python3 pose_tool.py set <bone> <rx> <ry> <rz>  # Set bone rotation
    python3 pose_tool.py key            # Set keyframe at current frame
    python3 pose_tool.py key-all        # Keyframe all bones at current frame
    python3 pose_tool.py select <bone>  # Select a bone
    python3 pose_tool.py drag <bone> <dx> <dy>  # Drag bone by pixel offset
    python3 pose_tool.py bones          # List all bone names
    python3 pose_tool.py state          # Print all bone rotations
    python3 pose_tool.py export <path>  # Export keyframes to JSON
    python3 pose_tool.py import <path>  # Import keyframes from JSON
    python3 pose_tool.py render <dir>   # Render all frames as PNGs
    python3 pose_tool.py interactive    # Interactive REPL mode
"""

import http.server
import json
import os
import signal
import socketserver
import subprocess
import sys
import threading
import time
from pathlib import Path
from urllib.parse import unquote

# Paths
TOOL_DIR = Path(__file__).parent
PROJECT_ROOT = TOOL_DIR.parent.parent
MODEL_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations" / "MixamoAnims"
SCREENSHOT_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations" / "pose_screenshots"
PORT = 8765

# ============================================================
# HTTP SERVER — serves the HTML + FBX model
# ============================================================

class PoseHTTPHandler(http.server.SimpleHTTPRequestHandler):
    """Custom handler that serves the pose editor and model files."""

    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.path = '/index.html'
            self.directory = str(TOOL_DIR)
            return http.server.SimpleHTTPRequestHandler.do_GET(self)
        elif self.path.startswith('/model/'):
            # Serve from MixamoAnims directory
            filename = unquote(self.path[7:])  # strip /model/ and decode %20
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
                self.send_error(404, f'Model not found: {filename}')
            return
        else:
            self.directory = str(TOOL_DIR)
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

    def log_message(self, format, *args):
        pass  # Suppress logs


def start_server():
    """Start HTTP server in background thread."""
    socketserver.TCPServer.allow_reuse_address = True
    handler = PoseHTTPHandler
    with socketserver.TCPServer(("", PORT), handler) as httpd:
        httpd.serve_forever()


# ============================================================
# PLAYWRIGHT INTERFACE
# ============================================================

_browser = None
_page = None


def get_page():
    """Get or create the Playwright page."""
    global _browser, _page
    if _page is not None:
        return _page

    from playwright.sync_api import sync_playwright
    pw = sync_playwright().start()
    _browser = pw.chromium.launch(headless=True, args=[
        '--no-sandbox',
        '--disable-gpu',
        '--use-gl=swiftshader',  # Software WebGL
    ])
    _page = _browser.new_page(viewport={'width': 1280, 'height': 960})
    _page.goto(f'http://localhost:{PORT}/', wait_until='networkidle')

    # Wait for model to load
    _page.wait_for_function('() => window.getBoneNames && window.getBoneNames().length > 0',
                            timeout=30000)
    time.sleep(1)  # Let render settle
    return _page


def screenshot(path=None):
    """Take a screenshot and return the path."""
    page = get_page()
    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    if path is None:
        frame = page.evaluate('() => window.getCurrentFrame()')
        path = SCREENSHOT_DIR / f'frame_{frame:04d}.png'
    else:
        path = Path(path)
    page.screenshot(path=str(path))
    print(f'Screenshot: {path}')
    return str(path)


def go_to_frame(frame):
    """Navigate to a specific frame."""
    page = get_page()
    page.evaluate(f'() => window.goToFrame({frame})')
    time.sleep(0.1)


def select_bone(bone_name):
    """Select a bone by name."""
    page = get_page()
    page.evaluate(f'() => window.selectBone("{bone_name}")')
    time.sleep(0.05)


def set_bone_rotation(bone_name, rx, ry, rz):
    """Set a bone's rotation in degrees."""
    page = get_page()
    result = page.evaluate(f'() => window.setBoneRotation("{bone_name}", {rx}, {ry}, {rz})')
    return result


def get_bone_rotation(bone_name):
    """Get a bone's current rotation."""
    page = get_page()
    return page.evaluate(f'() => window.getBoneRotation("{bone_name}")')


def set_keyframe():
    """Set a keyframe at the current frame for the selected bone (or all)."""
    page = get_page()
    page.evaluate('() => window.setKeyframe()')


def get_bone_names():
    """List all bone names."""
    page = get_page()
    return page.evaluate('() => window.getBoneNames()')


def get_bones_state():
    """Get all bones' current rotations."""
    page = get_page()
    return page.evaluate('() => window.getBonesState()')


def drag_bone(bone_name, dx_pixels, dy_pixels):
    """Simulate dragging a bone by pixel offset (right-click drag).

    This mimics a human artist grabbing a bone and dragging it.
    dx controls Y-rotation, dy controls X-rotation.
    """
    page = get_page()
    # First select the bone
    select_bone(bone_name)
    time.sleep(0.1)

    # Get the bone's screen position by finding its sphere
    pos = page.evaluate(f'''() => {{
        const bone = window.getBoneNames().indexOf("{bone_name}");
        // Use center of viewport as fallback
        return {{ x: 640, y: 400 }};
    }}''')

    x, y = pos['x'], pos['y']

    # Right-click drag on the canvas
    canvas = page.locator('#viewport')
    canvas.dispatch_event('mousedown', {'clientX': x, 'clientY': y, 'button': 2})
    time.sleep(0.05)

    # Move in steps for smooth dragging
    steps = max(abs(dx_pixels), abs(dy_pixels), 1)
    for i in range(1, steps + 1):
        cx = x + int(dx_pixels * i / steps)
        cy = y + int(dy_pixels * i / steps)
        canvas.dispatch_event('mousemove', {'clientX': cx, 'clientY': cy})
        time.sleep(0.01)

    canvas.dispatch_event('mouseup', {'clientX': x + dx_pixels, 'clientY': y + dy_pixels, 'button': 2})
    time.sleep(0.1)


def export_keyframes(path):
    """Export keyframes to JSON file."""
    page = get_page()
    data = page.evaluate('() => window.getKeyframes()')
    with open(path, 'w') as f:
        f.write(data)
    print(f'Exported: {path}')


def import_keyframes(path):
    """Import keyframes from JSON file."""
    page = get_page()
    with open(path) as f:
        data = f.read()
    page.evaluate(f'(d) => window.importKeyframes(d)', data)
    print(f'Imported: {path}')


def render_all_frames(output_dir):
    """Render all frames to PNG files."""
    page = get_page()
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    total = page.evaluate('() => window.getTotalFrames()')
    print(f'Rendering {total} frames...')

    for frame in range(1, total + 1):
        go_to_frame(frame)
        time.sleep(0.05)
        path = output_dir / f'{frame:04d}.png'
        page.screenshot(path=str(path), clip={'x': 0, 'y': 0, 'width': 1280, 'height': 840})
        if frame % 24 == 0:
            print(f'  Frame {frame}/{total}')

    print(f'Done. Frames at: {output_dir}')


# ============================================================
# INTERACTIVE REPL
# ============================================================

def interactive():
    """Interactive mode for the pose tool."""
    print("Shattered Arcana Pose Tool — Interactive Mode")
    print("Commands: frame <n>, select <bone>, rot <rx> <ry> <rz>, key, bones, state, shot, quit")
    print()

    while True:
        try:
            cmd = input("pose> ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if not cmd:
            continue

        parts = cmd.split()
        verb = parts[0].lower()

        try:
            if verb in ('q', 'quit', 'exit'):
                break
            elif verb == 'frame' and len(parts) >= 2:
                go_to_frame(int(parts[1]))
                print(f'Frame: {parts[1]}')
            elif verb == 'select' and len(parts) >= 2:
                select_bone(parts[1])
                rot = get_bone_rotation(parts[1])
                print(f'Selected: {parts[1]} rot=({rot["x"]:.1f}, {rot["y"]:.1f}, {rot["z"]:.1f})')
            elif verb == 'rot' and len(parts) >= 4:
                # Need a selected bone
                page = get_page()
                bone = page.evaluate('() => document.getElementById("bone-label").textContent')
                if bone == 'No bone selected':
                    print('Select a bone first')
                else:
                    set_bone_rotation(bone, float(parts[1]), float(parts[2]), float(parts[3]))
                    print(f'{bone} -> ({parts[1]}, {parts[2]}, {parts[3]})')
            elif verb == 'set' and len(parts) >= 5:
                set_bone_rotation(parts[1], float(parts[2]), float(parts[3]), float(parts[4]))
                print(f'{parts[1]} -> ({parts[2]}, {parts[3]}, {parts[4]})')
            elif verb == 'key':
                set_keyframe()
                print('Keyframe set')
            elif verb == 'bones':
                names = get_bone_names()
                for n in names:
                    print(f'  {n}')
            elif verb == 'state':
                state = get_bones_state()
                for name, rot in sorted(state.items()):
                    if abs(rot['x']) > 0.1 or abs(rot['y']) > 0.1 or abs(rot['z']) > 0.1:
                        print(f'  {name}: ({rot["x"]:.1f}, {rot["y"]:.1f}, {rot["z"]:.1f})')
            elif verb in ('shot', 'screenshot'):
                path = parts[1] if len(parts) > 1 else None
                screenshot(path)
            elif verb == 'drag' and len(parts) >= 4:
                drag_bone(parts[1], int(parts[2]), int(parts[3]))
                print(f'Dragged {parts[1]} by ({parts[2]}, {parts[3]}) px')
            elif verb == 'export' and len(parts) >= 2:
                export_keyframes(parts[1])
            elif verb == 'import' and len(parts) >= 2:
                import_keyframes(parts[1])
            elif verb == 'render' and len(parts) >= 2:
                render_all_frames(parts[1])
            else:
                print(f'Unknown: {cmd}')
        except Exception as e:
            print(f'Error: {e}')


# ============================================================
# MAIN
# ============================================================

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return

    cmd = sys.argv[1]

    # Start server for all commands
    server_thread = threading.Thread(target=start_server, daemon=True)
    server_thread.start()
    time.sleep(0.5)

    if cmd == 'serve':
        print(f'Pose editor running at http://localhost:{PORT}/')
        print('Press Ctrl+C to stop.')
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            pass

    elif cmd == 'screenshot':
        path = sys.argv[2] if len(sys.argv) > 2 else None
        screenshot(path)

    elif cmd == 'pose':
        frame = int(sys.argv[2])
        go_to_frame(frame)
        screenshot()

    elif cmd == 'set' and len(sys.argv) >= 6:
        bone = sys.argv[2]
        rx, ry, rz = float(sys.argv[3]), float(sys.argv[4]), float(sys.argv[5])
        set_bone_rotation(bone, rx, ry, rz)
        print(f'{bone} -> ({rx}, {ry}, {rz})')

    elif cmd == 'key':
        set_keyframe()

    elif cmd == 'key-all':
        set_keyframe()

    elif cmd == 'select':
        select_bone(sys.argv[2])

    elif cmd == 'drag' and len(sys.argv) >= 5:
        drag_bone(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]))

    elif cmd == 'bones':
        names = get_bone_names()
        for n in names:
            print(n)

    elif cmd == 'state':
        state = get_bones_state()
        print(json.dumps(state, indent=2))

    elif cmd == 'export':
        export_keyframes(sys.argv[2])

    elif cmd == 'import':
        import_keyframes(sys.argv[2])

    elif cmd == 'render':
        render_all_frames(sys.argv[2])

    elif cmd == 'interactive':
        interactive()

    else:
        print(__doc__)


if __name__ == '__main__':
    main()
