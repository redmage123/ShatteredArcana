extends Control
## Title screen — New Game starts a game with 1 human + 3 AI on Aurelith.

func _ready() -> void:
	$VBox/NewGameButton.pressed.connect(_on_new_game)
	$VBox/QuitButton.pressed.connect(_on_quit)
	
	# Style the title label bigger
	var title: Label = $VBox/Title
	title.add_theme_font_size_override("font_size", 48)
	title.add_theme_color_override("font_color", Color(0.85, 0.75, 1.0))
	
	var subtitle: Label = $VBox/Subtitle
	subtitle.add_theme_font_size_override("font_size", 18)
	subtitle.add_theme_color_override("font_color", Color(0.6, 0.55, 0.75))

func _on_new_game() -> void:
	get_tree().change_scene_to_file("res://scenes/world/world.tscn")

func _on_quit() -> void:
	get_tree().quit()
