import unreal

# Import the aurelith establishing shot as a menu background texture
task = unreal.AssetImportTask()
task.set_editor_property("automated", True)
task.set_editor_property("destination_path", "/Game/Textures/UI")
task.set_editor_property("destination_name", "T_MenuBackground")
task.set_editor_property("filename", "C:/Users/Braun/repos/ShatteredArcana/Content/Textures/UI/T_MenuBackground.png")
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

if task.get_editor_property("imported_object_paths"):
    unreal.log("Successfully imported menu background texture")
else:
    unreal.log_warning("Import may have failed - check the asset")
