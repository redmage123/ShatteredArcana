"""Import 8 plane-themed overworld music loops to /Game/Audio/Music/<plane>.
The audio subsystem PlayMusic dynamic-loads from there.

One per process to avoid the PythonScriptPlugin teardown crash.
"""
import unreal
import os

SRC = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/plane_music"
DEST = "/Game/Audio/Music"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for f in sorted(os.listdir(SRC)):
    if not f.endswith(".wav"):
        continue
    name = f.replace(".wav", "")  # e.g. plane_aurelith
    if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", os.path.join(SRC, f).replace("\\", "/"))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    unreal.log(f"Imported {name}")
    break
