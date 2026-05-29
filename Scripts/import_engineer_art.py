"""Import the 4 engineer portraits at /Game/UI/Units/<SpecID> (one per process,
relaunch loop drives the rest — same pattern as the summon-art importer)."""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/engineers"
DEST = "/Game/UI/Units"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for f in sorted(os.listdir(SRC_DIR)):
    if not f.endswith(".png"):
        continue
    name = f.replace(".png", "")
    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}"):
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", os.path.join(SRC_DIR, f).replace("\\", "/"))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    unreal.log(f"Imported {name}")
    break
