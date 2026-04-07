extends Node2D
## Main world scene — coordinates rendering, input, and game flow.

const TILE_SIZE := 128

var current_plane: int = 0
var selected_army_id: int = -1
var _hovered_tile := Vector2i(-1, -1)

@onready var camera: Camera2D = $WorldCamera
@onready var tile_renderer: Node2D = $TileRenderer
@onready var army_sprites_node: Node2D = $ArmySprites
@onready var city_markers_node: Node2D = $CityMarkers
@onready var fog_overlay: Node2D = $FogOverlay
@onready var resource_bar: HBoxContainer = %ResourceBar if has_node("%ResourceBar") else $HUD/ResourceBar
@onready var turn_button: Button = $HUD/TurnButton
@onready var army_panel: PanelContainer = $HUD/ArmyPanel
@onready var notification_log: VBoxContainer = $HUD/NotificationLog

# ── Lifecycle ────────────────────────────────────────────────────────────────

func _ready() -> void:
	turn_button.pressed.connect(on_end_turn_pressed)
	
	# Wait one frame for autoloads to initialise
	await get_tree().process_frame
	
	# Start a new game if GameState hasn't been set up yet
	if not GameState.is_game_started():
		_start_new_game()
	
	render_plane(current_plane)
	refresh_army_sprites()
	refresh_city_markers()
	_update_hud()

func _start_new_game() -> void:
	# 1 human wizard + 3 AI on Aurelith
	var wizards: Array[Dictionary] = []
	wizards.append({
		"id": 0,
		"name": "Player",
		"is_human": true,
		"race": "high_men",
		"color": Color.CORNFLOWER_BLUE,
		"gold": 100,
		"mana": 50,
		"fame": 0,
	})
	var ai_names := ["Rjak", "Sss'ra", "Lo Pan"]
	var ai_races := ["dark_elves", "draconians", "nomads"]
	var ai_colors := [Color.DARK_RED, Color.DARK_GREEN, Color.DARK_VIOLET]
	for i in range(3):
		wizards.append({
			"id": i + 1,
			"name": ai_names[i],
			"is_human": false,
			"race": ai_races[i],
			"color": ai_colors[i],
			"gold": 100,
			"mana": 50,
			"fame": 0,
		})
	GameState.start_game(wizards)

# ── Input ────────────────────────────────────────────────────────────────────

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var tile_pos := _mouse_to_tile()
			on_tile_clicked(tile_pos)
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			# Deselect
			selected_army_id = -1
			army_panel.visible = false

func _mouse_to_tile() -> Vector2i:
	var world_pos := get_global_mouse_position()
	var tx := int(floor(world_pos.x / TILE_SIZE))
	var ty := int(floor(world_pos.y / TILE_SIZE))
	return Vector2i(tx, ty)

# ── Tile interaction ─────────────────────────────────────────────────────────

func on_tile_clicked(tile_pos: Vector2i) -> void:
	if not WorldMap.is_valid_tile(current_plane, tile_pos):
		return
	
	# If an army is selected, try to move it
	if selected_army_id >= 0:
		_try_move_army(tile_pos)
		return
	
	# Check if there's an army at this tile belonging to the player
	var army_id := _army_at(tile_pos)
	if army_id >= 0:
		var army: Dictionary = GameState.get_army(army_id)
		if army.get("owner_id", -1) == GameState.current_wizard_id():
			selected_army_id = army_id
			_update_army_panel(army_id)
			return
	
	# Check for city at tile
	var city := WorldMap.get_city_at(current_plane, tile_pos)
	if city.size() > 0:
		show_notification("City: %s (pop %d)" % [city.get("name", "?"), city.get("population", 0)])

func _try_move_army(target: Vector2i) -> void:
	var army: Dictionary = GameState.get_army(selected_army_id)
	if army.is_empty():
		selected_army_id = -1
		return
	
	var from := Vector2i(army["x"], army["y"])
	var path: Array = Pathfinder.find_path(current_plane, from, target)
	if path.is_empty():
		show_notification("No valid path.")
		return
	
	# Move step-by-step up to movement budget
	var remaining_mp: int = army.get("movement_left", 0)
	var final_pos := from
	for i in range(1, path.size()):
		var step: Vector2i = path[i]
		var cost: int = WorldMap.get_move_cost(current_plane, step)
		if cost > remaining_mp:
			break
		remaining_mp -= cost
		final_pos = step
	
	if final_pos == from:
		show_notification("Not enough movement.")
		return
	
	# Check for enemy at destination — trigger combat
	var enemy_army := _enemy_army_at(final_pos)
	if enemy_army >= 0:
		var result := CombatEngine.resolve(selected_army_id, enemy_army)
		show_combat_result(result)
	else:
		GameState.move_army(selected_army_id, final_pos)
		GameState.set_army_movement(selected_army_id, remaining_mp)
	
	selected_army_id = -1
	army_panel.visible = false
	refresh_army_sprites()
	fog_overlay.refresh()

func _army_at(tile_pos: Vector2i) -> int:
	for army_id in GameState.get_all_army_ids():
		var a: Dictionary = GameState.get_army(army_id)
		if a.get("plane", -1) == current_plane and Vector2i(a["x"], a["y"]) == tile_pos:
			return army_id
	return -1

func _enemy_army_at(tile_pos: Vector2i) -> int:
	var pid := GameState.current_wizard_id()
	for army_id in GameState.get_all_army_ids():
		var a: Dictionary = GameState.get_army(army_id)
		if a.get("plane", -1) == current_plane and Vector2i(a["x"], a["y"]) == tile_pos:
			if a.get("owner_id", -1) != pid:
				return army_id
	return -1

# ── Rendering helpers ────────────────────────────────────────────────────────

func render_plane(plane_index: int) -> void:
	current_plane = plane_index
	tile_renderer.set_plane(plane_index)
	fog_overlay.queue_redraw()

func refresh_army_sprites() -> void:
	# Clear existing
	for child in army_sprites_node.get_children():
		child.queue_free()
	
	var pid := GameState.current_wizard_id()
	for army_id in GameState.get_all_army_ids():
		var army: Dictionary = GameState.get_army(army_id)
		if army.get("plane", -1) != current_plane:
			continue
		# Fog check — only show if tile is visible to player
		var apos := Vector2i(army["x"], army["y"])
		if not FogOfWar.is_visible(pid, current_plane, apos):
			continue
		
		var sprite := Sprite2D.new()
		# Use a simple colored circle for MVP
		var img := Image.create(64, 64, false, Image.FORMAT_RGBA8)
		var owner_color: Color = GameState.get_wizard_color(army.get("owner_id", 0))
		_draw_circle_on_image(img, 32, 32, 24, owner_color)
		sprite.texture = ImageTexture.create_from_image(img)
		sprite.position = Vector2(apos.x * TILE_SIZE + TILE_SIZE / 2, apos.y * TILE_SIZE + TILE_SIZE / 2)
		sprite.z_index = 10
		army_sprites_node.add_child(sprite)

func _draw_circle_on_image(img: Image, cx: int, cy: int, radius: int, color: Color) -> void:
	for y in range(img.get_height()):
		for x in range(img.get_width()):
			var dx := x - cx
			var dy := y - cy
			if dx * dx + dy * dy <= radius * radius:
				img.set_pixel(x, y, color)

func refresh_city_markers() -> void:
	for child in city_markers_node.get_children():
		child.queue_free()
	
	var cities: Array = WorldMap.get_cities(current_plane)
	for city in cities:
		var pos := Vector2i(city["x"], city["y"])
		var label := Label.new()
		label.text = city.get("name", "City")
		label.position = Vector2(pos.x * TILE_SIZE, pos.y * TILE_SIZE - 20)
		label.add_theme_color_override("font_color", Color.WHITE)
		label.z_index = 15
		city_markers_node.add_child(label)
		
		# Small square marker
		var marker := ColorRect.new()
		marker.color = GameState.get_wizard_color(city.get("owner_id", -1))
		marker.size = Vector2(32, 32)
		marker.position = Vector2(pos.x * TILE_SIZE + 48, pos.y * TILE_SIZE + 48)
		marker.z_index = 12
		city_markers_node.add_child(marker)

# ── Turn flow ────────────────────────────────────────────────────────────────

func on_end_turn_pressed() -> void:
	selected_army_id = -1
	army_panel.visible = false
	GameState.end_player_turn()
	render_plane(current_plane)
	refresh_army_sprites()
	refresh_city_markers()
	_update_hud()
	show_notification("Turn %d" % GameState.current_turn())

# ── HUD ──────────────────────────────────────────────────────────────────────

func _update_hud() -> void:
	var wiz: Dictionary = GameState.get_current_wizard()
	$HUD/ResourceBar/GoldLabel.text = "Gold: %d" % wiz.get("gold", 0)
	$HUD/ResourceBar/ManaLabel.text = "Mana: %d" % wiz.get("mana", 0)
	$HUD/ResourceBar/FameLabel.text = "Fame: %d" % wiz.get("fame", 0)
	$HUD/ResourceBar/TurnLabel.text = "Turn: %d" % GameState.current_turn()

func _update_army_panel(army_id: int) -> void:
	var army: Dictionary = GameState.get_army(army_id)
	if army.is_empty():
		army_panel.visible = false
		return
	army_panel.visible = true
	$HUD/ArmyPanel/ArmyInfo/ArmyTitle.text = "Army #%d" % army_id
	
	# Clear existing unit labels
	var unit_list: VBoxContainer = $HUD/ArmyPanel/ArmyInfo/UnitList
	for child in unit_list.get_children():
		child.queue_free()
	
	var units: Array = army.get("units", [])
	for unit in units:
		var lbl := Label.new()
		lbl.text = "%s  HP:%d/%d  ATK:%d" % [
			unit.get("name", "?"),
			unit.get("hp", 0),
			unit.get("max_hp", 0),
			unit.get("melee_attack", 0),
		]
		unit_list.add_child(lbl)

# ── Notifications ────────────────────────────────────────────────────────────

func show_combat_result(result: Dictionary) -> void:
	var msg := "Combat: "
	if result.get("attacker_won", false):
		msg += "Attacker wins!"
	else:
		msg += "Defender wins!"
	show_notification(msg)
	refresh_army_sprites()

func show_notification(text: String) -> void:
	var lbl := Label.new()
	lbl.text = text
	lbl.add_theme_color_override("font_color", Color.YELLOW)
	notification_log.add_child(lbl)
	# Keep only last 8 notifications
	while notification_log.get_child_count() > 8:
		notification_log.get_child(0).queue_free()
