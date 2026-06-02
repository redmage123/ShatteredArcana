"""Import 31 item portraits at /Game/UI/Items/<variant>.
One per process to avoid the PythonScriptPlugin teardown crash truncating
the batch — matches the import pattern of the other art passes.
"""
import unreal
import os

SRC = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/items"
DEST = "/Game/UI/Items"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for f in sorted(os.listdir(SRC)):
    if not f.endswith(".png"):
        continue
    name = f.replace(".png", "")
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
