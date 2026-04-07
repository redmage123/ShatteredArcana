Shattered Arcana — Motion Reference Capture Tool
=================================================

This tool captures YouTube videos for use as motion reference
when animating 3D characters in Blender.

REQUIREMENTS:
- Python 3.10+ (https://python.org)
- ffmpeg (https://ffmpeg.org) — for frame extraction
- SSH key set up for server access

INSTALL:
1. Double-click install.bat
2. Follow the prompts

USAGE:
  mc search "medieval sword salute"
  mc capture "https://youtube.com/watch?v=VIDEO_ID"
  mc capture "https://youtube.com/watch?v=VIDEO_ID" --duration 120
  mc upload                                          
  mc build-library
  mc build-library --categories knight_sword elf_archer
  mc list

The tool runs a HEADED browser (visible window) on Windows,
which avoids YouTube's headless bot detection. It:
1. Opens Chromium with stealth patches
2. Plays the YouTube video fullscreen
3. Records the screen via Playwright
4. Extracts frames (1 per second) 
5. Stores everything in ~/motion_capture/library/

Use "mc upload" to SCP all captures to the production server
at 78.47.104.139 and dev server at 176.9.99.103.

Server paths:
  Prod: /home/aielevate/motion_library/
  Dev:  /home/bbrelin/ShatteredArcana/Art/Reference/
