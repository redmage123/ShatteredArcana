"""Import animation FBX files one at a time, using the already-imported skeleton."""
import unreal
import os

ANIM_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\DenizenAnimations\MixamoAnims"
DEST_PATH = "/Game/Characters/Knight/Animations"
SKELETON_PATH = "/Game/Characters/Knight/SK_Knight_Skeleton.SK_Knight_Skeleton"

# Animation file → clean name mapping (skip skeleton files)
ANIMS = {
    "sword and shield idle.fbx": "Anim_Idle_01",
    "sword and shield idle (2).fbx": "Anim_Idle_02",
    "sword and shield walk.fbx": "Anim_Walk_01",
    "sword and shield walk (2).fbx": "Anim_Walk_02",
    "sword and shield run.fbx": "Anim_Run_01",
    "sword and shield run (2).fbx": "Anim_Run_02",
    "sword and shield attack.fbx": "Anim_Attack_01",
    "sword and shield attack (2).fbx": "Anim_Attack_02",
    "sword and shield attack (3).fbx": "Anim_Attack_03",
    "sword and shield slash.fbx": "Anim_Attack_Slash_01",
    "sword and shield slash (2).fbx": "Anim_Attack_Slash_02",
    "sword and shield slash (3).fbx": "Anim_Attack_Slash_03",
    "sword and shield block.fbx": "Anim_Block_01",
    "sword and shield block idle.fbx": "Anim_Block_Idle",
    "sword and shield death.fbx": "Anim_Death_01",
    "sword and shield death (2).fbx": "Anim_Death_02",
    "sword and shield impact.fbx": "Anim_Hit_01",
    "sword and shield impact (2).fbx": "Anim_Hit_02",
    "sword and shield casting.fbx": "Anim_Cast_01",
    "sword and shield casting (2).fbx": "Anim_Cast_02",
    "sword and shield kick.fbx": "Anim_Kick",
}

count = 0
for filename, asset_name in ANIMS.items():
    filepath = os.path.join(ANIM_DIR, filename)
    if not os.path.exists(filepath):
        unreal.log_warning(f"Not found: {filepath}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST_PATH)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("filename", filepath)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    count += 1
    unreal.log(f"Imported [{count}]: {asset_name}")

unreal.log(f"=== Done: {count} animations imported ===")
