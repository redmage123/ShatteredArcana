#!/usr/bin/env python3
"""Quick debug: check what the browser sees."""
import http.server
import socketserver
import threading
import time
from pathlib import Path
from urllib.parse import unquote

TOOL_DIR = Path(__file__).parent
PROJECT_ROOT = TOOL_DIR.parent.parent
MODEL_DIR = PROJECT_ROOT / "Art" / "DenizenAnimations" / "MixamoAnims"
PORT = 8765

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.path = '/index.html'
            self.directory = str(TOOL_DIR)
            return super().do_GET()
        elif self.path.startswith('/model/'):
            filename = unquote(self.path[7:])
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
        print(f"  HTTP: {args[0]}")

socketserver.TCPServer.allow_reuse_address = True
server = threading.Thread(target=lambda: socketserver.TCPServer(("", PORT), Handler).serve_forever(), daemon=True)
server.start()
time.sleep(0.5)

from playwright.sync_api import sync_playwright
pw = sync_playwright().start()
browser = pw.chromium.launch(headless=True, args=['--no-sandbox'])
page = browser.new_page(viewport={'width': 1280, 'height': 960})

# Capture console messages
page.on('console', lambda msg: print(f'  CONSOLE [{msg.type}]: {msg.text}'))
page.on('pageerror', lambda err: print(f'  PAGE ERROR: {err}'))

page.goto(f'http://localhost:{PORT}/', wait_until='networkidle', timeout=20000)
time.sleep(3)

# Check state
try:
    result = page.evaluate('() => { try { return window.getBoneNames ? window.getBoneNames().length : "no getBoneNames" } catch(e) { return e.message } }')
    print(f'\nBone count: {result}')
except Exception as e:
    print(f'\nEval error: {e}')

page.screenshot(path='/tmp/pose_debug.png')
print('Screenshot: /tmp/pose_debug.png')

browser.close()
pw.stop()
