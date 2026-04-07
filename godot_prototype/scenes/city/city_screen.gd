extends PanelContainer
## City management screen -- building, production, and unit recruitment.
## Opened when clicking a city on the world map.

signal closed

const BUILDING_ICONS: Dictionary = {
	"granary": "Gr", "barracks": "Bk", "smithy": "Sm", "marketplace": "Mk",
	"library": "Lb", "shrine": "Sh", "temple": "Tp", "walls": "Wl",
	"stable": "St", "archery_range": "AR", "sawmill": "Sw", "alchemist": "Al",
	"wizard_guild": "WG", "fortress": "Ft", "cathedral": "Ca",
	"war_college": "WC", "oracle": "Or", "armory": "Am",
}

var _city_id: int = -1
var _city: Dictionary = {}

# UI node references (created in _ready)
var _header_name: Label
var _header_pop: Label
var _header_owner: Label
var _food_label: Label
var _prod_label: Label
var _gold_label: Label
var _mana_label: Label
var _built_list: VBoxContainer
var _available_list: VBoxContainer
var _queue_container: HBoxContainer
var _unit_row: HBoxContainer
var _confirm_dialog: ConfirmationDialog


func _ready() -> void:
	visible = false
	_build_ui()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func open_city(city_id: int) -> void:
	_city_id = city_id
	_city = CityManager.get_city(city_id)
	if _city.is_empty():
		return
	visible = true
	update_display()


func close_city() -> void:
	visible = false
	_city_id = -1
	closed.emit()


# ---------------------------------------------------------------------------
# Display
# ---------------------------------------------------------------------------

func update_display() -> void:
	_city = CityManager.get_city(_city_id)
	if _city.is_empty():
		close_city()
		return

	# Header
	_header_name.text = _city.get("name", "Unknown City")
	_header_pop.text = "Pop: %d / %d" % [_city.get("population", 0), Constants.MAX_CITY_POPULATION]
	var owner_id: int = _city.get("owner", -1)
	var wiz: Dictionary = GameState.get_wizard(owner_id)
	_header_owner.text = "Owner: %s" % wiz.get("name", "Neutral")

	# Income
	var income = CityManager.get_city_income(_city_id)
	_food_label.text = "Food: +%d" % income.get("food", 0)
	_prod_label.text = "Prod: +%d" % income.get("production", 0)
	_gold_label.text = "Gold: +%d" % income.get("gold", 0)
	_mana_label.text = "Mana: +%d" % income.get("mana", 0)

	_refresh_built_buildings()
	_refresh_available_buildings()
	_refresh_build_queue()
	_refresh_unit_buttons()


func _refresh_built_buildings() -> void:
	for child in _built_list.get_children():
		child.queue_free()

	var buildings: Array = _city.get("buildings", [])
	if buildings.is_empty():
		var lbl = Label.new()
		lbl.text = "  No buildings yet"
		lbl.add_theme_color_override("font_color", Color(0.6, 0.6, 0.6))
		_built_list.add_child(lbl)
		return

	for bid in buildings:
		var spec = DataLoader.get_building_spec(bid)
		var btn = Button.new()
		var icon_str: String = BUILDING_ICONS.get(bid, "--")
		btn.text = "[%s] %s" % [icon_str, spec.get("name", bid)]
		btn.tooltip_text = _building_effect_text(spec)
		btn.disabled = true
		btn.flat = true
		btn.add_theme_color_override("font_disabled_color", Color(0.8, 0.9, 0.8))
		_built_list.add_child(btn)


func _refresh_available_buildings() -> void:
	for child in _available_list.get_children():
		child.queue_free()

	var built: Array = _city.get("buildings", [])
	var queue_ids: Array = []
	for qi in _city.get("build_queue", []):
		if qi.get("build_type", "") == "building":
			queue_ids.append(qi.get("spec_id", ""))

	for bid in DataLoader.building_specs:
		if bid in built:
			continue
		if bid in queue_ids:
			continue

		var spec = DataLoader.get_building_spec(bid)
		var prereqs: Array = spec.get("requires", [])
		var prereqs_met = true
		var missing_prereqs: Array = []
		for req in prereqs:
			if req not in built:
				prereqs_met = false
				missing_prereqs.append(req)

		var btn = Button.new()
		var icon_str: String = BUILDING_ICONS.get(bid, "--")
		btn.text = "[%s] %s (%d)" % [icon_str, spec.get("name", bid), spec.get("cost", 0)]
		btn.disabled = not prereqs_met or _city.get("build_queue", []).size() >= Constants.MAX_BUILD_QUEUE

		if not prereqs_met:
			var missing_names: Array = []
			for req_id in missing_prereqs:
				var req_spec = DataLoader.get_building_spec(req_id)
				missing_names.append(req_spec.get("name", req_id))
			btn.tooltip_text = "Requires: %s" % ", ".join(missing_names)
			btn.modulate = Color(0.5, 0.5, 0.5)
		else:
			btn.tooltip_text = _building_effect_text(spec)

		var b_id = bid  # capture for lambda
		btn.pressed.connect(func(): on_build_clicked(b_id))
		_available_list.add_child(btn)


func _refresh_build_queue() -> void:
	for child in _queue_container.get_children():
		child.queue_free()

	var queue: Array = _city.get("build_queue", [])
	if queue.is_empty():
		var lbl = Label.new()
		lbl.text = "  Queue empty"
		lbl.add_theme_color_override("font_color", Color(0.5, 0.5, 0.5))
		_queue_container.add_child(lbl)
		return

	for i in range(queue.size()):
		var item = queue[i] as Dictionary
		var vbox = VBoxContainer.new()

		var name_lbl = Label.new()
		var spec_id: String = item.get("spec_id", "")
		if item.get("build_type", "") == "building":
			var spec = DataLoader.get_building_spec(spec_id)
			name_lbl.text = spec.get("name", spec_id)
		else:
			var spec = DataLoader.get_unit_spec(spec_id)
			name_lbl.text = spec.get("name", spec_id)
		name_lbl.add_theme_font_size_override("font_size", 12)
		vbox.add_child(name_lbl)

		# Progress bar
		var progress = ProgressBar.new()
		progress.custom_minimum_size = Vector2(100, 16)
		var cost: int = item.get("cost", 1)
		var prog: int = item.get("progress", 0)
		progress.max_value = cost
		progress.value = prog
		progress.show_percentage = false
		vbox.add_child(progress)

		# Turns remaining (only for first item)
		if i == 0:
			var income = CityManager.get_city_income(_city_id)
			var prod_power: int = maxi(1, income.get("production", 1) - _city.get("unrest", 0))
			var remaining = ceili(float(cost - prog) / float(prod_power))
			var turns_lbl = Label.new()
			turns_lbl.text = "%d turns" % remaining
			turns_lbl.add_theme_font_size_override("font_size", 11)
			turns_lbl.add_theme_color_override("font_color", Color(1.0, 0.9, 0.4))
			vbox.add_child(turns_lbl)

		# Click to remove
		var remove_btn = Button.new()
		remove_btn.text = "X"
		remove_btn.custom_minimum_size = Vector2(24, 24)
		var idx = i  # capture
		remove_btn.pressed.connect(func(): on_queue_item_clicked(idx))
		vbox.add_child(remove_btn)

		_queue_container.add_child(vbox)


func _refresh_unit_buttons() -> void:
	for child in _unit_row.get_children():
		child.queue_free()

	# Determine city race from owner wizard
	var owner_id: int = _city.get("owner", -1)
	var wiz = GameState.get_wizard(owner_id)
	var race_id: String = wiz.get("race", "high_men")

	# Find race data from DataLoader
	var race_data: Dictionary = _find_race_data(race_id)

	var unit_types = ["infantry", "ranged", "cavalry"]
	var spec_keys = ["infantry_spec", "ranged_spec", "cavalry_spec"]
	var requires_building = ["barracks", "archery_range", "stable"]

	for i in range(unit_types.size()):
		var spec: Dictionary = race_data.get(spec_keys[i], {})
		if spec.is_empty():
			continue

		var has_building: bool = requires_building[i] in _city.get("buildings", [])
		var queue_full: bool = _city.get("build_queue", []).size() >= Constants.MAX_BUILD_QUEUE

		var btn = Button.new()
		btn.text = "%s\n%d prod" % [spec.get("name", unit_types[i]), spec.get("production_cost", 40)]
		btn.custom_minimum_size = Vector2(120, 50)
		btn.disabled = not has_building or queue_full

		if not has_building:
			btn.tooltip_text = "Requires: %s" % requires_building[i]
			btn.modulate = Color(0.5, 0.5, 0.5)

		var u_type = unit_types[i]
		btn.pressed.connect(func(): on_unit_clicked(u_type))
		_unit_row.add_child(btn)


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

func on_build_clicked(building_id: String) -> void:
	CityManager.add_to_build_queue(_city_id, "building", building_id)
	update_display()


func on_unit_clicked(unit_type: String) -> void:
	var owner_id: int = _city.get("owner", -1)
	var wiz = GameState.get_wizard(owner_id)
	var race_id: String = wiz.get("race", "high_men")
	var spec_id = "%s_%s" % [race_id, unit_type]
	CityManager.add_to_build_queue(_city_id, "unit", spec_id)
	update_display()


func on_queue_item_clicked(index: int) -> void:
	var queue: Array = _city.get("build_queue", [])
	if index >= 0 and index < queue.size():
		if _confirm_dialog == null:
			_confirm_dialog = ConfirmationDialog.new()
			_confirm_dialog.title = "Remove from Queue?"
			add_child(_confirm_dialog)

		_confirm_dialog.dialog_text = "Remove this item from the build queue?"
		_confirm_dialog.confirmed.connect(func():
			queue.remove_at(index)
			update_display()
		, CONNECT_ONE_SHOT)
		_confirm_dialog.popup_centered()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

func _find_race_data(race_id: String) -> Dictionary:
	var races_path = "res://data/races.json"
	if not FileAccess.file_exists(races_path):
		return {}
	var file = FileAccess.open(races_path, FileAccess.READ)
	if file == null:
		return {}
	var json = JSON.new()
	if json.parse(file.get_as_text()) != OK:
		return {}
	var data = json.data
	if data is Array:
		for rd in data:
			if rd.get("id", "") == race_id:
				return rd
	return {}


func _building_effect_text(spec: Dictionary) -> String:
	var effects: Dictionary = spec.get("effects", {})
	var parts: Array = []
	if effects.has("food_bonus"):
		parts.append("+%d Food" % effects["food_bonus"])
	if effects.has("gold_bonus"):
		parts.append("+%d Gold" % effects["gold_bonus"])
	if effects.has("mana_bonus"):
		parts.append("+%d Mana" % effects["mana_bonus"])
	if effects.has("research_bonus"):
		parts.append("+%d Research" % effects["research_bonus"])
	if effects.has("unit_attack_bonus"):
		parts.append("+%d Unit Attack" % effects["unit_attack_bonus"])
	if effects.has("city_defense"):
		parts.append("+%d City Defense" % effects["city_defense"])
	if effects.has("unlocks_units"):
		parts.append("Unlocks: %s" % ", ".join(effects["unlocks_units"]))
	if effects.has("resistance_bonus"):
		parts.append("+%d Resistance" % effects["resistance_bonus"])
	if parts.is_empty():
		return spec.get("name", "Building")
	return "; ".join(parts)


# ---------------------------------------------------------------------------
# UI Construction
# ---------------------------------------------------------------------------

func _build_ui() -> void:
	custom_minimum_size = Vector2(720, 520)
	anchors_preset = PRESET_CENTER
	anchor_left = 0.5
	anchor_top = 0.5
	anchor_right = 0.5
	anchor_bottom = 0.5
	offset_left = -360
	offset_top = -260
	offset_right = 360
	offset_bottom = 260

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.12, 0.12, 0.18, 0.95)
	style.border_color = Color(0.4, 0.35, 0.6)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(12)
	add_theme_stylebox_override("panel", style)

	var main_vbox = VBoxContainer.new()
	main_vbox.layout_mode = 2
	add_child(main_vbox)

	# --- Header row ---
	var header = HBoxContainer.new()
	header.layout_mode = 2
	main_vbox.add_child(header)

	_header_name = Label.new()
	_header_name.text = "City Name"
	_header_name.add_theme_font_size_override("font_size", 22)
	_header_name.add_theme_color_override("font_color", Color(1.0, 0.9, 0.6))
	_header_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(_header_name)

	_header_pop = Label.new()
	_header_pop.text = "Pop: 0"
	_header_pop.add_theme_font_size_override("font_size", 16)
	header.add_child(_header_pop)

	var spacer1 = Control.new()
	spacer1.custom_minimum_size = Vector2(20, 0)
	header.add_child(spacer1)

	_header_owner = Label.new()
	_header_owner.text = "Owner: ?"
	_header_owner.add_theme_font_size_override("font_size", 16)
	header.add_child(_header_owner)

	var close_btn = Button.new()
	close_btn.text = "X"
	close_btn.custom_minimum_size = Vector2(32, 32)
	close_btn.pressed.connect(close_city)
	header.add_child(close_btn)

	main_vbox.add_child(HSeparator.new())

	# --- Resources row ---
	var res_row = HBoxContainer.new()
	res_row.layout_mode = 2
	main_vbox.add_child(res_row)

	_food_label = Label.new()
	_food_label.text = "Food: +0"
	_food_label.add_theme_color_override("font_color", Color(0.4, 0.9, 0.3))
	res_row.add_child(_food_label)

	var spacer_r1 = Control.new()
	spacer_r1.custom_minimum_size = Vector2(30, 0)
	res_row.add_child(spacer_r1)

	_prod_label = Label.new()
	_prod_label.text = "Prod: +0"
	_prod_label.add_theme_color_override("font_color", Color(0.8, 0.6, 0.3))
	res_row.add_child(_prod_label)

	var spacer_r2 = Control.new()
	spacer_r2.custom_minimum_size = Vector2(30, 0)
	res_row.add_child(spacer_r2)

	_gold_label = Label.new()
	_gold_label.text = "Gold: +0"
	_gold_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.2))
	res_row.add_child(_gold_label)

	var spacer_r3 = Control.new()
	spacer_r3.custom_minimum_size = Vector2(30, 0)
	res_row.add_child(spacer_r3)

	_mana_label = Label.new()
	_mana_label.text = "Mana: +0"
	_mana_label.add_theme_color_override("font_color", Color(0.4, 0.6, 1.0))
	res_row.add_child(_mana_label)

	main_vbox.add_child(HSeparator.new())

	# --- Middle section: built | available ---
	var mid_split = HBoxContainer.new()
	mid_split.layout_mode = 2
	mid_split.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(mid_split)

	# Built buildings panel
	var built_panel = VBoxContainer.new()
	built_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	mid_split.add_child(built_panel)

	var built_title = Label.new()
	built_title.text = "Built Buildings"
	built_title.add_theme_font_size_override("font_size", 16)
	built_title.add_theme_color_override("font_color", Color(0.7, 0.9, 0.7))
	built_panel.add_child(built_title)

	var built_scroll = ScrollContainer.new()
	built_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	built_scroll.custom_minimum_size = Vector2(0, 150)
	built_panel.add_child(built_scroll)

	_built_list = VBoxContainer.new()
	_built_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	built_scroll.add_child(_built_list)

	mid_split.add_child(VSeparator.new())

	# Available buildings panel
	var avail_panel = VBoxContainer.new()
	avail_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	mid_split.add_child(avail_panel)

	var avail_title = Label.new()
	avail_title.text = "Available to Build"
	avail_title.add_theme_font_size_override("font_size", 16)
	avail_title.add_theme_color_override("font_color", Color(0.9, 0.8, 0.5))
	avail_panel.add_child(avail_title)

	var avail_scroll = ScrollContainer.new()
	avail_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	avail_scroll.custom_minimum_size = Vector2(0, 150)
	avail_panel.add_child(avail_scroll)

	_available_list = VBoxContainer.new()
	_available_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	avail_scroll.add_child(_available_list)

	main_vbox.add_child(HSeparator.new())

	# --- Build queue ---
	var queue_title = Label.new()
	queue_title.text = "Build Queue"
	queue_title.add_theme_font_size_override("font_size", 16)
	queue_title.add_theme_color_override("font_color", Color(0.6, 0.8, 1.0))
	main_vbox.add_child(queue_title)

	var queue_scroll = ScrollContainer.new()
	queue_scroll.custom_minimum_size = Vector2(0, 80)
	queue_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	main_vbox.add_child(queue_scroll)

	_queue_container = HBoxContainer.new()
	_queue_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	queue_scroll.add_child(_queue_container)

	main_vbox.add_child(HSeparator.new())

	# --- Unit production ---
	var unit_title = Label.new()
	unit_title.text = "Recruit Units"
	unit_title.add_theme_font_size_override("font_size", 16)
	unit_title.add_theme_color_override("font_color", Color(0.9, 0.5, 0.5))
	main_vbox.add_child(unit_title)

	_unit_row = HBoxContainer.new()
	_unit_row.layout_mode = 2
	main_vbox.add_child(_unit_row)
