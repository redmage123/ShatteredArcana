extends Node2D
## Main world scene -- coordinates rendering, input, and game flow.
## Integrates city screen, spell book, diplomacy, combat popup, minimap,
## and plane switching.

const TILE_SIZE := 128

var current_plane: int = 0
var selected_army_id: int = -1
var _hovered_tile := Vector2i(-1, -1)

@onready var camera: Camera2D = $WorldCamera
@onready var tile_renderer: Node2D = $TileRenderer
@onready var army_sprites_node: Node2D = $ArmySprites
@onready var city_markers_node: Node2D = $CityMarkers
@onready var fog_overlay: Node2D = $FogOverlay
@onready var resource_bar: HBoxContainer = $HUD/ResourceBar
@onready var turn_button: Button = $HUD/TurnButton
@onready var army_panel: PanelContainer = $HUD/ArmyPanel
@onready var notification_log: VBoxContainer = $HUD/NotificationLog

# UI panel instances (created at runtime)
var _city_screen: PanelContainer
var _spell_book: PanelContainer
var _diplomacy_screen: PanelContainer
var _combat_popup: PanelContainer
var _minimap: TextureRect
var _plane_bar: HBoxContainer

# Plane display names
const PLANE_NAMES: Array = [
	"Aurelith", "Noctharion", "Verdania", "Pyratheon",
	"Glacium", "Aethermyst", "Shadowdeep", "Celestara",
]

# -- Lifecycle ----------------------------------------------------------------

func _ready() -> void:
	turn_button.pressed.connect(on_end_turn_pressed)

	await get_tree().process_frame

	if not GameState.is_game_started():
		_start_new_game()

	_create_ui_panels()
	_connect_signals()

	render_plane(current_plane)
	refresh_army_sprites()
	refresh_city_markers()
	_update_hud()
	_update_minimap()

func _start_new_game() -> void:
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


# -- UI panel creation --------------------------------------------------------

func _create_ui_panels() -> void:
	# City screen
	var city_scene := load("res://scenes/city/city_screen.tscn")
	if city_scene:
		_city_screen = city_scene.instantiate()
		$HUD.add_child(_city_screen)

	# Spell book
	var spell_scene := load("res://scenes/magic/spell_book.tscn")
	if spell_scene:
		_spell_book = spell_scene.instantiate()
		$HUD.add_child(_spell_book)

	# Diplomacy screen
	var diplo_scene := load("res://scenes/diplomacy/diplomacy_screen.tscn")
	if diplo_scene:
		_diplomacy_screen = diplo_scene.instantiate()
		$HUD.add_child(_diplomacy_screen)

	# Combat popup
	var combat_scene := load("res://scenes/combat/combat_popup.tscn")
	if combat_scene:
		_combat_popup = combat_scene.instantiate()
		$HUD.add_child(_combat_popup)

	# Minimap (bottom-right corner)
	var minimap_script := load("res://scenes/hud/minimap.gd")
	if minimap_script:
		_minimap = TextureRect.new()
		_minimap.set_script(minimap_script)
		_minimap.anchors_preset = Control.PRESET_BOTTOM_RIGHT
		_minimap.anchor_left = 1.0
		_minimap.anchor_top = 1.0
		_minimap.anchor_right = 1.0
		_minimap.anchor_bottom = 1.0
		_minimap.offset_left = -210
		_minimap.offset_top = -135
		_minimap.offset_right = -5
		_minimap.offset_bottom = -5
		$HUD.add_child(_minimap)
		_minimap.camera_jump_requested.connect(_on_minimap_jump)

	# Plane selector bar (top of screen, after resource bar)
	_plane_bar = HBoxContainer.new()
	_plane_bar.anchors_preset = Control.PRESET_TOP_WIDE
	_plane_bar.anchor_right = 1.0
	_plane_bar.offset_top = 44
	_plane_bar.offset_bottom = 78
	$HUD.add_child(_plane_bar)

	var plane_label := Label.new()
	plane_label.text = "Plane: "
	plane_label.add_theme_font_size_override("font_size", 13)
	_plane_bar.add_child(plane_label)

	for i in range(Constants.NUM_PLANES):
		var btn := Button.new()
		btn.text = PLANE_NAMES[i] if i < PLANE_NAMES.size() else "Plane %d" % i
		btn.custom_minimum_size = Vector2(90, 28)
		btn.add_theme_font_size_override("font_size", 11)
		var plane_idx := i
		btn.pressed.connect(func(): _on_plane_selected(plane_idx))
		_plane_bar.add_child(btn)


func _connect_signals() -> void:
	# EventBus signals for refresh after turn processing
	EventBus.turn_started.connect(_on_turn_started)
	EventBus.turn_ended.connect(_on_turn_ended)
	EventBus.combat_resolved.connect(_on_combat_resolved)
	EventBus.city_founded.connect(_on_city_event)
	EventBus.city_production_complete.connect(_on_production_complete)
	EventBus.spell_researched.connect(_on_spell_researched)
	EventBus.war_declared.connect(_on_war_declared)
	EventBus.peace_made.connect(_on_peace_made)
	EventBus.wizard_eliminated.connect(_on_wizard_eliminated)
	EventBus.victory_achieved.connect(_on_victory)
	EventBus.plane_changed.connect(_on_plane_changed_signal)


# -- Input --------------------------------------------------------------------

func _unhandled_input(event: InputEvent) -> void:
	# Block world input when a panel is open
	if _is_panel_open():
		if event is InputEventKey and event.pressed:
			if event.keycode == KEY_ESCAPE:
				_close_all_panels()
		return

	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var tile_pos := _mouse_to_tile()
			on_tile_clicked(tile_pos)
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			selected_army_id = -1
			army_panel.visible = false

	if event is InputEventKey and event.pressed:
		match event.keycode:
			KEY_B:
				_toggle_spell_book()
			KEY_D:
				_toggle_diplomacy()
			KEY_ESCAPE:
				_close_all_panels()


func _is_panel_open() -> bool:
	if _city_screen and _city_screen.visible:
		return true
	if _spell_book and _spell_book.visible:
		return true
	if _diplomacy_screen and _diplomacy_screen.visible:
		return true
	if _combat_popup and _combat_popup.visible:
		return true
	return false


func _close_all_panels() -> void:
	if _city_screen and _city_screen.visible:
		_city_screen.close_city()
	if _spell_book and _spell_book.visible:
		_spell_book.close_spell_book()
	if _diplomacy_screen and _diplomacy_screen.visible:
		_diplomacy_screen.close_diplomacy()
	if _combat_popup and _combat_popup.visible:
		_combat_popup.dismiss()


func _mouse_to_tile() -> Vector2i:
	var world_pos := get_global_mouse_position()
	var tx := int(floor(world_pos.x / TILE_SIZE))
	var ty := int(floor(world_pos.y / TILE_SIZE))
	return Vector2i(tx, ty)


# -- Tile interaction ---------------------------------------------------------

func on_tile_clicked(tile_pos: Vector2i) -> void:
	if not WorldMap.is_valid_tile(current_plane, tile_pos):
		return

	# If an army is selected, try to move it
	if selected_army_id >= 0:
		_try_move_army(tile_pos)
		return

	# Check if there is an army at this tile belonging to the player
	var army_id := _army_at(tile_pos)
	if army_id >= 0:
		var army: Dictionary = GameState.get_army(army_id)
		if army.get("owner_id", -1) == GameState.current_wizard_id():
			selected_army_id = army_id
			_update_army_panel(army_id)
			return

	# Check for city at tile -- open city screen
	var city := WorldMap.get_city_at(current_plane, tile_pos)
	if city.size() > 0:
		var cid: int = city.get("city_id", -1)
		if cid >= 0 and _city_screen:
			_city_screen.open_city(cid)
			return
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

	var enemy_army := _enemy_army_at(final_pos)
	if enemy_army >= 0:
		var result := CombatEngine.resolve(selected_army_id, enemy_army)
		_show_combat_popup(result)
	else:
		GameState.move_army(selected_army_id, final_pos)
		GameState.set_army_movement(selected_army_id, remaining_mp)

	selected_army_id = -1
	army_panel.visible = false
	refresh_army_sprites()
	fog_overlay.refresh()
	_update_minimap()


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


# -- Panel toggles ------------------------------------------------------------

func _toggle_spell_book() -> void:
	if _spell_book == null:
		return
	if _spell_book.visible:
		_spell_book.close_spell_book()
	else:
		_close_all_panels()
		_spell_book.open_spell_book()


func _toggle_diplomacy() -> void:
	if _diplomacy_screen == null:
		return
	if _diplomacy_screen.visible:
		_diplomacy_screen.close_diplomacy()
	else:
		_close_all_panels()
		_diplomacy_screen.open_diplomacy()


# -- Rendering helpers --------------------------------------------------------

func render_plane(plane_index: int) -> void:
	current_plane = plane_index
	tile_renderer.set_plane(plane_index)
	fog_overlay.queue_redraw()
	_update_minimap()


func refresh_army_sprites() -> void:
	for child in army_sprites_node.get_children():
		child.queue_free()

	var pid := GameState.current_wizard_id()
	for army_id in GameState.get_all_army_ids():
		var army: Dictionary = GameState.get_army(army_id)
		if army.get("plane", -1) != current_plane:
			continue
		var apos := Vector2i(army["x"], army["y"])
		if not FogOfWar.is_visible(pid, current_plane, apos):
			continue

		var sprite := Sprite2D.new()
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

		var marker := ColorRect.new()
		marker.color = GameState.get_wizard_color(city.get("owner_id", -1))
		marker.size = Vector2(32, 32)
		marker.position = Vector2(pos.x * TILE_SIZE + 48, pos.y * TILE_SIZE + 48)
		marker.z_index = 12
		city_markers_node.add_child(marker)


# -- Turn flow ----------------------------------------------------------------

func on_end_turn_pressed() -> void:
	if _is_panel_open():
		return
	selected_army_id = -1
	army_panel.visible = false
	GameState.end_player_turn()
	render_plane(current_plane)
	refresh_army_sprites()
	refresh_city_markers()
	_update_hud()
	_update_minimap()
	show_notification("Turn %d" % GameState.current_turn())


# -- HUD ----------------------------------------------------------------------

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


# -- Minimap ------------------------------------------------------------------

func _update_minimap() -> void:
	if _minimap and _minimap.has_method("set_plane"):
		_minimap.set_plane(current_plane)
		_minimap.set_player_id(GameState.current_wizard_id() if GameState.has_method("current_wizard_id") else 0)
		_minimap.refresh()


func _on_minimap_jump(world_pos: Vector2) -> void:
	if camera:
		camera.position = world_pos


# -- Plane switching ----------------------------------------------------------

func _on_plane_selected(plane_idx: int) -> void:
	if plane_idx == current_plane:
		return
	render_plane(plane_idx)
	refresh_army_sprites()
	refresh_city_markers()
	_update_minimap()
	show_notification("Switched to %s" % (PLANE_NAMES[plane_idx] if plane_idx < PLANE_NAMES.size() else "Plane %d" % plane_idx))
	EventBus.plane_changed.emit(plane_idx)


# -- Combat popup -------------------------------------------------------------

func _show_combat_popup(result: Dictionary) -> void:
	if _combat_popup and _combat_popup.has_method("show_result"):
		_combat_popup.show_result(result)
	else:
		show_combat_result(result)


# -- EventBus signal handlers -------------------------------------------------

func _on_turn_started(_turn: int) -> void:
	_update_hud()
	refresh_army_sprites()
	refresh_city_markers()
	_update_minimap()


func _on_turn_ended(_turn: int) -> void:
	_update_hud()


func _on_combat_resolved(result: Dictionary) -> void:
	_show_combat_popup(result)
	refresh_army_sprites()
	_update_minimap()


func _on_city_event(_city_id: int) -> void:
	refresh_city_markers()
	_update_minimap()


func _on_production_complete(city_id: int, item: String) -> void:
	show_notification("Production complete in city #%d: %s" % [city_id, item])
	refresh_army_sprites()


func _on_spell_researched(wizard_id: int, spell_id: String) -> void:
	if wizard_id == GameState.current_wizard_id() if GameState.has_method("current_wizard_id") else 0:
		show_notification("Spell researched: %s" % spell_id)


func _on_war_declared(attacker_id: int, defender_id: int) -> void:
	var atk_name: String = GameState.get_wizard(attacker_id).get("name", "?")
	var def_name: String = GameState.get_wizard(defender_id).get("name", "?")
	show_notification("%s declared war on %s!" % [atk_name, def_name])


func _on_peace_made(wizard_a: int, wizard_b: int) -> void:
	var a_name: String = GameState.get_wizard(wizard_a).get("name", "?")
	var b_name: String = GameState.get_wizard(wizard_b).get("name", "?")
	show_notification("Peace between %s and %s" % [a_name, b_name])


func _on_wizard_eliminated(wizard_id: int) -> void:
	var name: String = GameState.get_wizard(wizard_id).get("name", "?")
	show_notification("%s has been eliminated!" % name)


func _on_victory(wizard_id: int, victory_type: String) -> void:
	var name: String = GameState.get_wizard(wizard_id).get("name", "?")
	show_notification("%s achieves %s victory!" % [name, victory_type])


func _on_plane_changed_signal(plane_idx: int) -> void:
	if plane_idx != current_plane:
		_on_plane_selected(plane_idx)


# -- Notifications ------------------------------------------------------------

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
	while notification_log.get_child_count() > 8:
		notification_log.get_child(0).queue_free()
