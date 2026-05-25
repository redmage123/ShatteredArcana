"""Import the global-enchantment tarot cards as UE texture assets.

Run inside the editor (commandlet) e.g.:
  UnrealEditor-Cmd <uproject> -run=pythonscript -script="Scripts/import_enchantment_cards.py"

Source PNGs: Art/GameAssets/processed/enchantments/enchant_<slug>.png
Dest assets: /Game/UI/Enchantments/enchant_<slug>
The CoMGlobalEnchantmentData slug maps to /Game/UI/Enchantments/enchant_<slug>.
"""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/enchantments"
DEST = "/Game/UI/Enchantments"

tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0

for f in sorted(os.listdir(SRC_DIR)):
    if not f.endswith(".png"):
        continue
    name = f.replace(".png", "")

    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}"):
        unreal.log(f"  Skip (exists): {name}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", os.path.join(SRC_DIR, f).replace("\\", "/"))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)  # persists before the plugin teardown crash

    try:
        tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"  Imported: {name}")
    except Exception as e:
        unreal.log_warning(f"  Failed: {name} - {e}")
        continue

    # Import one per process: the PythonScriptPlugin commandlet crashes on
    # teardown after importing, so a relaunch loop (see import loop) drives the
    # full set in one-asset-per-process steps. Stop after the first new import.
    break

unreal.log(f"Done - imported {count} enchantment cards to {DEST}")
