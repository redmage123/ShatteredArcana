"""
Automate AIVA music generation via Playwright.
Uses persistent browser profile so login state is preserved.
Launches visible browser, auto-detects login, generates tracks.
"""

import os
import time
import glob
from playwright.sync_api import sync_playwright

OUTPUT_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Content\Audio\Music\Menu"
USER_DATA = r"C:\Users\Braun\AppData\Local\aiva_playwright_profile"
AIVA_URL = "https://creators.aiva.ai"
SCREENSHOT_DIR = OUTPUT_DIR

def screenshot(page, name):
    path = os.path.join(SCREENSHOT_DIR, f"aiva_{name}.png")
    page.screenshot(path=path)
    return path

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    with sync_playwright() as p:
        # Use persistent context so login cookies are saved between runs
        context = p.chromium.launch_persistent_context(
            user_data_dir=USER_DATA,
            headless=False,
            accept_downloads=True,
            downloads_path=OUTPUT_DIR,
            viewport={"width": 1400, "height": 900},
        )
        page = context.pages[0] if context.pages else context.new_page()

        print("=== Opening AIVA ===")
        page.goto(AIVA_URL, wait_until="networkidle", timeout=60000)
        time.sleep(3)

        # Check if we need to log in
        url = page.url.lower()
        print(f"  Current URL: {page.url}")
        screenshot(page, "01_initial")

        if "login" in url or "sign" in url or "auth" in url:
            print("\n  *** AIVA requires login. ***")
            print("  *** Please log in in the browser window that just opened. ***")
            print("  *** The script will detect when you're logged in and continue automatically. ***\n")

            # Poll for login completion (max 5 minutes)
            for attempt in range(60):
                time.sleep(5)
                current_url = page.url.lower()
                if "login" not in current_url and "sign" not in current_url and "auth" not in current_url:
                    print(f"  Login detected! URL: {page.url}")
                    break
                if attempt % 6 == 0:
                    print(f"  Still waiting for login... ({attempt * 5}s)")
            else:
                print("  Timeout waiting for login. Continuing anyway...")

            time.sleep(3)

        screenshot(page, "02_dashboard")
        print(f"  Dashboard URL: {page.url}")

        # Explore the page to find the creation flow
        print("\n=== Exploring AIVA interface ===")

        # Look for Create / New / Compose buttons
        for selector in [
            'button:has-text("Create")',
            'button:has-text("New")',
            'button:has-text("Compose")',
            'a:has-text("Create")',
            'a:has-text("New")',
            '[class*="create"]',
            '[data-testid*="create"]',
        ]:
            try:
                el = page.locator(selector).first
                if el.is_visible(timeout=1000):
                    print(f"  Found: {selector} -> clicking")
                    el.click()
                    time.sleep(3)
                    screenshot(page, "03_after_create_click")
                    break
            except:
                pass

        # Look for style/genre selection
        print("\n  Looking for style selection...")
        for style_term in ["Epic", "Orchestral", "Cinematic", "Fantasy", "Symphon"]:
            try:
                style_el = page.locator(f'text="{style_term}"').first
                if style_el.is_visible(timeout=1000):
                    print(f"  Found style: {style_term}")
                    style_el.click()
                    time.sleep(2)
                    screenshot(page, f"04_style_{style_term.lower()}")
                    break
            except:
                pass

        # Take final screenshot showing current state
        screenshot(page, "05_current_state")

        # List all interactive elements
        print("\n  === Interactive elements ===")
        for tag in ["button", "a", "input", "select"]:
            elements = page.locator(tag).all()
            visible = [e for e in elements if e.is_visible()]
            if visible:
                print(f"  {tag}: {len(visible)} visible")
                for e in visible[:10]:
                    try:
                        txt = e.inner_text().strip()[:60]
                        if txt:
                            print(f"    - {txt}")
                    except:
                        pass

        print("\n  === Keeping browser open for 5 minutes ===")
        print("  === Screenshots saved to Menu folder ===")

        # Keep alive so user can interact if needed
        time.sleep(300)

        context.close()
        print("\n=== Done ===")


if __name__ == "__main__":
    main()
