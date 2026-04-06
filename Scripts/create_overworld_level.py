"""
create_overworld_level.py
Run inside the UE5 Python console or as an editor commandlet:
  Edit > Execute Python Script... → select this file

Creates Content/Maps/L_Overworld with:
  - GameMode Override: ACoMOverworldGameMode
  - Default Pawn: ACoMCameraPawn
  - Directional Light + Sky Atmosphere (placeholder ambient rig)
  - Player Start at (0, 0, 100)
  - Static mesh floor plane (placeholder grid, 1km × 1km)
"""
import unreal

# 1. Create a new empty level
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
map_pkg = "/Game/Maps/L_Overworld"
unreal.EditorLevelLibrary.new_level(map_pkg)

# 2. World settings — GameMode and Pawn
world_settings = unreal.GameplayStatics.get_game_mode(unreal.EditorLevelLibrary.get_editor_world())
world = unreal.EditorLevelLibrary.get_editor_world()

# Set via WorldSettings actor
ws_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)
if ws_actors:
    ws = ws_actors[0]
    ws.set_editor_property("default_game_mode",
        unreal.load_class(None, "/Script/CoMCore.CoMOverworldGameMode"))
    ws.set_editor_property("default_pawn_class",
        unreal.load_class(None, "/Script/CoMCore.CoMCameraPawn"))

# 3. Directional Light
dl_loc = unreal.Vector(0, 0, 10000)
dl_rot = unreal.Rotator(-45, 0, 0)
dl = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.DirectionalLight, dl_loc, dl_rot)
if dl:
    dl.set_actor_label("Sun_Directional")

# 4. Sky Atmosphere
sky_loc = unreal.Vector(0, 0, 0)
sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.SkyAtmosphere, sky_loc, unreal.Rotator(0, 0, 0))
if sky:
    sky.set_actor_label("SkyAtmosphere_Placeholder")

# 5. Player Start
ps = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(0, 0, 100), unreal.Rotator(0, 0, 0))
if ps:
    ps.set_actor_label("PlayerStart")

# 6. Floor plane (1km × 1km static mesh — use built-in plane)
plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
if floor and plane_mesh:
    floor.static_mesh_component.set_static_mesh(plane_mesh)
    floor.set_actor_scale3d(unreal.Vector(200, 200, 1))  # 100u × 200 = 20 000 units = 200m
    floor.set_actor_label("Floor_Placeholder")

# 7. Save
unreal.EditorLevelLibrary.save_current_level()
unreal.log("L_Overworld created and saved to " + map_pkg)
