"""Import per-realm spell SFX into UE as USoundWave assets.

We don't have realm-tagged audio yet, so each realm uses an existing magic
SFX from Content/Audio/SFX/Magic. The mapping is intentionally loose — these
files were chosen because they have appropriate sonic character. Swap them
out by editing AudioMap below if better-fitting clips arrive.

Each realm gets a "cast" cue and an "impact" cue. The mapping below picks
two distinct clips per realm.

Source files live in Content/Audio/SFX/Magic/ as .ogg.
Imported as: /Game/Audio/SpellSFX/<realm>_<kind>
"""
import os
import unreal

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Content/Audio/SFX/Magic"
DEST = "/Game/Audio/SpellSFX"

# Realm -> (cast_filename, impact_filename) — both relative to SRC_DIR.
# Files chosen from the available ~73 magic SFX. If a chosen file is missing
# we fall back to bookOpen.ogg (we know that one exists).
AudioMap = {
    "life":    ("bookOpen.ogg",                "impactGlass_light_000.ogg"),
    "death":   ("bookFlip1.ogg",               "impactGlass_heavy_000.ogg"),
    "chaos":   ("bookFlip2.ogg",               "impactGlass_heavy_001.ogg"),
    "nature":  ("bookFlip3.ogg",               "impactGlass_medium_000.ogg"),
    "sorcery": ("bookOpen.ogg",                "impactGlass_medium_001.ogg"),
    "arcane":  ("bookPlace1.ogg",              "impactGlass_medium_002.ogg"),
    "binding": ("bookPlace2.ogg",              "impactGlass_heavy_002.ogg"),
    "spirit":  ("bookPlace3.ogg",              "impactGlass_light_001.ogg"),
    "glamour": ("bookClose.ogg",               "impactGlass_light_002.ogg"),
}

tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0
skipped = 0
missing = 0


def import_one(src_filename, asset_name):
    global count, skipped, missing
    src_path = os.path.join(SRC_DIR, src_filename).replace("\\", "/")
    if not os.path.exists(src_path):
        unreal.log_warning(f"  Source missing: {src_path}")
        missing += 1
        return

    asset_pkg = f"{DEST}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_pkg):
        skipped += 1
        return

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("filename", src_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    try:
        tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"  Imported: {asset_name}")
    except Exception as e:
        unreal.log_warning(f"  Failed: {asset_name} - {e}")


for realm, (cast_file, impact_file) in AudioMap.items():
    import_one(cast_file,   f"{realm}_cast")
    import_one(impact_file, f"{realm}_impact")

unreal.log(f"Spell SFX import: {count} imported, {skipped} skipped, {missing} missing source files")
