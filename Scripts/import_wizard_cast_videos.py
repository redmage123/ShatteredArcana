"""Import 14 wizard cast .mp4 clips as UFileMediaSource assets.

Source:  Content/Movies/WizardCast/raw/wizard_NN.mp4
Asset:   /Game/Movies/WizardCast/wizard_cast_NN  (UFileMediaSource)
"""
import os
import unreal

SRC_DIR = "C:/Users/Braun/repos/ShatteredArcana/Content/Movies/WizardCast/raw"
DEST_PKG = "/Game/Movies/WizardCast"
# Where UE will look for the file at runtime. Use a project-relative path so it
# works in both editor and packaged builds.
RUNTIME_REL = "Content/Movies/WizardCast/raw"

tools = unreal.AssetToolsHelpers.get_asset_tools()

for i in range(1, 15):
    src_filename = f"wizard_{i:02d}.mp4"
    src_path = os.path.join(SRC_DIR, src_filename)
    if not os.path.exists(src_path):
        unreal.log_warning(f"  Missing: {src_path}")
        continue

    asset_name = f"wizard_cast_{i:02d}"
    asset_pkg = f"{DEST_PKG}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_pkg):
        unreal.log(f"  Skip (exists): {asset_name}")
        continue

    media_src = tools.create_asset(asset_name, DEST_PKG,
        unreal.FileMediaSource, None)
    if not media_src:
        unreal.log_warning(f"  Failed to create asset: {asset_name}")
        continue

    # Use a project-relative file path so it resolves in packaged builds too.
    media_src.set_editor_property("file_path",
        f"./{RUNTIME_REL}/{src_filename}")
    unreal.EditorAssetLibrary.save_loaded_asset(media_src)
    unreal.log(f"  Created: {asset_name} -> {RUNTIME_REL}/{src_filename}")

unreal.log("Wizard cast import complete.")
