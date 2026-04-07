extends CanvasLayer
## HUD management — resource display, army panel, notifications, minimap.

@onready var resource_bar: HBoxContainer = get_node_or_null("ResourceBar")
@onready var army_panel: PanelContainer = get_node_or_null("ArmyPanel")
@onready var notification_log: VBoxContainer = get_node_or_null("NotificationLog")

# ── Resources ────────────────────────────────────────────────────────────────

func update_resources() -> void:
	if resource_bar == null:
		return
	var wiz: Dictionary = GameState.get_current_wizard()
	_set_label("GoldLabel", "Gold: %d" % wiz.get("gold", 0))
	_set_label("ManaLabel", "Mana: %d" % wiz.get("mana", 0))
	_set_label("FameLabel", "Fame: %d" % wiz.get("fame", 0))
	_set_label("TurnLabel", "Turn: %d" % GameState.current_turn())

func _set_label(child_name: String, text: String) -> void:
	var lbl: Label = resource_bar.get_node_or_null(child_name)
	if lbl:
		lbl.text = text

# ── Army panel ───────────────────────────────────────────────────────────────

func update_army_panel(army_id: int) -> void:
	if army_panel == null:
		return
	var army: Dictionary = GameState.get_army(army_id)
	if army.is_empty():
		clear_army_panel()
		return
	army_panel.visible = true
	
	var info: VBoxContainer = army_panel.get_node_or_null("ArmyInfo")
	if info == null:
		return
	
	var title_lbl: Label = info.get_node_or_null("ArmyTitle")
	if title_lbl:
		title_lbl.text = "Army #%d" % army_id
	
	var unit_list: VBoxContainer = info.get_node_or_null("UnitList")
	if unit_list == null:
		return
	
	# Clear
	for child in unit_list.get_children():
		child.queue_free()
	
	# Populate
	var units: Array = army.get("units", [])
	for unit in units:
		var lbl := Label.new()
		lbl.text = "%s  HP:%d/%d  ATK:%d  DEF:%d" % [
			unit.get("name", "?"),
			unit.get("hp", 0),
			unit.get("max_hp", 0),
			unit.get("melee_attack", 0),
			unit.get("defense", 0),
		]
		lbl.add_theme_font_size_override("font_size", 14)
		unit_list.add_child(lbl)

func clear_army_panel() -> void:
	if army_panel:
		army_panel.visible = false

# ── Notifications ────────────────────────────────────────────────────────────

func add_notification(text: String) -> void:
	if notification_log == null:
		return
	var lbl := Label.new()
	lbl.text = text
	lbl.add_theme_color_override("font_color", Color.YELLOW)
	lbl.add_theme_font_size_override("font_size", 14)
	notification_log.add_child(lbl)
	# Keep recent only
	while notification_log.get_child_count() > 10:
		notification_log.get_child(0).queue_free()

# ── Minimap ──────────────────────────────────────────────────────────────────

func update_minimap() -> void:
	# MVP minimap: draw a small overview image
	# This can be called from the world scene when the plane or fog changes
	pass

func draw_minimap_to_texture(plane: int, width: int, height: int) -> ImageTexture:
	var scale := 2  # each tile = 2px
	var img := Image.create(width * scale, height * scale, false, Image.FORMAT_RGBA8)
	
	var terrain_colors: Dictionary = {
		0:  Color(0.10, 0.23, 0.36),
		1:  Color(0.29, 0.54, 0.69),
		2:  Color(0.29, 0.54, 0.23),
		3:  Color(0.16, 0.35, 0.16),
		4:  Color(0.54, 0.48, 0.35),
		5:  Color(0.42, 0.42, 0.48),
		6:  Color(0.85, 0.82, 0.70),
		7:  Color(0.76, 0.70, 0.50),
		8:  Color(0.20, 0.40, 0.22),
		9:  Color(0.88, 0.92, 0.96),
		10: Color(0.45, 0.30, 0.20),
	}
	
	for y in range(height):
		for x in range(width):
			var terrain: int = WorldMap.get_terrain(plane, Vector2i(x, y))
			var color: Color = terrain_colors.get(terrain, Color.MAGENTA)
			for py in range(scale):
				for px in range(scale):
					img.set_pixel(x * scale + px, y * scale + py, color)
	
	return ImageTexture.create_from_image(img)
