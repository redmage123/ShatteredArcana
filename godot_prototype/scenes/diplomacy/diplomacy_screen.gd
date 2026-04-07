extends PanelContainer
## Diplomacy overview -- view and manage relations with rival wizards.
## Shows reputation, treaties, and diplomatic actions.

signal closed

var _selected_wizard_id: int = -1
var _player_id: int = 0

# UI references
var _wizard_list: VBoxContainer
var _detail_panel: VBoxContainer
var _detail_name: Label
var _detail_realm: Label
var _reputation_bar: ProgressBar
var _reputation_label: Label
var _treaty_label: Label
var _war_btn: Button
var _peace_btn: Button
var _alliance_btn: Button
var _gift_btn: Button
var _events_log: VBoxContainer
var _your_strength: Label
var _your_cities: Label
var _your_mana: Label
var _your_allies: Label


func _ready() -> void:
	visible = false
	_build_ui()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func open_diplomacy() -> void:
	_player_id = GameState.current_wizard
	visible = true
	_selected_wizard_id = -1
	_refresh_wizard_list()
	_refresh_your_status()
	_detail_panel.visible = false


func close_diplomacy() -> void:
	visible = false
	closed.emit()


# ---------------------------------------------------------------------------
# Wizard list
# ---------------------------------------------------------------------------

func _refresh_wizard_list() -> void:
	for child in _wizard_list.get_children():
		child.queue_free()

	for wiz in GameState.wizards:
		var wid: int = wiz.get("id", -1)
		if wid == _player_id:
			continue
		if wiz.get("is_eliminated", false):
			continue

		var rel = DiplomacySystem.get_relation(_player_id, wid)
		var status_icon: String
		if rel.get("at_war", false):
			status_icon = "[WAR]"
		elif rel.get("allied", false):
			status_icon = "[ALLY]"
		elif rel.get("reputation", 0) < -20:
			status_icon = "[HOSTILE]"
		else:
			status_icon = "[PEACE]"

		var btn = Button.new()
		btn.text = "%s %s" % [status_icon, wiz.get("name", "Wizard %d" % wid)]
		btn.alignment = HORIZONTAL_ALIGNMENT_LEFT
		btn.custom_minimum_size = Vector2(200, 36)

		# Color by relation
		if rel.get("at_war", false):
			btn.add_theme_color_override("font_color", Color(1.0, 0.3, 0.3))
		elif rel.get("allied", false):
			btn.add_theme_color_override("font_color", Color(0.3, 1.0, 0.5))
		else:
			btn.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))

		if wid == _selected_wizard_id:
			btn.flat = false
		else:
			btn.flat = true

		var captured_id = wid
		btn.pressed.connect(func(): on_wizard_selected(captured_id))
		_wizard_list.add_child(btn)


# ---------------------------------------------------------------------------
# Detail view
# ---------------------------------------------------------------------------

func on_wizard_selected(wizard_id: int) -> void:
	_selected_wizard_id = wizard_id
	_refresh_wizard_list()

	var wiz = GameState.get_wizard(wizard_id)
	if wiz.is_empty():
		_detail_panel.visible = false
		return

	_detail_panel.visible = true
	_detail_name.text = wiz.get("name", "Unknown")
	_detail_realm.text = "Realm: %s" % wiz.get("realm", "?")

	var rel = DiplomacySystem.get_relation(_player_id, wizard_id)
	var rep: int = rel.get("reputation", 0)

	# Reputation bar: -100 to +100 mapped to 0-200
	_reputation_bar.max_value = 200
	_reputation_bar.value = rep + 100

	# Color the bar
	var bar_style = StyleBoxFlat.new()
	if rep < -30:
		bar_style.bg_color = Color(0.8, 0.2, 0.2)
	elif rep < 20:
		bar_style.bg_color = Color(0.8, 0.7, 0.2)
	else:
		bar_style.bg_color = Color(0.2, 0.8, 0.3)
	bar_style.set_corner_radius_all(3)
	_reputation_bar.add_theme_stylebox_override("fill", bar_style)
	_reputation_label.text = "Reputation: %d" % rep

	# Treaty status
	if rel.get("at_war", false):
		_treaty_label.text = "Status: AT WAR"
		_treaty_label.add_theme_color_override("font_color", Color(1.0, 0.3, 0.3))
	elif rel.get("allied", false):
		_treaty_label.text = "Status: ALLIED"
		_treaty_label.add_theme_color_override("font_color", Color(0.3, 1.0, 0.5))
	else:
		_treaty_label.text = "Status: Peace"
		_treaty_label.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))

	# Button states
	_war_btn.disabled = rel.get("at_war", false)
	_peace_btn.disabled = not rel.get("at_war", false)
	_alliance_btn.disabled = rel.get("at_war", false) or rel.get("allied", false) or rep < Constants.RELATION_ALLIANCE_THRESHOLD
	_gift_btn.disabled = rel.get("at_war", false)

	if not _alliance_btn.disabled:
		_alliance_btn.tooltip_text = ""
	elif rep < Constants.RELATION_ALLIANCE_THRESHOLD:
		_alliance_btn.tooltip_text = "Reputation too low (need %d)" % Constants.RELATION_ALLIANCE_THRESHOLD


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

func on_declare_war(wizard_id: int) -> void:
	DiplomacySystem.declare_war(_player_id, wizard_id)
	on_wizard_selected(wizard_id)
	_refresh_wizard_list()


func on_propose_peace(wizard_id: int) -> void:
	var accepted = DiplomacySystem.propose_peace(_player_id, wizard_id)
	_add_event("Peace proposal %s" % ("accepted!" if accepted else "rejected."))
	on_wizard_selected(wizard_id)
	_refresh_wizard_list()


func on_propose_alliance(wizard_id: int) -> void:
	var accepted = DiplomacySystem.propose_alliance(_player_id, wizard_id)
	_add_event("Alliance proposal %s" % ("accepted!" if accepted else "rejected."))
	on_wizard_selected(wizard_id)
	_refresh_wizard_list()


func on_send_gift() -> void:
	if _selected_wizard_id < 0:
		return
	var player_gold: int = GameState.get_wizard(_player_id).get("gold", 0)
	if player_gold < 50:
		_add_event("Not enough gold to send a gift (need 50).")
		return
	GameState.add_gold(_player_id, -50)
	DiplomacySystem.modify_reputation(_player_id, _selected_wizard_id, 10, "gift")
	_add_event("Sent 50 gold as gift. Reputation improved.")
	on_wizard_selected(_selected_wizard_id)


func _add_event(text: String) -> void:
	var lbl = Label.new()
	lbl.text = "- %s" % text
	lbl.add_theme_font_size_override("font_size", 12)
	lbl.add_theme_color_override("font_color", Color(0.9, 0.9, 0.7))
	_events_log.add_child(lbl)
	while _events_log.get_child_count() > 8:
		_events_log.get_child(0).queue_free()


# ---------------------------------------------------------------------------
# Your status
# ---------------------------------------------------------------------------

func _refresh_your_status() -> void:
	var wiz = GameState.get_wizard(_player_id)
	var cities = CityManager.get_cities_for_wizard(_player_id)
	var armies = UnitManager.get_armies_for_wizard(_player_id)

	# Military strength estimate
	var strength: float = 0.0
	for army in armies:
		strength += CombatResolver.calculate_army_power(army.get("army_id", -1))
	_your_strength.text = "Military: %.0f" % strength
	_your_cities.text = "Cities: %d" % cities.size()
	_your_mana.text = "Mana: %d" % wiz.get("mana", 0)

	# Allies list
	var allies: Array = []
	for other_wiz in GameState.wizards:
		var oid: int = other_wiz.get("id", -1)
		if oid == _player_id or other_wiz.get("is_eliminated", false):
			continue
		if DiplomacySystem.are_allied(_player_id, oid):
			allies.append(other_wiz.get("name", "?"))
	_your_allies.text = "Allies: %s" % (", ".join(allies) if not allies.is_empty() else "None")


# ---------------------------------------------------------------------------
# UI Construction
# ---------------------------------------------------------------------------

func _build_ui() -> void:
	custom_minimum_size = Vector2(700, 450)
	anchors_preset = PRESET_CENTER
	anchor_left = 0.5
	anchor_top = 0.5
	anchor_right = 0.5
	anchor_bottom = 0.5
	offset_left = -350
	offset_top = -225
	offset_right = 350
	offset_bottom = 225

	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.10, 0.10, 0.15, 0.95)
	style.border_color = Color(0.5, 0.45, 0.3)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_content_margin_all(10)
	add_theme_stylebox_override("panel", style)

	var main_vbox = VBoxContainer.new()
	main_vbox.layout_mode = 2
	add_child(main_vbox)

	# Title + close
	var title_row = HBoxContainer.new()
	title_row.layout_mode = 2
	main_vbox.add_child(title_row)

	var title = Label.new()
	title.text = "Diplomacy"
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color(1.0, 0.9, 0.6))
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title_row.add_child(title)

	var close_btn = Button.new()
	close_btn.text = "X"
	close_btn.custom_minimum_size = Vector2(32, 32)
	close_btn.pressed.connect(close_diplomacy)
	title_row.add_child(close_btn)

	main_vbox.add_child(HSeparator.new())

	# Content: wizard list (left) | detail (right)
	var content = HBoxContainer.new()
	content.layout_mode = 2
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(content)

	# Wizard list
	var list_scroll = ScrollContainer.new()
	list_scroll.custom_minimum_size = Vector2(220, 0)
	content.add_child(list_scroll)

	_wizard_list = VBoxContainer.new()
	_wizard_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list_scroll.add_child(_wizard_list)

	content.add_child(VSeparator.new())

	# Detail panel
	var detail_scroll = ScrollContainer.new()
	detail_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	content.add_child(detail_scroll)

	_detail_panel = VBoxContainer.new()
	_detail_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_detail_panel.visible = false
	detail_scroll.add_child(_detail_panel)

	_detail_name = Label.new()
	_detail_name.add_theme_font_size_override("font_size", 20)
	_detail_name.add_theme_color_override("font_color", Color(1.0, 0.95, 0.7))
	_detail_panel.add_child(_detail_name)

	_detail_realm = Label.new()
	_detail_realm.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_realm)

	_detail_panel.add_child(HSeparator.new())

	_reputation_label = Label.new()
	_reputation_label.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_reputation_label)

	_reputation_bar = ProgressBar.new()
	_reputation_bar.custom_minimum_size = Vector2(250, 20)
	_reputation_bar.show_percentage = false
	_detail_panel.add_child(_reputation_bar)

	_treaty_label = Label.new()
	_treaty_label.add_theme_font_size_override("font_size", 16)
	_detail_panel.add_child(_treaty_label)

	_detail_panel.add_child(HSeparator.new())

	# Action buttons
	var action_row = HBoxContainer.new()
	_detail_panel.add_child(action_row)

	_war_btn = Button.new()
	_war_btn.text = "Declare War"
	_war_btn.custom_minimum_size = Vector2(100, 32)
	_war_btn.pressed.connect(func(): on_declare_war(_selected_wizard_id))
	action_row.add_child(_war_btn)

	_peace_btn = Button.new()
	_peace_btn.text = "Propose Peace"
	_peace_btn.custom_minimum_size = Vector2(100, 32)
	_peace_btn.pressed.connect(func(): on_propose_peace(_selected_wizard_id))
	action_row.add_child(_peace_btn)

	_alliance_btn = Button.new()
	_alliance_btn.text = "Alliance"
	_alliance_btn.custom_minimum_size = Vector2(100, 32)
	_alliance_btn.pressed.connect(func(): on_propose_alliance(_selected_wizard_id))
	action_row.add_child(_alliance_btn)

	_gift_btn = Button.new()
	_gift_btn.text = "Send Gift"
	_gift_btn.custom_minimum_size = Vector2(100, 32)
	_gift_btn.pressed.connect(on_send_gift)
	action_row.add_child(_gift_btn)

	_detail_panel.add_child(HSeparator.new())

	# Events log
	var events_title = Label.new()
	events_title.text = "Recent Events"
	events_title.add_theme_font_size_override("font_size", 14)
	events_title.add_theme_color_override("font_color", Color(0.7, 0.7, 0.5))
	_detail_panel.add_child(events_title)

	_events_log = VBoxContainer.new()
	_detail_panel.add_child(_events_log)

	main_vbox.add_child(HSeparator.new())

	# Your status bar
	var status_row = HBoxContainer.new()
	status_row.layout_mode = 2
	main_vbox.add_child(status_row)

	var status_title = Label.new()
	status_title.text = "Your Status: "
	status_title.add_theme_font_size_override("font_size", 13)
	status_title.add_theme_color_override("font_color", Color(0.6, 0.8, 1.0))
	status_row.add_child(status_title)

	_your_strength = Label.new()
	_your_strength.text = "Military: 0"
	_your_strength.add_theme_font_size_override("font_size", 13)
	status_row.add_child(_your_strength)

	var sp1 = Control.new()
	sp1.custom_minimum_size = Vector2(15, 0)
	status_row.add_child(sp1)

	_your_cities = Label.new()
	_your_cities.text = "Cities: 0"
	_your_cities.add_theme_font_size_override("font_size", 13)
	status_row.add_child(_your_cities)

	var sp2 = Control.new()
	sp2.custom_minimum_size = Vector2(15, 0)
	status_row.add_child(sp2)

	_your_mana = Label.new()
	_your_mana.text = "Mana: 0"
	_your_mana.add_theme_font_size_override("font_size", 13)
	status_row.add_child(_your_mana)

	var sp3 = Control.new()
	sp3.custom_minimum_size = Vector2(15, 0)
	status_row.add_child(sp3)

	_your_allies = Label.new()
	_your_allies.text = "Allies: None"
	_your_allies.add_theme_font_size_override("font_size", 13)
	status_row.add_child(_your_allies)
