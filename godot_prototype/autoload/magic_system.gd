extends Node
## Simplified port of CoMMagicSubsystem.
## Manages mana, spell research, spell casting, and mana nodes.

var wizard_magic: Dictionary = {}  # wizard_id -> magic state dict


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Initialization
# ---------------------------------------------------------------------------

func initialize_for_wizard(wizard_id: int, realm: String) -> void:
	wizard_magic[wizard_id] = {
		"current_mana": 50,
		"max_mana": 200,
		"mana_per_turn": 5,
		"known_spells": [],
		"research_spell": "",
		"research_progress": 0,
		"controlled_nodes": [],   # Array of {plane, pos}
		"casting_skill": Constants.BASE_CASTING_SKILL,
		"spell_books": _initial_spell_books(realm),
	}


func _initial_spell_books(primary_realm: String) -> Dictionary:
	var books: Dictionary = {
		Constants.REALM_ARCANE: 0,
		Constants.REALM_NATURE: 0,
		Constants.REALM_SHADOW: 0,
		Constants.REALM_FIRE: 0,
		Constants.REALM_ICE: 0,
	}
	# Primary realm gets 3 books, secondary gets 1
	books[primary_realm] = 3
	var realms = books.keys()
	for r in realms:
		if r != primary_realm:
			books[r] = 1
			break
	return books


# ---------------------------------------------------------------------------
# Queries
# ---------------------------------------------------------------------------

func get_magic_state(wizard_id: int) -> Dictionary:
	return wizard_magic.get(wizard_id, {})


func can_cast_spell(wizard_id: int, spell_id: String) -> bool:
	var state = get_magic_state(wizard_id)
	if state.is_empty():
		return false
	if spell_id not in state["known_spells"]:
		return false
	var spell = DataLoader.get_spell(spell_id)
	if spell.is_empty():
		return false
	var mana_cost: int = spell.get("mana_cost", 10)
	return state["current_mana"] >= mana_cost


func get_available_spells(wizard_id: int) -> Array:
	var state = get_magic_state(wizard_id)
	if state.is_empty():
		return []
	var result: Array = []
	for sid in state["known_spells"]:
		if can_cast_spell(wizard_id, sid):
			result.append(sid)
	return result


func get_researchable_spells(wizard_id: int) -> Array:
	var state = get_magic_state(wizard_id)
	if state.is_empty():
		return []
	var result: Array = []
	for spell in DataLoader.spells:
		var sid: String = spell.get("id", "")
		if sid in state["known_spells"]:
			continue  # already known
		var realm: String = spell.get("realm", "")
		var required_books: int = spell.get("required_books", 1)
		if state["spell_books"].get(realm, 0) >= required_books:
			result.append(sid)
	return result


# ---------------------------------------------------------------------------
# Mana nodes
# ---------------------------------------------------------------------------

func claim_mana_node(wizard_id: int, plane: int, pos: Vector2i) -> bool:
	var tile = WorldMap.get_tile(plane, pos.x, pos.y)
	if tile == null or tile.resource != "mana_node":
		return false
	var state = get_magic_state(wizard_id)
	if state.is_empty():
		return false

	# Check if already claimed by this wizard
	for node in state["controlled_nodes"]:
		if node["plane"] == plane and node["pos"] == pos:
			return false

	state["controlled_nodes"].append({"plane": plane, "pos": pos})
	state["mana_per_turn"] += Constants.MANA_PER_NODE
	EventBus.mana_node_claimed.emit(wizard_id, plane, pos)
	return true


# ---------------------------------------------------------------------------
# Casting
# ---------------------------------------------------------------------------

func cast_spell(wizard_id: int, spell_id: String, target: Variant) -> bool:
	if not can_cast_spell(wizard_id, spell_id):
		return false

	var spell = DataLoader.get_spell(spell_id)
	var mana_cost: int = spell.get("mana_cost", 10)
	var state = get_magic_state(wizard_id)

	state["current_mana"] -= mana_cost

	# Apply spell effect based on type
	var spell_type: String = spell.get("type", "")
	match spell_type:
		"combat_damage":
			_apply_damage_spell(wizard_id, spell, target)
		"summon":
			_apply_summon_spell(wizard_id, spell, target)
		"enchantment":
			_apply_enchantment_spell(wizard_id, spell, target)
		"global":
			_apply_global_spell(wizard_id, spell)
		"heal":
			_apply_heal_spell(wizard_id, spell, target)

	EventBus.spell_cast.emit(wizard_id, spell_id)
	GameState.add_fame(wizard_id, spell.get("fame", 1))
	return true


# ---------------------------------------------------------------------------
# Research
# ---------------------------------------------------------------------------

func start_research(wizard_id: int, spell_id: String) -> void:
	var state = get_magic_state(wizard_id)
	if state.is_empty():
		return
	state["research_spell"] = spell_id
	state["research_progress"] = 0


func process_turn() -> void:
	for wizard_id in wizard_magic:
		var state = wizard_magic[wizard_id] as Dictionary
		if GameState.get_wizard(wizard_id).get("is_eliminated", true):
			continue

		# Mana income
		state["current_mana"] = mini(state["max_mana"], state["current_mana"] + state["mana_per_turn"])

		# Research progress (use half of casting skill as research points)
		if state["research_spell"] != "":
			var research_rate: int = maxi(1, state["casting_skill"] / 2)
			state["research_progress"] += research_rate

			var spell = DataLoader.get_spell(state["research_spell"])
			var research_cost: int = spell.get("research_cost", 50)
			if state["research_progress"] >= research_cost:
				state["known_spells"].append(state["research_spell"])
				EventBus.spell_researched.emit(wizard_id, state["research_spell"])
				state["research_spell"] = ""
				state["research_progress"] = 0


# ---------------------------------------------------------------------------
# Spell effect helpers
# ---------------------------------------------------------------------------

func _apply_damage_spell(_wizard_id: int, spell: Dictionary, target: Variant) -> void:
	# target should be a unit_id
	if target is int:
		var unit = UnitManager.get_unit(target as int)
		if not unit.is_empty():
			var dmg: int = spell.get("damage", 5)
			unit["current_hp"] -= dmg
			if unit["current_hp"] <= 0:
				UnitManager.despawn_unit(target as int)


func _apply_summon_spell(wizard_id: int, spell: Dictionary, target: Variant) -> void:
	# target should be a Vector2i position; summon on current plane
	var pos = target as Vector2i if target is Vector2i else Vector2i.ZERO
	var summon_id: String = spell.get("summon_unit", "")
	if summon_id != "":
		# Use plane 0 as default; caller should set proper plane
		var uid = UnitManager.spawn_unit(summon_id, 0, pos, wizard_id)
		# Add to or create army
		var armies = UnitManager.get_armies_at(0, pos)
		var added = false
		for army in armies:
			if army["owner"] == wizard_id and army["unit_ids"].size() < Constants.MAX_UNITS_PER_ARMY:
				UnitManager.add_unit_to_army(uid, army["army_id"])
				added = true
				break
		if not added:
			var aid = UnitManager.create_army(wizard_id, 0, pos)
			UnitManager.add_unit_to_army(uid, aid)


func _apply_enchantment_spell(_wizard_id: int, _spell: Dictionary, _target: Variant) -> void:
	# Placeholder for enchantment effects (buffs on units/cities)
	pass


func _apply_global_spell(_wizard_id: int, _spell: Dictionary) -> void:
	# Placeholder for global effects (e.g., plane-wide weather)
	pass


func _apply_heal_spell(_wizard_id: int, spell: Dictionary, target: Variant) -> void:
	if target is int:
		var unit = UnitManager.get_unit(target as int)
		if not unit.is_empty():
			var heal_amount: int = spell.get("heal", 5)
			unit["current_hp"] = mini(unit["max_hp"], unit["current_hp"] + heal_amount)
