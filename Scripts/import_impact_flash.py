"""Import 9 per-realm impact_flash sheet textures to
/Game/Textures/SpellVFX/<realm>/impact_flash_sheet. One per process to avoid
the PythonScriptPlugin teardown crash truncating the batch.
"""
import unreal
import os

SRC = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/impact_flash"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for f in sorted(os.listdir(SRC)):
    if not f.endswith(".png"):
        continue
    realm = f.replace("_impact_flash_sheet.png", "")
    dest = f"/Game/Textures/SpellVFX/{realm}"
    name = "impact_flash_sheet"
    if unreal.EditorAssetLibrary.does_asset_exist(f"{dest}/{name}"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", os.path.join(SRC, f).replace("\\", "/"))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    unreal.log(f"Imported {realm}/{name}")
    break
