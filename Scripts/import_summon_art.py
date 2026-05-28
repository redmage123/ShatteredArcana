"""Import summoned-creature portraits as UE textures, one per process.

Source: Art/GameAssets/processed/summons/<SpecID>.png
Dest:   /Game/UI/Units/<SpecID>  (matches CoMUnitCardWidget's lookup path)
The PythonScriptPlugin commandlet crashes on teardown, so this imports the
first not-yet-imported file (save=True persists it) and exits; a relaunch loop
drives the full set.
"""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/summons"
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
    break  # one per process
