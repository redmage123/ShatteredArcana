"""Import all wizard art as UE texture assets — one at a time to avoid Slate crash."""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/wizards"
DEST = "/Game/Textures/Wizards"

tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0

for f in sorted(os.listdir(SRC_DIR)):
    if not f.endswith(".png"):
        continue
    name = f.replace(".png", "")

    # Check if already imported
    asset_path = f"{DEST}/{name}.{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}"):
        unreal.log(f"  Skip (exists): {name}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", os.path.join(SRC_DIR, f).replace("\\", "/"))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    try:
        tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"  Imported: {name}")
    except Exception as e:
        unreal.log_warning(f"  Failed: {name} — {e}")

unreal.log(f"Done — imported {count} wizard textures to {DEST}")
