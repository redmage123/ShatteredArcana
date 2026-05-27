"""Import the open-book spellbook background as a UE texture.

Maps the chosen variant (spellbook_bg_alt.png) to the asset
/Game/UI/SpellBook/spellbook_bg referenced by CoMSpellBookWidget.
save=True persists the asset before the PythonScriptPlugin teardown crash.
"""
import unreal
import os

SRC = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/ui/spellbook_bg_alt.png"
DEST = "/Game/UI/SpellBook"
NAME = "spellbook_bg"

tools = unreal.AssetToolsHelpers.get_asset_tools()

if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST}/{NAME}"):
    unreal.log(f"Skip (exists): {NAME}")
else:
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", NAME)
    task.set_editor_property("filename", SRC)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    unreal.log(f"Imported {NAME}")
