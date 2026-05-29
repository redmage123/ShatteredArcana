"""Import the four missing audio assets at the canonical paths the audio
subsystem's dynamic-load fallback expects. Reuses existing raw .wav source
files already in the repo. One asset per process (the PythonScriptPlugin
commandlet crashes on teardown but save=True persists each first).
"""
import unreal
import os

# (source-file (absolute), dest-folder, dest-asset-name)
JOBS = [
    ("C:/Users/Braun/repos/ShatteredArcana/Content/Audio/SFX/UI/turn_start.wav",
     "/Game/Audio/SFX/UI", "SFX_UI_TurnStart"),
    ("C:/Users/Braun/repos/ShatteredArcana/Content/Audio/SFX/UI/build_complete.wav",
     "/Game/Audio/SFX", "CityFounded"),
    ("C:/Users/Braun/repos/ShatteredArcana/Content/Audio/Music/Menu/menu_theme.wav",
     "/Game/Audio/Music", "MainMenu"),
    ("C:/Users/Braun/repos/ShatteredArcana/Content/Audio/Music/Combat/combat_heavy.wav",
     "/Game/Audio/Music/Combat", "Combat_Intensity_2"),
]

tools = unreal.AssetToolsHelpers.get_asset_tools()

for src, dest, name in JOBS:
    if unreal.EditorAssetLibrary.does_asset_exist(f"{dest}/{name}"):
        continue
    if not os.path.exists(src):
        unreal.log_warning(f"Missing source file: {src}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("filename", src)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    unreal.log(f"Imported {dest}/{name}")
    break  # one per process — relaunch loop drives the rest
