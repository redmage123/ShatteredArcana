"""
Run this script FROM WITHIN the UE Editor (Edit > Execute Python Script).
It batch-imports all Mixamo FBX animation files targeting the SK_Knight skeleton.

Prerequisites: SK_Knight must already be imported at /Game/Characters/Knight/
"""

import unreal
import os

ANIM_DIR = "C:/Users/Braun/repos/ShatteredArcana/Art/DenizenAnimations/MixamoAnims"
DEST = "/Game/Characters/Knight/Animations"
SKELETON = "/Game/Characters/Knight/SK_Knight_Skeleton.SK_Knight_Skeleton"

# Load the target skeleton
skeleton = unreal.load_asset(SKELETON)
if not skeleton:
    unreal.log_error(f"Could not load skeleton at {SKELETON}")
else:
    unreal.log(f"Target skeleton: {skeleton.get_name()}")

ANIMS = {
    "sword and shield idle.fbx": "Anim_Idle_01",
    "sword and shield idle (2).fbx": "Anim_Idle_02",
    "sword and shield idle (3).fbx": "Anim_Idle_03",
    "sword and shield idle (4).fbx": "Anim_Idle_04",
    "sword and shield walk.fbx": "Anim_Walk_01",
    "sword and shield walk (2).fbx": "Anim_Walk_02",
    "sword and shield run.fbx": "Anim_Run_01",
    "sword and shield run (2).fbx": "Anim_Run_02",
    "sword and shield attack.fbx": "Anim_Attack_01",
    "sword and shield attack (2).fbx": "Anim_Attack_02",
    "sword and shield attack (3).fbx": "Anim_Attack_03",
    "sword and shield attack (4).fbx": "Anim_Attack_04",
    "sword and shield slash.fbx": "Anim_Attack_Slash_01",
    "sword and shield slash (2).fbx": "Anim_Attack_Slash_02",
    "sword and shield slash (3).fbx": "Anim_Attack_Slash_03",
    "sword and shield slash (4).fbx": "Anim_Attack_Slash_04",
    "sword and shield slash (5).fbx": "Anim_Attack_Slash_05",
    "sword and shield block.fbx": "Anim_Block_01",
    "sword and shield block (2).fbx": "Anim_Block_02",
    "sword and shield block idle.fbx": "Anim_Block_Idle",
    "sword and shield death.fbx": "Anim_Death_01",
    "sword and shield death (2).fbx": "Anim_Death_02",
    "sword and shield impact.fbx": "Anim_Hit_01",
    "sword and shield impact (2).fbx": "Anim_Hit_02",
    "sword and shield impact (3).fbx": "Anim_Hit_03",
    "sword and shield casting.fbx": "Anim_Cast_01",
    "sword and shield casting (2).fbx": "Anim_Cast_02",
    "sword and shield turn.fbx": "Anim_Turn_01",
    "sword and shield turn (2).fbx": "Anim_Turn_02",
    "sword and shield 180 turn.fbx": "Anim_Turn_180_01",
    "sword and shield 180 turn (2).fbx": "Anim_Turn_180_02",
    "sword and shield strafe.fbx": "Anim_Strafe_01",
    "sword and shield strafe (2).fbx": "Anim_Strafe_02",
    "sword and shield strafe (3).fbx": "Anim_Strafe_03",
    "sword and shield strafe (4).fbx": "Anim_Strafe_04",
    "sword and shield crouch.fbx": "Anim_Crouch_Start",
    "sword and shield crouch idle.fbx": "Anim_Crouch_Idle",
    "sword and shield crouch block.fbx": "Anim_Crouch_Block_01",
    "sword and shield crouch block (2).fbx": "Anim_Crouch_Block_02",
    "sword and shield crouch block idle.fbx": "Anim_Crouch_Block_Idle",
    "sword and shield jump.fbx": "Anim_Jump_01",
    "sword and shield jump (2).fbx": "Anim_Jump_02",
    "sword and shield kick.fbx": "Anim_Kick",
    "sword and shield power up.fbx": "Anim_PowerUp",
    "sheath sword 1.fbx": "Anim_Sheath_01",
    "sheath sword 2.fbx": "Anim_Sheath_02",
}

imported = 0
failed = 0

for filename, asset_name in ANIMS.items():
    filepath = os.path.join(ANIM_DIR, filename)
    if not os.path.exists(filepath):
        unreal.log_warning(f"Not found: {filename}")
        failed += 1
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("filename", filepath)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    # Set FBX import options to import as animation only
    fbx_options = unreal.FbxImportUI()
    fbx_options.set_editor_property("import_mesh", False)
    fbx_options.set_editor_property("import_materials", False)
    fbx_options.set_editor_property("import_textures", False)
    fbx_options.set_editor_property("import_animations", True)
    fbx_options.set_editor_property("skeleton", skeleton)
    fbx_options.skeleton_import_data = unreal.FbxSkeletalMeshImportData()
    fbx_options.anim_sequence_import_data = unreal.FbxAnimSequenceImportData()

    task.set_editor_property("options", fbx_options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported += 1
    unreal.log(f"[{imported}] Imported: {asset_name}")

unreal.log(f"=== DONE: {imported} imported, {failed} failed ===")
