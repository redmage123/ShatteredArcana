"""
create_overworld_level.py
Run inside the UE5 Python console or as an editor commandlet:
  Edit > Execute Python Script... > select this file

Creates Content/Maps/L_Overworld with:
  - GameMode Override: ACoMOverworldGameMode
  - Default Pawn: ACoMCameraPawn
  - Directional Light + Sky Atmosphere (placeholder ambient rig)
  - Player Start at (0, 0, 100)
  - Static mesh floor plane (placeholder grid, 1km x 1km)

UE 5.4 compatible — uses LevelEditorSubsystem (EditorLevelLibrary deprecated in 5.1+).
"""
import unreal

# Get the subsystem-based API (UE 5.1+)
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

# 1. Create a new empty level
map_path = "/Game/Maps/L_Overworld"
level_subsystem.new_level(map_path)

# 2. Get editor world
world = editor_subsystem.get_editor_world()
if not world:
    unreal.log_error("Failed to get editor world!")
    raise RuntimeError("No editor world")

# 3. World settings — GameMode and Pawn
# Find the WorldSettings actor
ws_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)
if ws_actors:
    ws = ws_actors[0]

    # Load the GameMode class — UObject name strips the 'A' prefix
    gm_class = unreal.load_class(None, "/Script/CoMCore.CoMOverworldGameMode")
    if gm_class:
        ws.set_editor_property("default_game_mode", gm_class)
        unreal.log("Set GameMode Override: CoMOverworldGameMode")
    else:
        unreal.log_warning("Could not load CoMOverworldGameMode class! Verify CoMCore module is compiled.")

    pawn_class = unreal.load_class(None, "/Script/CoMCore.CoMCameraPawn")
    if pawn_class:
        ws.set_editor_property("default_pawn_class", pawn_class)
        unreal.log("Set Default Pawn: CoMCameraPawn")
    else:
        unreal.log_warning("Could not load CoMCameraPawn class!")
else:
    unreal.log_error("No WorldSettings actor found in level!")

# 4. Directional Light (Sun)
dl = unreal.EditorActorSubsystem().spawn_actor_from_class(
    unreal.DirectionalLight,
    unreal.Vector(0, 0, 10000),
    unreal.Rotator(-45, 0, 0)
)
if dl:
    dl.set_actor_label("Sun_Directional")

# 5. Sky Atmosphere
sky = unreal.EditorActorSubsystem().spawn_actor_from_class(
    unreal.SkyAtmosphere,
    unreal.Vector(0, 0, 0),
    unreal.Rotator(0, 0, 0)
)
if sky:
    sky.set_actor_label("SkyAtmosphere_Placeholder")

# 6. Player Start
ps = unreal.EditorActorSubsystem().spawn_actor_from_class(
    unreal.PlayerStart,
    unreal.Vector(0, 0, 100),
    unreal.Rotator(0, 0, 0)
)
if ps:
    ps.set_actor_label("PlayerStart")

# 7. Floor plane (1km x 1km static mesh — use built-in plane)
plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
floor = unreal.EditorActorSubsystem().spawn_actor_from_class(
    unreal.StaticMeshActor,
    unreal.Vector(0, 0, 0),
    unreal.Rotator(0, 0, 0)
)
if floor and plane_mesh:
    floor.static_mesh_component.set_static_mesh(plane_mesh)
    floor.set_actor_scale3d(unreal.Vector(200, 200, 1))
    floor.set_actor_label("Floor_Placeholder")

# 8. Save
level_subsystem.save_current_level()
unreal.log("L_Overworld created and saved to " + map_path)
