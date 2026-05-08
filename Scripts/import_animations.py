"""
Import Mixamo FBX animations into UE5 Content directory.
Maps the 52 FBX files to named animation categories for the game's Animation Blueprint.

Animation categories:
- Idle (4 variants)
- Walk (2 variants)
- Run (2 variants)
- Attack/Slash (5 variants)
- Block (3 variants + idle)
- Death (2 variants)
- Impact/Hit (3 variants)
- Cast/Spell (2 variants)
- Turn (2 variants + 180 turn)
- Strafe (4 variants)
- Crouch (3 variants + idle + block)
- Jump (2 variants)
- Special (kick, power up, sheath)
- Skeleton mesh (Knight D Pelegrini)
"""

import unreal
import os

ANIM_DIR = r"C:\Users\Braun\repos\ShatteredArcana\Art\DenizenAnimations\MixamoAnims"
DEST_SKELETON = "/Game/Characters/Knight/SK_Knight"
DEST_ANIM_PATH = "/Game/Characters/Knight/Animations"

# Map FBX filenames to clean asset names
ANIM_MAP = {
    # Skeleton mesh
    "Knight D Pelegrini.fbx": ("SK_Knight", True),  # (name, is_skeleton)

    # Idle
    "sword and shield idle.fbx": ("Anim_Idle_01", False),
    "sword and shield idle (2).fbx": ("Anim_Idle_02", False),
    "sword and shield idle (3).fbx": ("Anim_Idle_03", False),
    "sword and shield idle (4).fbx": ("Anim_Idle_04", False),

    # Walk
    "sword and shield walk.fbx": ("Anim_Walk_01", False),
    "sword and shield walk (2).fbx": ("Anim_Walk_02", False),

    # Run
    "sword and shield run.fbx": ("Anim_Run_01", False),
    "sword and shield run (2).fbx": ("Anim_Run_02", False),

    # Attack - slash variants
    "sword and shield slash.fbx": ("Anim_Attack_Slash_01", False),
    "sword and shield slash (2).fbx": ("Anim_Attack_Slash_02", False),
    "sword and shield slash (3).fbx": ("Anim_Attack_Slash_03", False),
    "sword and shield slash (4).fbx": ("Anim_Attack_Slash_04", False),
    "sword and shield slash (5).fbx": ("Anim_Attack_Slash_05", False),
    "sword and shield attack.fbx": ("Anim_Attack_01", False),
    "sword and shield attack (2).fbx": ("Anim_Attack_02", False),
    "sword and shield attack (3).fbx": ("Anim_Attack_03", False),
    "sword and shield attack (4).fbx": ("Anim_Attack_04", False),

    # Block
    "sword and shield block.fbx": ("Anim_Block_01", False),
    "sword and shield block (2).fbx": ("Anim_Block_02", False),
    "sword and shield block idle.fbx": ("Anim_Block_Idle", False),

    # Death
    "sword and shield death.fbx": ("Anim_Death_01", False),
    "sword and shield death (2).fbx": ("Anim_Death_02", False),

    # Impact / Hit reaction
    "sword and shield impact.fbx": ("Anim_Hit_01", False),
    "sword and shield impact (2).fbx": ("Anim_Hit_02", False),
    "sword and shield impact (3).fbx": ("Anim_Hit_03", False),

    # Casting / Spell
    "sword and shield casting.fbx": ("Anim_Cast_01", False),
    "sword and shield casting (2).fbx": ("Anim_Cast_02", False),

    # Turn
    "sword and shield turn.fbx": ("Anim_Turn_01", False),
    "sword and shield turn (2).fbx": ("Anim_Turn_02", False),
    "sword and shield 180 turn.fbx": ("Anim_Turn_180_01", False),
    "sword and shield 180 turn (2).fbx": ("Anim_Turn_180_02", False),

    # Strafe
    "sword and shield strafe.fbx": ("Anim_Strafe_01", False),
    "sword and shield strafe (2).fbx": ("Anim_Strafe_02", False),
    "sword and shield strafe (3).fbx": ("Anim_Strafe_03", False),
    "sword and shield strafe (4).fbx": ("Anim_Strafe_04", False),

    # Crouch
    "sword and shield crouch.fbx": ("Anim_Crouch_Start", False),
    "sword and shield crouch idle.fbx": ("Anim_Crouch_Idle", False),
    "sword and shield crouch block.fbx": ("Anim_Crouch_Block_01", False),
    "sword and shield crouch block (2).fbx": ("Anim_Crouch_Block_02", False),
    "sword and shield crouch block idle.fbx": ("Anim_Crouch_Block_Idle", False),
    "sword and shield crouching.fbx": ("Anim_Crouching_01", False),
    "sword and shield crouching (2).fbx": ("Anim_Crouching_02", False),
    "sword and shield crouching (3).fbx": ("Anim_Crouching_03", False),

    # Jump
    "sword and shield jump.fbx": ("Anim_Jump_01", False),
    "sword and shield jump (2).fbx": ("Anim_Jump_02", False),

    # Special
    "sword and shield kick.fbx": ("Anim_Kick", False),
    "sword and shield power up.fbx": ("Anim_PowerUp", False),
    "sheath sword 1.fbx": ("Anim_Sheath_01", False),
    "sheath sword 2.fbx": ("Anim_Sheath_02", False),
}

def import_fbx():
    """Import all FBX files into the UE Content directory."""

    tasks = []
    skeleton_path = None

    # First pass: import skeleton mesh
    for filename, (asset_name, is_skeleton) in ANIM_MAP.items():
        if not is_skeleton:
            continue

        filepath = os.path.join(ANIM_DIR, filename)
        if not os.path.exists(filepath):
            unreal.log_warning(f"Skeleton FBX not found: {filepath}")
            continue

        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", "/Game/Characters/Knight")
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("filename", filepath)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
        skeleton_path = f"/Game/Characters/Knight/{asset_name}"
        unreal.log(f"Queued skeleton: {filename} -> {asset_name}")

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        unreal.log(f"Skeleton imported to {skeleton_path}")

    # Second pass: import animations
    anim_tasks = []
    imported_count = 0

    for filename, (asset_name, is_skeleton) in ANIM_MAP.items():
        if is_skeleton:
            continue

        filepath = os.path.join(ANIM_DIR, filename)
        if not os.path.exists(filepath):
            unreal.log_warning(f"Animation FBX not found: {filepath}")
            continue

        task = unreal.AssetImportTask()
        task.set_editor_property("automated", True)
        task.set_editor_property("destination_path", DEST_ANIM_PATH)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("filename", filepath)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        anim_tasks.append(task)
        imported_count += 1

    if anim_tasks:
        # Import in batches of 10 to avoid overwhelming the engine
        batch_size = 10
        for i in range(0, len(anim_tasks), batch_size):
            batch = anim_tasks[i:i+batch_size]
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(batch)
            unreal.log(f"Imported batch {i//batch_size + 1} ({len(batch)} animations)")

    unreal.log(f"=== Import complete: {imported_count} animations imported ===")


if __name__ == "__main__":
    import_fbx()
