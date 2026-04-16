"""Import all spell book icons as UE texture assets."""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/ui/books"
DEST = "/Game/Textures/UI/Books"

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
    task.set_editor_property("save", True)

    try:
        tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"  Imported: {name}")
    except Exception as e:
        unreal.log_warning(f"  Failed: {name} - {e}")

unreal.log(f"Done - imported {count} book textures to {DEST}")
