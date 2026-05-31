"""Import the city-panorama art set into /Game/Textures/CityView/.

Per-process relaunch pattern: imports one PNG per process so the UE
PythonScriptPlugin teardown doesn't truncate the batch.
"""
import unreal
import os

SPRITES = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/cityview/sprites"
BG      = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/cityview/backgrounds"
DEST    = "/Game/Textures/CityView"

tools = unreal.AssetToolsHelpers.get_asset_tools()


def try_import(src_dir):
    for f in sorted(os.listdir(src_dir)):
        if not f.endswith(".png"):
            continue
        name = f.replace(".png", "")
        if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{name}"):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("filename", os.path.join(src_dir, f).replace("\\", "/"))
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tools.import_asset_tasks([task])
        unreal.log(f"Imported {name}")
        return True
    return False


if not try_import(SPRITES):
    try_import(BG)
