#!/usr/bin/env python3
"""
Asset Processing Pipeline for Shattered Arcana
================================================
Walks all PNGs in raw/, processes them, and writes optimized versions to processed/.
Generates an asset_manifest.json with statistics.

Usage:
    python3 process_assets.py
"""

import json
import os
import sys
import time
from pathlib import Path

try:
    from PIL import Image, PngImagePlugin
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BASE_DIR = Path(__file__).resolve().parent
RAW_DIR = BASE_DIR / "raw"
PROCESSED_DIR = BASE_DIR / "processed"
MANIFEST_PATH = BASE_DIR / "asset_manifest.json"

# Maximum dimensions per category
SIZE_CAPS = {
    "terrain": (128, 128),
    "heroes": (256, 256),
    # Everything else defaults to 512x512
}
DEFAULT_SIZE_CAP = (512, 512)

# Known asset categories (top-level directories under raw/)
CATEGORIES = [
    "terrain", "units", "buildings", "heroes",
    "spells", "map_features", "ui",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_category(rel_path: str) -> str:
    """Return the top-level category from a relative path."""
    top = rel_path.split(os.sep)[0] if os.sep in rel_path else rel_path.split("/")[0]
    return top if top in CATEGORIES else "other"


def has_transparency(img: Image.Image) -> bool:
    """Check whether an image actually uses transparency."""
    if img.mode == "RGBA":
        alpha = img.getchannel("A")
        extrema = alpha.getextrema()
        # If the minimum alpha is 255, there is no transparency
        return extrema[0] < 255
    if img.mode == "P":
        return "transparency" in img.info
    return False


def can_reduce_palette(img: Image.Image, threshold: int = 256) -> bool:
    """Check if the image uses fewer than `threshold` unique colors."""
    # Only practical for smaller images; skip very large ones to save time.
    if img.width * img.height > 512 * 512:
        return False
    try:
        colors = img.getcolors(maxcolors=threshold)
        return colors is not None
    except Exception:
        return False


def process_image(src: Path, dst: Path, category: str) -> dict:
    """
    Process a single PNG image.

    Returns a dict with keys:
        ok (bool), warning (str|None),
        original_size (int), processed_size (int),
        resized (bool), mode_changed (bool), palette_reduced (bool)
    """
    result = {
        "ok": True,
        "warning": None,
        "original_size": src.stat().st_size,
        "processed_size": 0,
        "resized": False,
        "mode_changed": False,
        "palette_reduced": False,
    }

    try:
        img = Image.open(src)
        img.load()  # Force full decode — catches truncated files
    except Exception as exc:
        result["ok"] = False
        result["warning"] = f"Corrupt/unreadable PNG: {exc}"
        return result

    # ---- Determine target size cap ----
    max_w, max_h = SIZE_CAPS.get(category, DEFAULT_SIZE_CAP)

    # ---- Resize if oversized ----
    if img.width > max_w or img.height > max_h:
        img.thumbnail((max_w, max_h), Image.LANCZOS)
        result["resized"] = True

    # ---- Normalize color mode ----
    original_mode = img.mode

    if has_transparency(img):
        if img.mode != "RGBA":
            img = img.convert("RGBA")
            result["mode_changed"] = True
    else:
        if img.mode == "RGBA":
            img = img.convert("RGB")
            result["mode_changed"] = True
        elif img.mode not in ("RGB", "P", "L"):
            img = img.convert("RGB")
            result["mode_changed"] = True

    # ---- Palette reduction (if < 256 colors) ----
    if img.mode == "RGB" and can_reduce_palette(img):
        try:
            img = img.convert("P", palette=Image.ADAPTIVE, colors=256)
            result["palette_reduced"] = True
        except Exception:
            pass  # Keep as RGB if conversion fails

    # ---- Write optimized PNG ----
    dst.parent.mkdir(parents=True, exist_ok=True)

    save_kwargs = {
        "optimize": True,
    }

    # Strip metadata by not passing any PngInfo
    # (Pillow's default is to carry over text chunks — we explicitly pass empty info)
    save_kwargs["pnginfo"] = PngImagePlugin.PngInfo()

    img.save(dst, format="PNG", **save_kwargs)
    result["processed_size"] = dst.stat().st_size

    return result


# ---------------------------------------------------------------------------
# Main pipeline
# ---------------------------------------------------------------------------

def main():
    print("=" * 65)
    print("  Shattered Arcana — Asset Processing Pipeline")
    print("=" * 65)
    print(f"  Raw dir:       {RAW_DIR}")
    print(f"  Processed dir: {PROCESSED_DIR}")
    print()

    if not RAW_DIR.is_dir():
        print(f"ERROR: raw directory not found: {RAW_DIR}")
        sys.exit(1)

    PROCESSED_DIR.mkdir(parents=True, exist_ok=True)

    # Collect all PNGs
    png_files = sorted(RAW_DIR.rglob("*.png"))
    total = len(png_files)
    print(f"  Found {total} PNG files to process.\n")

    if total == 0:
        print("Nothing to do.")
        return

    # Tracking
    stats = {
        "total_files": total,
        "processed_ok": 0,
        "errors": 0,
        "resized": 0,
        "mode_changed": 0,
        "palette_reduced": 0,
        "total_raw_bytes": 0,
        "total_processed_bytes": 0,
        "per_category": {cat: {"count": 0, "raw_bytes": 0, "processed_bytes": 0} for cat in CATEGORIES + ["other"]},
    }
    warnings_list = []

    start = time.time()

    for idx, src in enumerate(png_files, 1):
        rel = src.relative_to(RAW_DIR)
        dst = PROCESSED_DIR / rel
        category = get_category(str(rel))

        result = process_image(src, dst, category)

        stats["total_raw_bytes"] += result["original_size"]
        cat_stats = stats["per_category"][category]
        cat_stats["count"] += 1
        cat_stats["raw_bytes"] += result["original_size"]

        if result["ok"]:
            stats["processed_ok"] += 1
            stats["total_processed_bytes"] += result["processed_size"]
            cat_stats["processed_bytes"] += result["processed_size"]
            if result["resized"]:
                stats["resized"] += 1
            if result["mode_changed"]:
                stats["mode_changed"] += 1
            if result["palette_reduced"]:
                stats["palette_reduced"] += 1
        else:
            stats["errors"] += 1
            warning_entry = {"file": str(rel), "message": result["warning"]}
            warnings_list.append(warning_entry)
            print(f"  WARNING [{rel}]: {result['warning']}")

        # Progress indicator every 100 files
        if idx % 100 == 0 or idx == total:
            elapsed = time.time() - start
            rate = idx / elapsed if elapsed > 0 else 0
            print(f"  [{idx:>5}/{total}]  {rate:.0f} files/sec  ({category}: {rel.name})")

    elapsed = time.time() - start

    # ---- Build manifest ----
    manifest = {
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "total_files": stats["total_files"],
        "processed_ok": stats["processed_ok"],
        "errors": stats["errors"],
        "total_raw_bytes": stats["total_raw_bytes"],
        "total_processed_bytes": stats["total_processed_bytes"],
        "savings_pct": round(
            (1 - stats["total_processed_bytes"] / stats["total_raw_bytes"]) * 100, 2
        ) if stats["total_raw_bytes"] > 0 else 0,
        "per_category": {},
        "warnings": warnings_list,
    }

    for cat in CATEGORIES + ["other"]:
        cs = stats["per_category"][cat]
        if cs["count"] > 0:
            manifest["per_category"][cat] = {
                "count": cs["count"],
                "raw_bytes": cs["raw_bytes"],
                "processed_bytes": cs["processed_bytes"],
                "savings_pct": round(
                    (1 - cs["processed_bytes"] / cs["raw_bytes"]) * 100, 2
                ) if cs["raw_bytes"] > 0 else 0,
            }

    with open(MANIFEST_PATH, "w") as f:
        json.dump(manifest, f, indent=2)

    # ---- Print summary ----
    def fmt_bytes(n):
        if n < 1024:
            return f"{n} B"
        elif n < 1024 * 1024:
            return f"{n / 1024:.1f} KB"
        else:
            return f"{n / (1024 * 1024):.2f} MB"

    print()
    print("=" * 65)
    print("  PROCESSING COMPLETE")
    print("=" * 65)
    print(f"  Total files:      {stats['total_files']}")
    print(f"  Processed OK:     {stats['processed_ok']}")
    print(f"  Errors:           {stats['errors']}")
    print(f"  Resized:          {stats['resized']}")
    print(f"  Mode changed:     {stats['mode_changed']}")
    print(f"  Palette reduced:  {stats['palette_reduced']}")
    print(f"  Raw size:         {fmt_bytes(stats['total_raw_bytes'])}")
    print(f"  Processed size:   {fmt_bytes(stats['total_processed_bytes'])}")
    print(f"  Savings:          {manifest['savings_pct']}%")
    print(f"  Time:             {elapsed:.1f}s")
    print()
    print("  Per-category breakdown:")
    for cat, data in manifest["per_category"].items():
        print(f"    {cat:<16} {data['count']:>5} files   "
              f"{fmt_bytes(data['raw_bytes']):>10} -> {fmt_bytes(data['processed_bytes']):>10}  "
              f"({data['savings_pct']}% saved)")
    print()
    print(f"  Manifest written to: {MANIFEST_PATH}")
    if warnings_list:
        print(f"  {len(warnings_list)} warning(s) — see manifest for details.")
    print("=" * 65)


if __name__ == "__main__":
    main()
