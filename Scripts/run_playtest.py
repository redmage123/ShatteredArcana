"""Run the playtest by calling RunPlaytest directly on the subsystem."""
import unreal

# Find the GameInstance. In a no-map commandlet UE still constructs one.
gi = unreal.GameplayStatics.get_game_instance(unreal.EditorLevelLibrary.get_editor_world())
if gi is None:
    unreal.log_error("No GameInstance available; commandlet needs a world or PIE context.")
else:
    sub = gi.get_subsystem(unreal.CoMPlaytestSubsystem)
    if sub is None:
        unreal.log_error("UCoMPlaytestSubsystem not loaded.")
    else:
        unreal.log("Calling RunPlaytest(2, 50, 42, '')...")
        sub.run_playtest(2, 50, 42, "")
        unreal.log("RunPlaytest returned.")
