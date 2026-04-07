extends PanelContainer
## Combat result popup -- shows after auto-resolve combat finishes.
## Displays attacker vs defender, casualties, and outcome.

signal dismissed

var _result: Dictionary = {}
var _tween: Tween

# UI references
var _header_label: Label
var _attacker_col: VBoxContainer
var _defender_col: VBoxContainer
var _result_label: Label
var _stats_label: Label
var _ok_btn: Button


func _ready() -> void:
	visible = false
	_build_ui()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func show_result(result: Dictionary) -> void:
	_result = result
	visible = true
	_populate(result)
	_animate_entrance()


func dismiss() -> void:
	visible = false
	dismissed.emit()


# ---------------------------------------------------------------------------
# Display
# ---------------------------------------------------------------------------

func _populate(result: Dictionary) -> void:
	# Header
	_header_label.text = "Battle Results"

	# Clear columns
	for child in _attacker_col.get_children():
		child.queue_free()
	for child in _defender_col.get_children():
		child.queue_free()

	# Attacker column
	var atk_title := Label.new()
	atk_title.text = "Attacker"
	atk_title.add_theme_font_size_override("font_size", 16)
	atk_title.add_theme_color_override("font_color", Color(0.5, 0.7, 1.0))
	_attacker_col.add_child(atk_title)

	var atk_losses: Array = result.get("attacker_losses", [])
	var atk_army_id: int = result.get("attacker_army_id", -1)
	_populate_army_column(_attacker_col, atk_army_id, atk_losses)

	# Defender column
	var def_title := Label.new()
	def_title.text = "Defender"
	def_title.add_theme_font_size_override("font_size", 16)
	def_title.add_theme_color_override("font_color", Color(1.0, 0.5, 0.5))
	_defender_col.add_child(def_title)

	var def_losses: Array = result.get("defender_losses", [])
	var def_army_id: int = result.get("defender_army_id", -1)
	_populate_army_column(_defender_col, def_army_id, def_losses)

	# Result
	var winner: int = result.get("winner", -1)
	var player_id: int = GameState.current_wizard
	if winner == player_id:
		_result_label.text = "VICTORY!"
		_result_label.add_theme_color_override("font_color", Color(0.3, 1.0, 0.4))
	elif winner >= 0:
		_result_label.text = "DEFEAT!"
		_result_label.add_theme_color_override("font_color", Color(1.0, 0.3, 0.3))
	else:
		_result_label.text = "DRAW"
		_result_label.add_theme_color_override("font_color", Color(0.8, 0.8, 0.4))

	# Stats
	var rounds: int = result.get("rounds", 0)
	var xp: int = result.get("xp_gained", 0)
	_stats_label.text = "Rounds: %d | XP Gained: %d" % [rounds, xp]


func _populate_army_column(col: VBoxContainer, army_id: int, losses: Array) -> void:
	# Show units that were involved -- since combat may have already cleaned up,
	# we show what we can from losses and surviving units
	if losses.is_empty():
		var lbl := Label.new()
		lbl.text = "  No casualties"
		lbl.add_theme_color_override("font_color", Color(0.5, 0.8, 0.5))
		lbl.add_theme_font_size_override("font_size", 13)
		col.add_child(lbl)
	else:
		for uid in losses:
			var lbl := Label.new()
			lbl.text = "  [X] Unit #%d - Killed" % uid
			lbl.add_theme_color_override("font_color", Color(0.8, 0.3, 0.3))
			lbl.add_theme_font_size_override("font_size", 13)
			lbl.name = "loss_%d" % uid
			col.add_child(lbl)

	# Show survivors from the army if it still exists
	var army := UnitManager.get_army(army_id)
	if not army.is_empty():
		for uid in army.get("unit_ids", []):
			var unit := UnitManager.get_unit(uid)
			if unit.is_empty():
				continue
			var hp_pct := float(unit.get("current_hp", 0)) / float(maxi(1, unit.get("max_hp", 1))) * 100.0

			var hbox := HBoxContainer.new()
			col.add_child(hbox)

			var name_lbl := Label.new()
			name_lbl.text = "  %s" % unit.get("spec_id", "unit")
			name_lbl.add_theme_font_size_override("font_size", 13)
			hbox.add_child(name_lbl)

			var hp_bar := ProgressBar.new()
			hp_bar.custom_minimum_size = Vector2(60, 14)
			hp_bar.max_value = unit.get("max_hp", 1)
			hp_bar.value = unit.get("current_hp", 0)
			hp_bar.show_percentage = false
			hbox.add_child(hp_bar)

			var hp_lbl := Label.new()
			hp_lbl.text = "%.0f%%" % hp_pct
			hp_lbl.add_theme_font_size_override("font_size", 11)
			hbox.add_child(hp_lbl)


# ---------------------------------------------------------------------------
# Animation
# ---------------------------------------------------------------------------

func _animate_entrance() -> void:
	modulate.a = 0.0
	scale = Vector2(0.8, 0.8)

	if _tween:
		_tween.kill()
	_tween = create_tween()
	_tween.set_parallel(true)
	_tween.tween_property(self, "modulate:a", 1.0, 0.3)
	_tween.tween_property(self, "scale", Vector2.ONE, 0.3).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_BACK)

	# Animate casualty labels fading
	await get_tree().create_timer(0.5).timeout
	_animate_casualties(_attacker_col)
	_animate_casualties(_defender_col)


func _animate_casualties(col: VBoxContainer) -> void:
	for child in col.get_children():
		if child.name.begins_with("loss_"):
			var t := create_tween()
			t.tween_property(child, "modulate:a", 0.4, 0.8)


# ---------------------------------------------------------------------------
# UI Construction
# ---------------------------------------------------------------------------

func _build_ui() -> void:
	custom_minimum_size = Vector2(500, 380)
	anchors_preset = PRESET_CENTER
	anchor_left = 0.5
	anchor_top = 0.5
	anchor_right = 0.5
	anchor_bottom = 0.5
	offset_left = -250
	offset_top = -190
	offset_right = 250
	offset_bottom = 190

	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.10, 0.08, 0.12, 0.96)
	style.border_color = Color(0.6, 0.3, 0.3)
	style.set_border_width_all(2)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(12)
	add_theme_stylebox_override("panel", style)

	var main_vbox := VBoxContainer.new()
	main_vbox.layout_mode = 2
	add_child(main_vbox)

	# Header
	_header_label = Label.new()
	_header_label.text = "Battle Results"
	_header_label.add_theme_font_size_override("font_size", 22)
	_header_label.add_theme_color_override("font_color", Color(1.0, 0.85, 0.5))
	_header_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	main_vbox.add_child(_header_label)

	main_vbox.add_child(HSeparator.new())

	# Two columns
	var columns := HBoxContainer.new()
	columns.layout_mode = 2
	columns.size_flags_vertical = Control.SIZE_EXPAND_FILL
	main_vbox.add_child(columns)

	var atk_scroll := ScrollContainer.new()
	atk_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	columns.add_child(atk_scroll)

	_attacker_col = VBoxContainer.new()
	_attacker_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	atk_scroll.add_child(_attacker_col)

	columns.add_child(VSeparator.new())

	var def_scroll := ScrollContainer.new()
	def_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	columns.add_child(def_scroll)

	_defender_col = VBoxContainer.new()
	_defender_col.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	def_scroll.add_child(_defender_col)

	main_vbox.add_child(HSeparator.new())

	# Result text
	_result_label = Label.new()
	_result_label.add_theme_font_size_override("font_size", 28)
	_result_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	main_vbox.add_child(_result_label)

	# Stats
	_stats_label = Label.new()
	_stats_label.add_theme_font_size_override("font_size", 14)
	_stats_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_stats_label.add_theme_color_override("font_color", Color(0.7, 0.7, 0.7))
	main_vbox.add_child(_stats_label)

	main_vbox.add_child(HSeparator.new())

	# OK button
	_ok_btn = Button.new()
	_ok_btn.text = "OK"
	_ok_btn.custom_minimum_size = Vector2(100, 36)
	_ok_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_ok_btn.pressed.connect(dismiss)
	main_vbox.add_child(_ok_btn)
