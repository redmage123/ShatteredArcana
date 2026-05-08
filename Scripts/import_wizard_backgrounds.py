"""Import 14 SDXL wizard combat backgrounds as UE texture assets.

Source PNGs:  Content/Textures/Wizards/Backgrounds/raw/wizard_NN.png
Asset path:   /Game/Textures/Wizards/Backgrounds/wizard_bg_NN
"""
import unreal
import os

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Content/Textures/Wizards/Backgrounds/raw"
DEST = "/Game/Textures/Wizards/Backgrounds"

tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0

for i in range(1, 15):
    src_name = f"wizard_{i:02d}.png"
    src_path = os.path.join(SRC_DIR, src_name).replace("\\", "/")
    dest_name = f"wizard_bg_{i:02d}"

    if not os.path.exists(src_path):
        unreal.log_warning(f"  Missing source: {src_path}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("filename", src_path)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    try:
        tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"  Imported: {dest_name}")
    except Exception as e:
        unreal.log_warning(f"  Failed: {dest_name} - {e}")

unreal.log(f"Done - imported {count} wizard backgrounds to {DEST}")
