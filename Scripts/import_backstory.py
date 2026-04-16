import unreal

task = unreal.AssetImportTask()
task.set_editor_property("automated", True)
task.set_editor_property("destination_path", "/Game/Textures/UI")
task.set_editor_property("destination_name", "T_Backstory")
task.set_editor_property("filename", "C:/Users/Braun/repos/ShatteredArcana/Content/Textures/UI/T_Backstory.png")
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.log("Backstory texture imported")
