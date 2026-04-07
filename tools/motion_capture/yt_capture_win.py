#!/usr/bin/env python3
"""
Shattered Arcana — Motion Reference Library Builder (Windows)

Fully automated: searches YouTube, picks best videos, captures them,
extracts frames, and SCPs everything to the dev server.

Usage:
    python motion_capture.py              # Run full auto-build
    python motion_capture.py --dry-run    # Show what would be captured
    python motion_capture.py --category knight_sword
    python motion_capture.py --search-only
"""

import sys
import argparse
import time
import json
import os
import shutil
import random
import subprocess
from pathlib import Path
from datetime import datetime

try:
    from playwright.sync_api import sync_playwright
    from playwright_stealth import Stealth
    HAS_STEALTH = True
except ImportError:
    try:
        from playwright.sync_api import sync_playwright
        HAS_STEALTH = False
    except ImportError:
        print("ERROR: Run install.bat first to set up dependencies")
        sys.exit(1)

# ============================================================
# CONFIG
# ============================================================
HOME = Path.home()
LIBRARY_DIR = HOME / "motion_capture" / "library"
VIDEOS_DIR = LIBRARY_DIR / "videos"
FRAMES_DIR = LIBRARY_DIR / "frames"
INDEX_FILE = LIBRARY_DIR / "index.json"
LOG_FILE = LIBRARY_DIR / "capture_log.txt"
SEARCHES_FILE = LIBRARY_DIR / "searches.json"

# Dev server — direct SCP from Windows laptop
DEV_HOST = "176.9.99.103"
DEV_USER = "bbrelin"
DEV_PATH = "/home/bbrelin/ShatteredArcana/Art/Reference"

# ============================================================
# DEFAULT SEARCHES — written to searches.json on first run
# Edit searches.json to add/remove/modify topics
# ============================================================
DEFAULT_SEARCHES = {
    "knight_sword_salute": {
        "description": "Knight drawing sword and performing formal salute",
        "searches": [
            "medieval sword salute demonstration HEMA",
            "knightly salute longsword ceremony",
            "fencing salute with sword tutorial"
        ],
        "max_duration": 120,
        "priority": 1
    },
    "longsword_draw": {
        "description": "Drawing a longsword from scabbard",
        "searches": [
            "drawing longsword from scabbard slow motion",
            "how to draw a medieval sword properly",
            "sword unsheathing technique"
        ],
        "max_duration": 180,
        "priority": 1
    },
    "longsword_flourish": {
        "description": "Sword flourish and spinning techniques",
        "searches": [
            "longsword flourish demonstration HEMA",
            "sword spinning technique slow motion",
            "sword flourish choreography tutorial"
        ],
        "max_duration": 120,
        "priority": 1
    },
    "longsword_combat": {
        "description": "Longsword fighting stances and strikes",
        "searches": [
            "HEMA longsword sparring slow motion",
            "medieval longsword combat techniques",
            "longsword guard positions demonstration"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "sword_and_shield": {
        "description": "Fighting with sword and shield together",
        "searches": [
            "sword and shield combat technique medieval",
            "shield wall fighting demonstration",
            "viking sword shield combat HEMA"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "axe_combat": {
        "description": "Battle axe fighting for dwarf warriors",
        "searches": [
            "battle axe fighting technique medieval",
            "dane axe combat demonstration HEMA",
            "two handed axe swing slow motion"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "staff_combat": {
        "description": "Staff/bo fighting for wizards",
        "searches": [
            "bo staff fighting demonstration slow motion",
            "quarterstaff combat medieval technique",
            "wizard staff spinning choreography"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "archery": {
        "description": "Bow drawing and shooting for elven archers",
        "searches": [
            "traditional archery draw release slow motion",
            "longbow shooting technique side view",
            "archery form demonstration close up"
        ],
        "max_duration": 180,
        "priority": 2
    },
    "dagger_fighting": {
        "description": "Dagger/knife fighting for rogues and assassins",
        "searches": [
            "medieval dagger fighting technique HEMA",
            "knife combat choreography slow motion",
            "dual dagger fighting demonstration"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "plate_armor_movement": {
        "description": "Moving in full plate armor",
        "searches": [
            "knight in full plate armor walking running",
            "medieval armor mobility demonstration",
            "HEMA armored combat movement"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "chainmail_movement": {
        "description": "Moving in chainmail",
        "searches": [
            "chainmail armor movement demonstration",
            "medieval chain armor fighting"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "spell_casting": {
        "description": "Wizard/mage spell casting gestures",
        "searches": [
            "wizard spell casting motion reference",
            "dramatic hand gestures magic performance",
            "sorcerer casting spell choreography"
        ],
        "max_duration": 120,
        "priority": 2
    },
    "ritual_gestures": {
        "description": "Ritual and ceremonial hand movements for warlocks",
        "searches": [
            "ritual hand movements ceremonial",
            "occult gesture demonstration theatrical",
            "conductor dramatic hand movements orchestra"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "reptile_movement": {
        "description": "Reptilian movement for lizardmen and draconians",
        "searches": [
            "komodo dragon walking slow motion",
            "monitor lizard aggressive movement",
            "reptile running slow motion side view"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "bird_wings": {
        "description": "Wing movement for dragons and flying creatures",
        "searches": [
            "eagle takeoff slow motion wings",
            "large bird of prey flying slow motion",
            "bat flying wing movement slow motion"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "predator_stalk": {
        "description": "Predatory stalking movement for demons and monsters",
        "searches": [
            "big cat stalking prey slow motion",
            "wolf pack hunting movement",
            "predator aggressive movement reference"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "ethereal_dance": {
        "description": "Flowing graceful movement for fey and ethereal beings",
        "searches": [
            "contemporary dance flowing movement slow",
            "ethereal fairy dance performance",
            "ballet adagio slow graceful movement"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "aggressive_stance": {
        "description": "Threatening and aggressive body language",
        "searches": [
            "aggressive fighting stance martial arts",
            "intimidation body language demonstration",
            "warrior battle cry stance"
        ],
        "max_duration": 120,
        "priority": 2
    },
    "undead_shamble": {
        "description": "Shambling undead movement",
        "searches": [
            "zombie walk tutorial acting",
            "undead shambling movement reference",
            "horror movie monster walk"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "mounted_combat": {
        "description": "Fighting on horseback for cavalry units",
        "searches": [
            "mounted combat sword horseback",
            "jousting demonstration medieval",
            "cavalry charge slow motion"
        ],
        "max_duration": 180,
        "priority": 4
    },
    "idle_stance": {
        "description": "Standing idle with subtle breathing and weight shifts",
        "searches": [
            "soldier standing at attention breathing",
            "guard standing idle reference animation",
            "human idle pose subtle movement reference"
        ],
        "max_duration": 60,
        "priority": 2
    },
    "choy_li_fut": {
        "description": "Choy Li Fut kung fu forms — wide sweeping strikes, low stances",
        "searches": [
            "Choy Li Fut form demonstration",
            "Choy Lee Fut kung fu form slow motion",
            "Choy Li Fut traditional form full",
            "Cai Li Fo kung fu techniques"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "northern_shaolin": {
        "description": "Northern Shaolin kung fu forms — long range kicks, acrobatic movement",
        "searches": [
            "Northern Shaolin kung fu form demonstration",
            "Northern Shaolin long fist form full",
            "Chang Quan northern style kung fu form",
            "Shaolin wushu form traditional"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "lizardman_reptile": {
        "description": "Reptilian movement for lizardmen and draconians — swaying gait, head bobbing",
        "searches": [
            "komodo dragon walking slow motion close up",
            "monitor lizard aggressive display",
            "crocodile walking on land slow motion",
            "iguana running slow motion side view"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "demon_gorilla_threat": {
        "description": "Primal threat displays for demons — gorilla charges, bear standing",
        "searches": [
            "gorilla threat display chest beating",
            "bear standing charging slow motion",
            "big cat snarling aggressive behavior",
            "primate aggressive display"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "demon_butoh_contortion": {
        "description": "Supernatural demon movement — Butoh dance, contortion",
        "searches": [
            "Butoh dance performance slow movement",
            "contortionist performance dark theatrical",
            "parkour aggressive movement wall climb"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "dragon_wings_flight": {
        "description": "Dragon wing movement — raptors, bats, heavy takeoff",
        "searches": [
            "eagle takeoff slow motion close up wings",
            "bat flying slow motion wing membrane",
            "pelican takeoff heavy bird slow motion",
            "large raptor soaring wing movement"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "dragon_head_neck": {
        "description": "Dragon head and neck movement — snake strikes, lizard head bob",
        "searches": [
            "snake striking slow motion",
            "cobra head movement sway",
            "crocodile snapping jaws slow motion",
            "monitor lizard head movement hunting"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "fey_aerial_dance": {
        "description": "Fey/dream weaver movement — ethereal, weightless, flowing",
        "searches": [
            "aerial silk performance slow graceful",
            "contemporary dance flowing arms slow",
            "tai chi form full slow flowing",
            "underwater swimming graceful movement"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "undead_noh_puppet": {
        "description": "Undead movement — unnatural, jerky, deliberate",
        "searches": [
            "Noh theater slow walk movement",
            "marionette puppet movement reference",
            "zombie walk acting tutorial detailed",
            "stop motion puppet walking reference"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "dryad_organic": {
        "description": "Dryad/ether weaver movement — organic, vine-like, flowing",
        "searches": [
            "belly dance flowing movement slow",
            "ribbon gymnastics performance",
            "vine growing timelapse movement",
            "tree swaying wind slow motion"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "efreeti_fire_dance": {
        "description": "Efreeti/Djinn movement — fire dancers, spinning, acrobatic",
        "searches": [
            "fire dancer poi spinning performance",
            "capoeira acrobatic fluid combat",
            "whirling dervish spinning performance",
            "fire staff spinning slow motion"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "insectoid_chithari": {
        "description": "Insectoid movement for Chithari — mantis, spider, scorpion",
        "searches": [
            "praying mantis movement hunting slow motion",
            "spider walking close up slow motion",
            "scorpion striking slow motion",
            "crab fighting aggressive display"
        ],
        "max_duration": 120,
        "priority": 3
    },
    "crystal_elf_fencing": {
        "description": "Crystal elf movement — precise, elegant, athletic",
        "searches": [
            "fencing epee bout slow motion",
            "wushu competition form performance",
            "ballet male variation athletic",
            "rapier fencing demonstration technique"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "hotd_dragon_combat": {
        "description": "House of the Dragon — dragon flight, dragon combat, dragonrider",
        "searches": [
            "House of the Dragon dragon fight scene",
            "House of the Dragon Vhagar vs Arrax",
            "House of the Dragon dragon flying scene",
            "Game of Thrones dragon attack scene"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "hotd_sword_combat": {
        "description": "House of the Dragon — sword fighting, Crabfeeder, tourney",
        "searches": [
            "House of the Dragon sword fight scene",
            "House of the Dragon Daemon fight scene",
            "Game of Thrones best sword fight scenes",
            "House of the Dragon tourney combat scene"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "lotr_combat": {
        "description": "Lord of the Rings — sword combat, orc movement, Nazgul",
        "searches": [
            "Lord of the Rings Aragorn fight scene",
            "Lord of the Rings Legolas combat scene",
            "Lord of the Rings orc battle scene",
            "Rings of Power Sauron fight scene"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "lotr_creatures": {
        "description": "Lord of the Rings — Balrog, cave troll, Shelob, fell beast",
        "searches": [
            "Lord of the Rings Balrog scene",
            "Lord of the Rings cave troll fight",
            "Lord of the Rings fell beast Nazgul flying",
            "Lord of the Rings Shelob spider scene"
        ],
        "max_duration": 300,
        "priority": 3
    },
    "witcher_combat": {
        "description": "The Witcher — Geralt sword fighting, monster combat",
        "searches": [
            "Witcher Geralt sword fight scene",
            "Witcher Blaviken fight scene",
            "Witcher monster fight scene",
            "Witcher Season 3 combat scene"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "witcher_magic": {
        "description": "The Witcher — Yennefer magic, signs, spell casting",
        "searches": [
            "Witcher Yennefer magic scene",
            "Witcher signs Igni Aard casting",
            "Witcher spell casting scene"
        ],
        "max_duration": 180,
        "priority": 3
    },
    "fantasy_archery": {
        "description": "Fantasy archery — Legolas, Hawkeye, elven archer scenes",
        "searches": [
            "Legolas archery scenes compilation",
            "fantasy movie archery best scenes",
            "elven archer movie scenes bow"
        ],
        "max_duration": 300,
        "priority": 3
    },
    "fantasy_magic_casting": {
        "description": "Fantasy movie magic — Doctor Strange, Harry Potter, spell effects",
        "searches": [
            "Doctor Strange magic scene hand gestures",
            "Harry Potter wizard duel scene",
            "fantasy movie magic casting compilation",
            "Gandalf staff magic scene"
        ],
        "max_duration": 300,
        "priority": 3
    },
    "fantasy_undead_army": {
        "description": "Fantasy undead — White Walkers, Army of the Dead, Nazgul",
        "searches": [
            "Game of Thrones White Walker scene",
            "Lord of the Rings Army of the Dead scene",
            "fantasy movie undead army scene",
            "Game of Thrones Night King scene"
        ],
        "max_duration": 300,
        "priority": 3
    },
    "conan_barbarian_combat": {
        "description": "Conan/barbarian style — raw heavy weapon fighting",
        "searches": [
            "Conan the Barbarian fight scene",
            "barbarian sword fight movie scene",
            "Viking movie combat scene axe",
            "Northman fight scene"
        ],
        "max_duration": 300,
        "priority": 3
    },
    "dark_souls_animation": {
        "description": "Dark Souls / Elden Ring — game animation reference for knight movement",
        "searches": [
            "Dark Souls knight animation reference",
            "Elden Ring combat animation slow motion",
            "Dark Souls boss fight slow motion",
            "Elden Ring sword moveset all attacks"
        ],
        "max_duration": 300,
        "priority": 2
    },
    "monster_hunter_creatures": {
        "description": "Monster Hunter — creature animation, dragon attacks, wyvern flight",
        "searches": [
            "Monster Hunter World monster animation",
            "Monster Hunter dragon attack animation",
            "Monster Hunter creature movement reference",
            "Monster Hunter Rathalos flying"
        ],
        "max_duration": 300,
        "priority": 3
    }
}


def load_search_library():
    """Load search topics from searches.json. Creates it with defaults on first run."""
    ensure_dirs()
    if SEARCHES_FILE.exists():
        try:
            data = json.loads(SEARCHES_FILE.read_text())
            log(f"Loaded {len(data)} search topics from {SEARCHES_FILE}")
            return data
        except json.JSONDecodeError:
            log(f"WARNING: {SEARCHES_FILE} is invalid JSON, using defaults")
            return DEFAULT_SEARCHES
    else:
        # First run — write defaults to file
        SEARCHES_FILE.write_text(json.dumps(DEFAULT_SEARCHES, indent=2))
        log(f"Created {SEARCHES_FILE} with {len(DEFAULT_SEARCHES)} default topics")
        log(f"Edit this file to add/remove/modify search topics.")
        return DEFAULT_SEARCHES


# Load on import — will be populated in main()
SEARCH_LIBRARY = {}

# ============================================================
# BROWSER SESSION — persistent profile to look like a real user
# ============================================================
SESSION_FILE = LIBRARY_DIR / "browser_session.json"


def ensure_dirs():
    for d in [LIBRARY_DIR, VIDEOS_DIR, FRAMES_DIR]:
        d.mkdir(parents=True, exist_ok=True)


def human_delay(min_s=1.0, max_s=3.0):
    """Random delay to simulate human timing."""
    time.sleep(random.uniform(min_s, max_s))


def human_mouse(page, duration=2.0):
    """Random mouse movements to look human."""
    import random as r
    w = page.viewport_size['width']
    h = page.viewport_size['height']
    steps = r.randint(3, 6)
    for _ in range(steps):
        x = r.randint(100, w - 100)
        y = r.randint(100, h - 100)
        page.mouse.move(x, y, steps=r.randint(5, 15))
        time.sleep(r.uniform(0.1, 0.4))


def human_scroll(page, direction="down", amount=None):
    """Random scroll to look human."""
    if amount is None:
        amount = random.randint(100, 400)
    if direction == "down":
        page.mouse.wheel(0, amount)
    else:
        page.mouse.wheel(0, -amount)
    time.sleep(random.uniform(0.3, 0.8))


def dismiss_consent(page):
    """Handle all YouTube consent/cookie dialogs."""
    for btn_text in ['Accept all', 'Reject all', 'Alle akzeptieren',
                     'Alle ablehnen', 'I agree', 'Accept', 'Akzeptieren']:
        try:
            page.click(f"button:has-text('{btn_text}')", timeout=1500)
            time.sleep(1)
            return True
        except:
            pass
    return False


def wait_for_video_playing(page, timeout=20):
    """Wait until the video is actually playing. Returns True if playing."""
    for attempt in range(timeout // 2):
        try:
            state = page.evaluate("""() => {
                const v = document.querySelector('video');
                if (!v) return {found: false};
                return {
                    found: true,
                    paused: v.paused,
                    currentTime: v.currentTime,
                    duration: v.duration || 0,
                    readyState: v.readyState,
                };
            }""")
            if state.get('found') and not state.get('paused') and state.get('currentTime', 0) > 0.5:
                return True
            if state.get('found') and state.get('paused'):
                # Try clicking to play
                page.evaluate('() => { const v = document.querySelector("video"); if (v) v.play(); }')
        except:
            pass
        time.sleep(2)
    return False


def is_bot_blocked(page):
    """Check if YouTube is showing bot detection screen."""
    try:
        text = page.evaluate('() => document.body.innerText.substring(0, 3000)')
        return ('confirm you' in text.lower() or
                'not a bot' in text.lower() or
                'unusual traffic' in text.lower())
    except:
        return False


def create_browser(pw, record_dir=None):
    """Create a browser that looks like a real human.
    Uses system Chrome if available, persistent session, stealth patches."""
    ensure_dirs()

    # Try to use system Chrome instead of Playwright's Chromium
    chrome_paths = [
        "C:/Program Files/Google/Chrome/Application/chrome.exe",
        "C:/Program Files (x86)/Google/Chrome/Application/chrome.exe",
        os.path.expanduser("~/AppData/Local/Google/Chrome/Application/chrome.exe"),
    ]
    chrome_path = None
    for cp in chrome_paths:
        if os.path.exists(cp):
            chrome_path = cp
            break

    launch_args = {
        'headless': False,
        'args': [
            '--disable-blink-features=AutomationControlled',
            '--disable-dev-shm-usage',
            '--no-first-run',
            '--no-default-browser-check',
        ],
    }
    if chrome_path:
        launch_args['executable_path'] = chrome_path
        launch_args['channel'] = 'chrome'
        log(f"  Using system Chrome: {chrome_path}")

    browser = pw.chromium.launch(**launch_args)

    # Context with persistent session
    ctx_args = {
        'locale': 'en-US',
        'timezone_id': 'America/New_York',
        'viewport': {'width': random.choice([1366, 1440, 1536, 1920]),
                      'height': random.choice([768, 900, 864, 1080])},
    }

    # Load saved session state (cookies, localStorage) if exists
    if SESSION_FILE.exists():
        try:
            ctx_args['storage_state'] = str(SESSION_FILE)
            log("  Loaded saved browser session")
        except:
            pass

    if record_dir:
        os.makedirs(record_dir, exist_ok=True)
        ctx_args['record_video_dir'] = str(record_dir)
        ctx_args['record_video_size'] = {'width': 1920, 'height': 1080}

    context = browser.new_context(**ctx_args)

    # Set consent cookies if no saved session
    if not SESSION_FILE.exists():
        context.add_cookies([
            {'name': 'CONSENT', 'value': 'YES+cb.20210328-17-p0.en+FX+971',
             'domain': '.youtube.com', 'path': '/'},
            {'name': 'SOCS',
             'value': 'CAISHAgBEhJnd3NfMjAyMzA4MTAtMF9SQzIaAmVuIAEaBgiA_LSmBg',
             'domain': '.youtube.com', 'path': '/'},
        ])

    return browser, context


def save_session(context):
    """Save browser session state for reuse."""
    try:
        ensure_dirs()
        context.storage_state(path=str(SESSION_FILE))
    except:
        pass


def log(msg):
    ts = datetime.now().strftime("%H:%M:%S")
    line = f"[{ts}] {msg}"
    # Strip non-ASCII for safe console/file output on Windows
    safe_line = line.encode('ascii', 'replace').decode('ascii')
    print(safe_line)
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(line + "\n")


def load_index():
    if INDEX_FILE.exists():
        return json.loads(INDEX_FILE.read_text())
    return {"videos": [], "searches": [], "created": datetime.now().isoformat()}


def save_index(index):
    INDEX_FILE.write_text(json.dumps(index, indent=2))


def already_captured(index, category):
    """Check if we already have a video for this category."""
    return any(v.get("category") == category for v in index.get("videos", []))


def parse_duration(dur_str):
    """Parse YouTube duration string like '3:42' or '12:05' to seconds."""
    if not dur_str:
        return 0
    parts = dur_str.strip().split(":")
    try:
        if len(parts) == 3:
            return int(parts[0]) * 3600 + int(parts[1]) * 60 + int(parts[2])
        elif len(parts) == 2:
            return int(parts[0]) * 60 + int(parts[1])
        elif len(parts) == 1:
            return int(parts[0])
    except ValueError:
        return 0
    return 0


def extract_video_id(url):
    """Extract YouTube video ID from various URL formats."""
    import re
    # Standard: youtube.com/watch?v=ID
    m = re.search(r'[?&]v=([a-zA-Z0-9_-]{11})', url)
    if m: return m.group(1)
    # Shorts: youtube.com/shorts/ID
    m = re.search(r'/shorts/([a-zA-Z0-9_-]{11})', url)
    if m: return m.group(1)
    # Embed: youtube.com/embed/ID
    m = re.search(r'/embed/([a-zA-Z0-9_-]{11})', url)
    if m: return m.group(1)
    # youtu.be/ID
    m = re.search(r'youtu\.be/([a-zA-Z0-9_-]{11})', url)
    if m: return m.group(1)
    return None


def pick_best_result(results, max_duration=300):
    """Pick the best video from search results.
    Prefers: moderate length, high view counts, relevant channels.
    Filters out: shorts, live streams, playlists."""
    scored = []
    for r in results:
        title = r.get("title", "").lower()
        url = r.get("url", "")

        # Skip shorts, live, playlists
        if "#shorts" in title or "/shorts/" in url or "#short" in title:
            continue
        if "live" in title and ("stream" in title or "24/7" in title):
            continue
        # Skip very short hashtag-heavy titles (usually shorts)
        if title.count("#") >= 3:
            continue

        dur = parse_duration(r.get("duration", ""))
        if dur > max_duration or dur < 15:
            continue  # too long or too short (raised min to 15s)

        score = 100
        # Prefer 30-120 second videos
        if 30 <= dur <= 120:
            score += 20
        elif 120 < dur <= 180:
            score += 10
        
        # Prefer videos with more views (rough heuristic from meta text)
        meta = r.get("meta", "").lower()
        if "m views" in meta or "million" in meta:
            score += 30
        elif "k views" in meta:
            score += 15
        
        # Prefer channels with relevant keywords
        channel = r.get("channel", "").lower()
        for kw in ["hema", "martial", "sword", "medieval", "combat", "animation", "reference"]:
            if kw in channel:
                score += 10
        
        scored.append((score, r))
    
    if not scored:
        return results[0] if results else None
    
    scored.sort(key=lambda x: x[0], reverse=True)
    return scored[0][1]


def search_youtube(query, max_results=8):
    """Search YouTube like a human — go to youtube.com, type in search, browse results."""
    log(f"Searching: {query}")

    with sync_playwright() as p:
        browser, context = create_browser(p)
        page = context.new_page()

        if HAS_STEALTH:
            Stealth().apply_stealth_sync(page)

        # Step 1: Go to YouTube homepage first (not direct search URL)
        page.goto("https://www.youtube.com", timeout=45000)
        human_delay(2, 4)
        dismiss_consent(page)
        human_delay(1, 2)

        # Step 2: Random mouse movement on homepage
        human_mouse(page)
        human_scroll(page, "down")
        human_delay(1, 2)

        # Step 3: Click the search box and type the query like a human
        try:
            search_box = page.query_selector('input#search, input[name="search_query"]')
            if search_box:
                search_box.click()
                human_delay(0.5, 1)
                # Type slowly like a human
                for char in query:
                    page.keyboard.type(char, delay=random.randint(30, 120))
                    if random.random() < 0.05:  # occasional pause
                        human_delay(0.3, 0.6)
                human_delay(0.5, 1)
                page.keyboard.press("Enter")
            else:
                # Fallback to direct URL
                page.goto(f"https://www.youtube.com/results?search_query={query.replace(' ', '+')}", timeout=45000)
        except:
            page.goto(f"https://www.youtube.com/results?search_query={query.replace(' ', '+')}", timeout=45000)

        human_delay(3, 5)
        dismiss_consent(page)
        human_delay(1, 2)

        # Step 4: Scroll through results like a human
        human_scroll(page, "down", 200)
        human_delay(0.5, 1.5)
        human_scroll(page, "down", 150)
        human_delay(0.5, 1)

        # Step 5: Extract results
        results = page.evaluate("""(max) => {
            const items = document.querySelectorAll('ytd-video-renderer');
            const results = [];
            for (let i = 0; i < Math.min(items.length, max); i++) {
                const item = items[i];
                const titleEl = item.querySelector('#video-title');
                const linkEl = item.querySelector('a#video-title');
                const channelEl = item.querySelector('#channel-name a');
                const durEl = item.querySelector('.ytd-thumbnail-overlay-time-status-renderer');
                const metaEl = item.querySelector('#metadata-line');
                if (titleEl && linkEl) {
                    const href = linkEl.getAttribute('href') || '';
                    results.push({
                        title: titleEl.textContent?.trim() || '',
                        url: href.startsWith('http') ? href : 'https://www.youtube.com' + href,
                        channel: channelEl?.textContent?.trim() || '',
                        duration: durEl?.textContent?.trim() || '',
                        meta: metaEl?.textContent?.trim() || '',
                    });
                }
            }
            return results;
        }""", max_results)

        # Save session for next time
        save_session(context)
        context.close()
        browser.close()

    log(f"  Found {len(results)} results")
    return results


def capture_video(url, category="uncategorized", max_duration=300):
    """Capture a YouTube video with human-like browsing behavior."""
    ensure_dirs()

    vid_id = extract_video_id(url) or f"vid_{int(time.time())}"
    output = str(VIDEOS_DIR / f"{vid_id}.webm")

    if os.path.exists(output):
        log(f"  Already captured: {vid_id}")
        return None

    log(f"  Capturing: {vid_id}")
    raw_dir = str(LIBRARY_DIR / f"_raw_{vid_id}")

    # Convert shorts URL
    if '/shorts/' in url and vid_id:
        url = f'https://www.youtube.com/watch?v={vid_id}'
        log(f"  Converted shorts URL to: {url}")

    with sync_playwright() as p:
        browser, context = create_browser(p, record_dir=raw_dir)
        page = context.new_page()

        if HAS_STEALTH:
            Stealth().apply_stealth_sync(page)

        # Step 1: Go to YouTube homepage first, browse briefly
        human_delay(1, 2)
        page.goto("https://www.youtube.com", timeout=45000)
        human_delay(2, 4)
        dismiss_consent(page)
        human_mouse(page, 1.5)
        human_scroll(page, "down")
        human_delay(1, 3)

        # Step 2: Navigate to the video
        page.goto(url, timeout=45000)
        human_delay(3, 5)
        dismiss_consent(page)
        human_delay(1, 2)

        # Step 3: Check for bot detection
        if is_bot_blocked(page):
            log("  WARNING: Bot detection triggered. Skipping.")
            save_session(context)
            context.close()
            browser.close()
            shutil.rmtree(raw_dir, ignore_errors=True)
            return None

        # Step 4: Skip ads with human-like clicking
        for sel in ['.ytp-ad-skip-button', '.ytp-ad-skip-button-modern', '.ytp-skip-ad-button']:
            try:
                human_delay(1, 3)
                page.click(sel, timeout=5000)
                log("  Skipped ad")
                human_delay(1, 2)
                break
            except:
                pass

        # Step 5: Wait for video to actually start playing
        log("  Waiting for video to play...")
        is_playing = wait_for_video_playing(page, timeout=20)
        if not is_playing:
            log("  WARNING: Video not playing after 20s. Skipping.")
            save_session(context)
            context.close()
            browser.close()
            shutil.rmtree(raw_dir, ignore_errors=True)
            return None

        log("  Video confirmed playing")
        
        # Wait for video element to appear (may take time after ads/consent)
        vid_found = False
        for attempt in range(5):
            vid_found = page.evaluate('() => !!document.querySelector("video")')
            if vid_found:
                break
            log(f"  Waiting for video element (attempt {attempt+1}/5)...")
            time.sleep(3)

        # Check for bot detection
        is_blocked = page.evaluate('() => document.body.innerText.includes("confirm you") || document.body.innerText.includes("not a bot")')
        if is_blocked:
            log("  WARNING: Bot detection triggered. Skipping this video.")
            context.close()
            browser.close()
            shutil.rmtree(raw_dir, ignore_errors=True)
            return None

        vid_info = {'duration': 0, 'width': 0, 'height': 0}
        duration = max_duration

        if vid_found:
            try:
                page.evaluate('() => { const v = document.querySelector("video"); if (v) v.play(); }')
                time.sleep(3)
                vid_info = page.evaluate('''() => {
                    const v = document.querySelector("video");
                    if (!v) return {duration: 0, width: 0, height: 0};
                    return {duration: v.duration || 0, width: v.videoWidth || 0, height: v.videoHeight || 0};
                }''')
            except Exception as e:
                log(f"  WARNING: Could not play video: {e}")

            actual_dur = vid_info.get('duration', 0)
            if actual_dur > 0:
                duration = min(int(actual_dur) + 3, max_duration)

            log(f"  Video: {vid_info['width']}x{vid_info['height']}, {actual_dur:.0f}s (recording {duration}s)")

            try:
                page.evaluate('''() => {
                    const v = document.querySelector("video");
                    if (!v) return;
                    v.style.position = "fixed"; v.style.top = "0"; v.style.left = "0";
                    v.style.width = "100vw"; v.style.height = "100vh";
                    v.style.zIndex = "99999"; v.style.objectFit = "contain";
                    v.style.backgroundColor = "black"; v.currentTime = 0; v.play();
                }''')
            except Exception as e:
                log(f"  WARNING: Could not fullscreen video: {e}")
        else:
            log("  WARNING: No video element found — bot blocked or page error")
            duration = min(60, max_duration)
        
        start = time.time()
        while time.time() - start < duration:
            time.sleep(2)
            elapsed = int(time.time() - start)
            if vid_found:
                try:
                    if page.evaluate('() => document.querySelector("video")?.ended || false'):
                        log(f"  Video ended at {elapsed}s")
                        time.sleep(2)
                        break
                except:
                    pass
            sys.stdout.write(f"\r  Recording: {elapsed}/{duration}s")
            sys.stdout.flush()
        
        print()
        video_path = page.video.path()
        context.close()
        browser.close()
    
    if video_path and os.path.exists(video_path):
        shutil.move(video_path, output)
        size_mb = os.path.getsize(output) / 1024 / 1024
        log(f"  Saved: {output} ({size_mb:.1f}MB)")
    else:
        log("  ERROR: No video recorded")
        shutil.rmtree(raw_dir, ignore_errors=True)
        return None
    
    shutil.rmtree(raw_dir, ignore_errors=True)
    
    # Extract frames
    frame_dir = str(FRAMES_DIR / vid_id)
    os.makedirs(frame_dir, exist_ok=True)
    ffmpeg_cmd = f'ffmpeg -y -i "{output}" -vf "fps=2" "{frame_dir}/frame_%04d.png"'
    os.system(f'{ffmpeg_cmd} 2>nul' if sys.platform == 'win32' else f'{ffmpeg_cmd} 2>/dev/null')
    frame_count = len([f for f in os.listdir(frame_dir) if f.endswith('.png')])
    log(f"  Extracted {frame_count} frames")
    
    record = {
        "id": vid_id, "url": url, "output": output, "frame_dir": frame_dir,
        "frame_count": frame_count, "duration": vid_info.get('duration', 0),
        "resolution": f"{vid_info.get('width', 0)}x{vid_info.get('height', 0)}",
        "category": category, "captured_at": datetime.now().isoformat(),
        "uploaded": False,
    }
    index = load_index()
    index["videos"].append(record)
    save_index(index)
    
    return record


def upload_all():
    """SCP all un-uploaded videos and frames to the dev server."""
    index = load_index()
    to_upload = [v for v in index["videos"] if not v.get("uploaded")]
    
    if not to_upload:
        log("All videos already uploaded.")
        return
    
    log(f"Uploading {len(to_upload)} videos to {DEV_HOST}...")
    
    # Create remote directory
    os.system(f'ssh {DEV_USER}@{DEV_HOST} "mkdir -p {DEV_PATH}/videos {DEV_PATH}/frames"')
    
    for v in to_upload:
        vid_file = v["output"]
        vid_id = v["id"]
        frame_dir = v.get("frame_dir", "")
        
        if not os.path.exists(vid_file):
            log(f"  SKIP {vid_id}: file missing")
            continue
        
        log(f"  Uploading {vid_id}...")
        
        # Upload video
        ret = os.system(f'scp "{vid_file}" {DEV_USER}@{DEV_HOST}:{DEV_PATH}/videos/{vid_id}.webm')
        if ret != 0:
            log(f"  FAILED: {vid_id}")
            continue
        
        # Upload frames
        if frame_dir and os.path.exists(frame_dir):
            os.system(f'ssh {DEV_USER}@{DEV_HOST} "mkdir -p {DEV_PATH}/frames/{vid_id}"')
            os.system(f'scp "{frame_dir}"/*.png {DEV_USER}@{DEV_HOST}:{DEV_PATH}/frames/{vid_id}/')
        
        v["uploaded"] = True
        v["uploaded_at"] = datetime.now().isoformat()
        log(f"  Done: {vid_id}")
    
    # Upload index
    idx_path = str(LIBRARY_DIR / "_index_upload.json")
    with open(idx_path, "w") as f:
        json.dump(index, f, indent=2)
    os.system(f'scp "{idx_path}" {DEV_USER}@{DEV_HOST}:{DEV_PATH}/index.json')
    os.remove(idx_path)
    
    save_index(index)
    uploaded_count = sum(1 for v in to_upload if v.get("uploaded"))
    log(f"Upload complete: {uploaded_count}/{len(to_upload)} videos")


def run_auto_build(categories=None, dry_run=False, search_only=False):
    """Main automated pipeline: search → pick best → capture → upload."""
    ensure_dirs()
    index = load_index()
    
    # Sort by priority
    items = sorted(SEARCH_LIBRARY.items(), key=lambda x: x[1].get("priority", 99))
    
    if categories:
        items = [(k, v) for k, v in items if k in categories]
    
    total = len(items)
    log(f"Motion Reference Library Builder")
    log(f"Categories: {total}")
    log(f"Library: {LIBRARY_DIR}")
    if dry_run:
        log("DRY RUN — no captures will be made")
    log("")
    
    captured_count = 0
    
    for i, (category, config) in enumerate(items):
        desc = config["description"]
        searches = config["searches"]
        max_dur = config.get("max_duration", 300)
        priority = config.get("priority", 99)
        
        log(f"\n{'='*50}")
        log(f"[{i+1}/{total}] {category} (priority {priority})")
        log(f"  {desc}")
        
        if already_captured(index, category):
            log(f"  SKIP — already have a video for this category")
            continue
        
        # Search with each query until we find a good result
        best_result = None
        for query in searches:
            if dry_run:
                log(f"  Would search: {query}")
                continue
            
            results = search_youtube(query, max_results=5)
            
            if results:
                best = pick_best_result(results, max_dur)
                if best:
                    best_result = best
                    best_result['_query'] = query
                    log(f"  Best match: {best['title']} ({best.get('duration', '?')})")
                    break
            
            # Delay between searches
            time.sleep(random.uniform(3, 6))
        
        if dry_run:
            continue
        
        if search_only:
            if best_result:
                log(f"  URL: {best_result['url']}")
            continue
        
        if not best_result:
            log(f"  No suitable video found for {category}")
            continue
        
        # Capture
        delay = random.uniform(5, 15)
        log(f"  Waiting {delay:.0f}s before capture...")
        time.sleep(delay)
        
        record = capture_video(best_result['url'], category, max_dur)
        if record:
            record['search_query'] = best_result.get('_query', '')
            record['video_title'] = best_result.get('title', '')
            # Update in index
            idx = load_index()
            for v in idx['videos']:
                if v['id'] == record['id']:
                    v['search_query'] = record['search_query']
                    v['video_title'] = record['video_title']
            save_index(idx)
            
            captured_count += 1
            log(f"  Captured: {category} -> {record['id']}")
        
        # Longer delay between captures to avoid detection
        time.sleep(random.uniform(15, 30))
    
    if not dry_run and not search_only and captured_count > 0:
        log(f"\n{'='*50}")
        log(f"Captured {captured_count} videos. Uploading to dev server...")
        upload_all()
    
    log(f"\nDone. Total videos in library: {len(load_index().get('videos', []))}")


def list_library():
    index = load_index()
    videos = index.get("videos", [])
    
    print(f"\nMotion Reference Library: {LIBRARY_DIR}")
    print(f"Videos: {len(videos)}\n")
    
    by_cat = {}
    for v in videos:
        cat = v.get('category', 'uncategorized')
        by_cat.setdefault(cat, []).append(v)
    
    for cat in sorted(by_cat):
        vids = by_cat[cat]
        desc = SEARCH_LIBRARY.get(cat, {}).get("description", "")
        print(f"  {cat} — {desc}")
        for v in vids:
            up = "UPLOADED" if v.get("uploaded") else "local"
            print(f"    {v['id']} | {v.get('duration', '?')}s | {v.get('frame_count', '?')} frames | {up}")
            print(f"      {v.get('video_title', v['url'])}")
        print()
    
    # Show missing categories
    missing = [k for k in SEARCH_LIBRARY if k not in by_cat]
    if missing:
        print(f"  Missing categories ({len(missing)}):")
        for m in missing:
            print(f"    {m}: {SEARCH_LIBRARY[m]['description']}")


def main():
    parser = argparse.ArgumentParser(
        description="Shattered Arcana — Motion Reference Library Builder",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--dry-run", action="store_true", help="Show what would be captured without doing it")
    parser.add_argument("--search-only", action="store_true", help="Search and show results but don't capture")
    parser.add_argument("--category", nargs="+", help="Only process specific categories")
    parser.add_argument("--capture", metavar="URL", help="Capture a specific YouTube URL")
    parser.add_argument("--upload", action="store_true", help="Upload all un-uploaded videos")
    parser.add_argument("--list", action="store_true", help="List library contents")
    parser.add_argument("--search", metavar="QUERY", help="Search YouTube")
    
    parser.add_argument("--searches-file", metavar="FILE", help="Path to searches.json (default: ~/motion_capture/library/searches.json)")
    parser.add_argument("--init-searches", action="store_true", help="Create/reset searches.json with defaults")

    args = parser.parse_args()

    # Load search library from file
    global SEARCH_LIBRARY
    if args.searches_file:
        sf = Path(args.searches_file)
        if sf.exists():
            SEARCH_LIBRARY = json.loads(sf.read_text())
            log(f"Loaded {len(SEARCH_LIBRARY)} topics from {sf}")
        else:
            log(f"ERROR: {sf} not found")
            sys.exit(1)
    else:
        SEARCH_LIBRARY = load_search_library()

    if args.init_searches:
        ensure_dirs()
        SEARCHES_FILE.write_text(json.dumps(DEFAULT_SEARCHES, indent=2))
        log(f"Created {SEARCHES_FILE} with {len(DEFAULT_SEARCHES)} default topics")
        log(f"Edit this file to customize search topics.")
        sys.exit(0)
    elif args.list:
        list_library()
    elif args.upload:
        upload_all()
    elif args.capture:
        ensure_dirs()
        capture_video(args.capture)
        upload_all()
    elif args.search:
        search_youtube(args.search)
    else:
        run_auto_build(
            categories=args.category,
            dry_run=args.dry_run,
            search_only=args.search_only,
        )


if __name__ == "__main__":
    main()
