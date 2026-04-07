extends PanelContainer
## Spell Book UI -- browse spells by realm, research new spells, cast known ones.
## Integrates with MagicSystem autoload for wizard state.

signal closed

# Realm display config: id -> {name, color}
const REALM_CONFIG: Dictionary = {
	"arcane":  {"name": "Arcane",  "color": Color(0.6, 0.5, 1.0)},
	"nature":  {"name": "Nature",  "color": Color(0.3, 0.8, 0.3)},
	"shadow":  {"name": "Death",   "color": Color(0.5, 0.2, 0.5)},
	"fire":    {"name": "Chaos",   "color": Color(1.0, 0.3, 0.2)},
	"ice":     {"name": "Sorcery", "color": Color(0.3, 0.7, 1.0)},
	"life":    {"name": "Life",    "color": Color(1.0, 1.0, 0.6)},
	"magma":   {"name": "Magma",   "color": Color(0.9, 0.4, 0.1)},
	"glamour": {"name": "Glamour", "color": Color(0.9, 0.5, 0.8)},
	"binding": {"name": "Binding", "color": Color(0.6, 0.6, 0.6)},
	"spirit":  {"name": "Spirit",  "color": Color(0.7, 0.9, 1.0)},
}

# All realm keys for tab ordering
const REALM_ORDER: Array = [
	"arcane", "nature", "shadow", "fire", "ice",
	"life", "magma", "glamour", "binding", "spirit",
]

var _active_realm: String = "arcane"
var _selected_spell_id: String = ""
var _wizard_id: int = 0

# UI references
var _tab_container: HBoxContainer
var _spell_grid: GridContainer
var _detail_panel: VBoxContainer
var _detail_name: Label
var _detail_realm: Label
var _detail_tier: Label
var _detail_cost: Label
var _detail_research_cost: Label
var _detail_cast_time: Label
var _detail_scope: Label
var _detail_desc: RichTextLabel
var _research_btn: Button
var _cast_btn: Button
var _research_bar: ProgressBar
var _research_label: Label
var _mana_label: Label
var _mana_rate_label: Label
var _allocation_slider: HSlider
var _allocation_label: Label
var _spell_icon_rect: ColorRect


func _ready() -> void:
	visible = false
	_build_ui()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func open_spell_book() -> void:
	_wizard_id = GameState.current_wizard
	visible = true
	_active_realm = "arcane"
	_selected_spell_id = ""
	_refresh_tabs()
	_refresh_spell_grid()
	_refresh_research_bar()
	_refresh_mana_display()
	_clear_detail()


func close_spell_book() -> void:
	visible = false
	closed.emit()


# ---------------------------------------------------------------------------
# Tab handling
# ---------------------------------------------------------------------------

func on_realm_tab_clicked(realm: String) -> void:
	_active_realm = realm
	_selected_spell_id = ""
	_refresh_tabs()
	_refresh_spell_grid()
	_clear_detail()


func _refresh_tabs() -> void:
	for child in _tab_container.get_children():
		if child is Button:
			var realm_key: String = child.get_meta("realm", "")
			var cfg: Dictionary = REALM_CONFIG.get(realm_key, {})
			if realm_key == _active_realm:
				child.modulate = Color.WHITE
				child.flat = false
			else:
				child.modulate = Color(0.6, 0.6, 0.6)
				child.flat = true


# ---------------------------------------------------------------------------
# Spell grid
# ---------------------------------------------------------------------------

func _refresh_spell_grid() -> void:
	for child in _spell_grid.get_children():
		child.queue_free()

	var state := MagicSystem.get_magic_state(_wizard_id)
	var known_spells: Array = state.get("known_spells", [])
	var researchable := MagicSystem.get_researchable_spells(_wizard_id)

	# Get all spells for this realm, sorted by tier
	var realm_spells := DataLoader.get_spells_by_realm(_active_realm)
	realm_spells.sort_custom(func(a, b): return a.get("tier", 1) < b.get("tier", 1))

	var current_tier: int = 0
	for spell in realm_spells:
		var tier: int = spell.get("tier", 1)
		var sid: String = spell.get("id", "")

		# Add tier header if new tier
		if tier != current_tier:
			current_tier = tier
			var tier_label := Label.new()
			tier_label.text = "-- Tier %d --" % tier
			tier_label.add_theme_font_size_override("font_size", 13)
			tier_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.5))
			tier_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			# span full grid width: add placeholder cells
			_spell_grid.add_child(tier_label)
			for _p in range(_spell_grid.columns - 1):
				var spacer := Control.new()
				_spell_grid.add_child(spacer)

		var is_known: bool = sid in known_spells
		var is_researchable: bool = sid in researchable

		var spell_btn := Button.new()
		spell_btn.custom_minimum_size = Vector2(110, 60)
		spell_btn.text = spell.get("name", sid)
		spell_btn.add_theme_font_size_override("font_size", 12)

		# Color coding
		if is_known:
			spell_btn.modulate = Color(1.0, 1.0, 1.0)
			spell_btn.tooltip_text = "Known - %d mana" % spell.get("cost", 0)
		elif is_researchable:
			spell_btn.modulate = Color(0.7, 0.8, 1.0)
			spell_btn.tooltip_text = "Researchable"
			# Glow border for researchable
			var glow_style := StyleBoxFlat.new()
			glow_style.bg_color = Color(0.15, 0.15, 0.25)
			glow_style.border_color = Color(0.4, 0.6, 1.0, 0.8)
			glow_style.set_border_width_all(2)
			glow_style.set_corner_radius_all(4)
			spell_btn.add_theme_stylebox_override("normal", glow_style)
		else:
			spell_btn.modulate = Color(0.35, 0.35, 0.35)
			spell_btn.tooltip_text = "Not available"
			spell_btn.disabled = true

		var captured_id := sid
		spell_btn.pressed.connect(func(): on_spell_selected(captured_id))
		_spell_grid.add_child(spell_btn)


# ---------------------------------------------------------------------------
# Spell detail
# ---------------------------------------------------------------------------

func on_spell_selected(spell_id: String) -> void:
	_selected_spell_id = spell_id
	var spell := DataLoader.get_spell(spell_id)
	if spell.is_empty():
		_clear_detail()
		return

	var state := MagicSystem.get_magic_state(_wizard_id)
	var is_known: bool = spell_id in state.get("known_spells", [])
	var is_researchable: bool = spell_id in MagicSystem.get_researchable_spells(_wizard_id)

	_detail_name.text = spell.get("name", spell_id)

	var cfg: Dictionary = REALM_CONFIG.get(spell.get("realm", ""), {})
	_detail_realm.text = "Realm: %s" % cfg.get("name", spell.get("realm", "?"))
	_detail_realm.add_theme_color_override("font_color", cfg.get("color", Color.WHITE))

	_detail_tier.text = "Tier: %d" % spell.get("tier", 1)
	_detail_cost.text = "Mana Cost: %d" % spell.get("cost", 0)
	_detail_cast_time.text = "Casting Time: %d turn(s)" % spell.get("time", 1)
	_detail_scope.text = "Scope: %s" % spell.get("scope", "?")

	if is_known:
		_detail_research_cost.text = "Status: Known"
		_detail_research_cost.add_theme_color_override("font_color", Color(0.4, 1.0, 0.4))
	else:
		_detail_research_cost.text = "Research Cost: %d" % spell.get("research", 0)
		_detail_research_cost.add_theme_color_override("font_color", Color(0.8, 0.8, 0.8))

	_detail_desc.text = spell.get("desc", "No description available.")

	# Spell icon placeholder (colored rect based on realm)
	_spell_icon_rect.color = cfg.get("color", Color(0.3, 0.3, 0.3))
	_spell_icon_rect.visible = true

	# Research button
	_research_btn.visible = not is_known and is_researchable
	_research_btn.disabled = state.get("research_spell", "") != ""

	# Cast button
	_cast_btn.visible = is_known
	_cast_btn.disabled = not MagicSystem.can_cast_spell(_wizard_id, spell_id)

	_detail_panel.visible = true


func _clear_detail() -> void:
	_detail_panel.visible = false


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

func on_research_clicked() -> void:
	if _selected_spell_id == "":
		return
	MagicSystem.start_research(_wizard_id, _selected_spell_id)
	_refresh_spell_grid()
	_refresh_research_bar()
	on_spell_selected(_selected_spell_id)


func on_cast_clicked() -> void:
	if _selected_spell_id == "":
		return
	# For MVP, cast with null target; game can prompt for target later
	var success := MagicSystem.cast_spell(_wizard_id, _selected_spell_id, null)
	if success:
		_refresh_mana_display()
		on_spell_selected(_selected_spell_id)


func on_allocation_changed(value: float) -> void:
	# Update the label to show the split
	var research_pct := int(value)
	_allocation_label.text = "Research: %d%% / Savings: %d%%" % [research_pct, 100 - research_pct]
	# Allocation logic would be applied to MagicSystem if extended


# ---------------------------------------------------------------------------
# Research and mana bars
# ---------------------------------------------------------------------------

func _refresh_research_bar() -> void:
	var state := MagicSystem.get_magic_state(_wizard_id)
	var research_spell: String = state.get("research_spell", "")
	if research_spell == "":
		_research_bar.visible = false
		_research_label.text = "No active research"
		return

	_research_bar.visible = true
	var spell := DataLoader.get_spell(research_spell)
	var cost: int = spell.get("research", 50)
	var progress: int = state.get("research_progress", 0)
	_research_bar.max_value = cost
	_research_bar.value = progress

	var pct := 0
	if cost > 0:
		pct = int(float(progress) / float(cost) * 100.0)
	_research_label.text = "Researching: %s (%d%%)" % [spell.get("name", research_spell), pct]


func _refresh_mana_display() -> void:
	var state := MagicSystem.get_magic_state(_wizard_id)
	var current: int = state.get("current_mana", 0)
	var max_mana: int = state.get("max_mana", 100)
	var per_turn: int = state.get("mana_per_turn", 0)
	_mana_label.text = "Mana: %d / %d" % [current, max_mana]
	_mana_rate_label.text = "+%d per turn" % per_turn


# ---------------------------------------------------------------------------
# UI Construction
# ---------------------------------------------------------------------------

func _build_ui() -> void:
	custom_minimum_size = Vector2(900, 600)
	anchors_preset = PRESET_CENTER
	anchor_left = 0.5
	anchor_top = 0.5
	anchor_right = 0.5
	anchor_bottom = 0.5
	offset_left = -450
	offset_top = -300
	offset_right = 450
	offset_bottom = 300

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.06, 0.14, 0.96)
	style.border_color = Color(0.5, 0.4, 0.8)
	style.set_border_width_all(2)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(10)
	add_theme_stylebox_override("panel", style)

	var main_vbox := VBoxContainer.new()
	main_vbox.layout_mode = 2
	add_child(main_vbox)

	# --- Title + close ---
	var title_row := HBoxContainer.new()
	title_row.layout_mode = 2
	main_vbox.add_child(title_row)

	var title := Label.new()
	title.text = "Spell Book"
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", Color(0.8, 0.7, 1.0))
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	title_row.add_child(title)

	var close_btn := Button.new()
	close_btn.text = "X"
	close_btn.custom_minimum_size = Vector2(32, 32)
	close_btn.pressed.connect(close_spell_book)
	title_row.add_child(close_btn)

	# --- Realm tabs ---
	_tab_container = HBoxContainer.new()
	_tab_container.layout_mode = 2
	main_vbox.add_child(_tab_container)

	for realm_key in REALM_ORDER:
		var cfg: Dictionary = REALM_CONFIG.get(realm_key, {})
		var tab_btn := Button.new()
		tab_btn.text = cfg.get("name", realm_key)
		tab_btn.custom_minimum_size = Vector2(75, 30)
		tab_btn.add_theme_font_size_override("font_size", 12)
		tab_btn.set_meta("realm", realm_key)

		# Color the button text
		var btn_color: Color = cfg.get("color", Color.WHITE)
		tab_btn.add_theme_color_override("font_color", btn_color)
		tab_btn.add_theme_color_override("font_hover_color", btn_color.lightened(0.3))

		var captured_realm := realm_key
		tab_btn.pressed.connect(func(): on_realm_tab_clicked(captured_realm))
		_tab_container.add_child(tab_btn)

	main_vbox.add_child(HSeparator.new())

	# --- Content: spell grid (left) + detail panel (right) ---
	var content := HBoxContainer.new()
	content.layout_mode = 2
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(content)

	# Spell grid in a scroll container
	var grid_scroll := ScrollContainer.new()
	grid_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid_scroll.size_flags_stretch_ratio = 1.5
	content.add_child(grid_scroll)

	_spell_grid = GridContainer.new()
	_spell_grid.columns = 4
	_spell_grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	grid_scroll.add_child(_spell_grid)

	content.add_child(VSeparator.new())

	# Detail panel
	var detail_scroll := ScrollContainer.new()
	detail_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	detail_scroll.size_flags_stretch_ratio = 1.0
	content.add_child(detail_scroll)

	_detail_panel = VBoxContainer.new()
	_detail_panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_detail_panel.visible = false
	detail_scroll.add_child(_detail_panel)

	# Spell icon placeholder
	_spell_icon_rect = ColorRect.new()
	_spell_icon_rect.custom_minimum_size = Vector2(80, 80)
	_spell_icon_rect.color = Color(0.3, 0.3, 0.3)
	_spell_icon_rect.visible = false
	_detail_panel.add_child(_spell_icon_rect)

	_detail_name = Label.new()
	_detail_name.add_theme_font_size_override("font_size", 20)
	_detail_name.add_theme_color_override("font_color", Color(1.0, 0.95, 0.7))
	_detail_panel.add_child(_detail_name)

	_detail_realm = Label.new()
	_detail_realm.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_realm)

	_detail_tier = Label.new()
	_detail_tier.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_tier)

	_detail_cost = Label.new()
	_detail_cost.add_theme_font_size_override("font_size", 14)
	_detail_cost.add_theme_color_override("font_color", Color(0.5, 0.7, 1.0))
	_detail_panel.add_child(_detail_cost)

	_detail_research_cost = Label.new()
	_detail_research_cost.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_research_cost)

	_detail_cast_time = Label.new()
	_detail_cast_time.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_cast_time)

	_detail_scope = Label.new()
	_detail_scope.add_theme_font_size_override("font_size", 14)
	_detail_panel.add_child(_detail_scope)

	_detail_panel.add_child(HSeparator.new())

	_detail_desc = RichTextLabel.new()
	_detail_desc.custom_minimum_size = Vector2(0, 60)
	_detail_desc.fit_content = true
	_detail_desc.bbcode_enabled = false
	_detail_desc.scroll_active = false
	_detail_desc.add_theme_font_size_override("normal_font_size", 13)
	_detail_panel.add_child(_detail_desc)

	_detail_panel.add_child(HSeparator.new())

	# Action buttons
	_research_btn = Button.new()
	_research_btn.text = "Research"
	_research_btn.custom_minimum_size = Vector2(120, 36)
	_research_btn.visible = false
	_research_btn.pressed.connect(on_research_clicked)
	_detail_panel.add_child(_research_btn)

	_cast_btn = Button.new()
	_cast_btn.text = "Cast Spell"
	_cast_btn.custom_minimum_size = Vector2(120, 36)
	_cast_btn.visible = false
	_cast_btn.pressed.connect(on_cast_clicked)
	_detail_panel.add_child(_cast_btn)

	main_vbox.add_child(HSeparator.new())

	# --- Bottom bar: research progress + mana ---
	var bottom := HBoxContainer.new()
	bottom.layout_mode = 2
	main_vbox.add_child(bottom)

	# Research progress
	var research_vbox := VBoxContainer.new()
	research_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bottom.add_child(research_vbox)

	_research_label = Label.new()
	_research_label.text = "No active research"
	_research_label.add_theme_font_size_override("font_size", 13)
	research_vbox.add_child(_research_label)

	_research_bar = ProgressBar.new()
	_research_bar.custom_minimum_size = Vector2(200, 18)
	_research_bar.show_percentage = false
	_research_bar.visible = false
	research_vbox.add_child(_research_bar)

	bottom.add_child(VSeparator.new())

	# Mana display
	var mana_vbox := VBoxContainer.new()
	mana_vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bottom.add_child(mana_vbox)

	_mana_label = Label.new()
	_mana_label.text = "Mana: 0 / 0"
	_mana_label.add_theme_font_size_override("font_size", 14)
	_mana_label.add_theme_color_override("font_color", Color(0.4, 0.6, 1.0))
	mana_vbox.add_child(_mana_label)

	_mana_rate_label = Label.new()
	_mana_rate_label.text = "+0 per turn"
	_mana_rate_label.add_theme_font_size_override("font_size", 12)
	mana_vbox.add_child(_mana_rate_label)

	# Allocation slider
	var slider_row := HBoxContainer.new()
	mana_vbox.add_child(slider_row)

	var slider_label := Label.new()
	slider_label.text = "Alloc:"
	slider_label.add_theme_font_size_override("font_size", 12)
	slider_row.add_child(slider_label)

	_allocation_slider = HSlider.new()
	_allocation_slider.min_value = 0
	_allocation_slider.max_value = 100
	_allocation_slider.value = 50
	_allocation_slider.custom_minimum_size = Vector2(120, 20)
	_allocation_slider.value_changed.connect(on_allocation_changed)
	slider_row.add_child(_allocation_slider)

	_allocation_label = Label.new()
	_allocation_label.text = "Research: 50% / Savings: 50%"
	_allocation_label.add_theme_font_size_override("font_size", 11)
	slider_row.add_child(_allocation_label)
