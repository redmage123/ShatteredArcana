"""Import all spell-VFX sprite sheets into UE.

Source: Art/GameAssets/processed/spell_vfx/<realm>/<name>_sheet.png
Asset:  /Game/Textures/SpellVFX/<realm>/<name>_sheet
"""
import os
import unreal

SRC_ROOT = "C:/Users/Braun/repos/ShatteredArcana/Art/GameAssets/processed/spell_vfx"
DEST_ROOT = "/Game/Textures/SpellVFX"

tools = unreal.AssetToolsHelpers.get_asset_tools()
count = 0
skipped = 0

for realm in sorted(os.listdir(SRC_ROOT)):
    realm_dir = os.path.join(SRC_ROOT, realm)
    if not os.path.isdir(realm_dir):
        continue
    dest_dir = f"{DEST_ROOT}/{realm}"
    for f in sorted(os.listdir(realm_dir)):
        if not f.endswith("_sheet.png"):
            continue
        name = f[:-4]
        asset_pkg = f"{dest_dir}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(asset_pkg):
            skipped += 1
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", dest_dir)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("filename",
            os.path.join(realm_dir, f).replace("\\", "/"))
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        try:
            tools.import_asset_tasks([task])
            count += 1
            unreal.log(f"  Imported: {realm}/{name}")
        except Exception as e:
            unreal.log_warning(f"  Failed: {realm}/{name} - {e}")

unreal.log(f"Spell VFX import complete: {count} imported, {skipped} skipped")
