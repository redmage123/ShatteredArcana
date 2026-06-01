"""Import the 8 combat SFX WAVs to /Game/Audio/SFX/UI/. The audio subsystem
PlayUISound path looks there by canonical FName, so dropping them in that
folder is enough for the tactical widget hooks to find them.

One per process to avoid the PythonScriptPlugin teardown crash.
"""
import unreal
import os

SRC = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/combat_sfx"
DEST = "/Game/Audio/SFX/UI"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for f in sorted(os.listdir(SRC)):
    if not f.endswith(".wav"):
        continue
    name = f.replace(".wav", "")
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
