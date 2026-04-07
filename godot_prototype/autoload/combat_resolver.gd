extends Node
## Port of CoMCombatSubsystem — Master of Magic-style auto-resolve combat.
## Per-figure attack rolls, defense blocks, HP damage, max 50 rounds.

var rng: RandomNumberGenerator


func _ready() -> void:
	rng = RandomNumberGenerator.new()
	rng.randomize()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func auto_resolve(attacker_army_id: int, defender_army_id: int) -> Dictionary:
	var atk_army = UnitManager.get_army(attacker_army_id)
	var def_army = UnitManager.get_army(defender_army_id)

	if atk_army.is_empty() or def_army.is_empty():
		return {"winner": -1, "attacker_losses": [], "defender_losses": [], "rounds": 0, "xp_gained": 0}

	# Clone unit data for combat simulation
	var atk_units = _get_combat_units(atk_army["unit_ids"])
	var def_units = _get_combat_units(def_army["unit_ids"])

	var terrain_mod = get_terrain_modifier(def_army.get("plane", 0), def_army.get("pos", Vector2i.ZERO))
	var rounds: int = 0

	while rounds < Constants.MAX_COMBAT_ROUNDS:
		rounds += 1

		# Attacker phase: each attacker unit attacks a random defender
		_combat_phase(atk_units, def_units, 1.0)
		_remove_dead(def_units)

		if def_units.is_empty():
			break

		# Defender phase: each defender attacks a random attacker (with terrain bonus)
		_combat_phase(def_units, atk_units, terrain_mod)
		_remove_dead(atk_units)

		if atk_units.is_empty():
			break

	# Determine winner
	var winner: int = -1
	var atk_losses: Array = []
	var def_losses: Array = []

	if def_units.is_empty() and not atk_units.is_empty():
		winner = atk_army["owner"]
	elif atk_units.is_empty() and not def_units.is_empty():
		winner = def_army["owner"]
	else:
		# Draw or both dead: defender holds
		winner = def_army["owner"] if not def_units.is_empty() else -1

	# Identify losses
	for uid in atk_army["unit_ids"]:
		var survived = false
		for cu in atk_units:
			if cu["unit_id"] == uid:
				survived = true
				break
		if not survived:
			atk_losses.append(uid)

	for uid in def_army["unit_ids"]:
		var survived = false
		for cu in def_units:
			if cu["unit_id"] == uid:
				survived = true
				break
		if not survived:
			def_losses.append(uid)

	# Calculate XP from defeated enemies
	var xp_gained: int = (atk_losses.size() + def_losses.size()) * 25

	var result = {
		"winner": winner,
		"attacker_losses": atk_losses,
		"defender_losses": def_losses,
		"rounds": rounds,
		"xp_gained": xp_gained,
	}

	# Apply results
	_apply_combat_results(attacker_army_id, defender_army_id, result, atk_units, def_units)

	EventBus.combat_resolved.emit(result)
	return result


func detect_encounters() -> Array:
	## Returns array of [attacker_army_id, defender_army_id] pairs.
	var encounters: Array = []
	var checked: Dictionary = {}  # avoid duplicates

	for army in UnitManager.all_armies.values():
		var key = "%d_%d_%d" % [army["plane"], army["pos"].x, army["pos"].y]
		if checked.has(key):
			continue
		checked[key] = true

		var armies_here = UnitManager.get_armies_at(army["plane"], army["pos"])
		if armies_here.size() < 2:
			continue

		# Find hostile pairs
		for i in range(armies_here.size()):
			for j in range(i + 1, armies_here.size()):
				var a = armies_here[i] as Dictionary
				var b = armies_here[j] as Dictionary
				if a["owner"] != b["owner"] and DiplomacySystem.are_at_war(a["owner"], b["owner"]):
					encounters.append([a["army_id"], b["army_id"]])
	return encounters


func resolve_all_encounters() -> void:
	var encounters = detect_encounters()
	for pair in encounters:
		# Armies may have been destroyed in earlier combat this turn
		if UnitManager.all_armies.has(pair[0]) and UnitManager.all_armies.has(pair[1]):
			auto_resolve(pair[0], pair[1])


func calculate_army_power(army_id: int) -> float:
	var army = UnitManager.get_army(army_id)
	if army.is_empty():
		return 0.0
	var power: float = 0.0
	for uid in army["unit_ids"]:
		var unit = UnitManager.get_unit(uid)
		if unit.is_empty():
			continue
		power += float(unit["melee_attack"] + unit["ranged_attack"]) * float(unit["current_hp"]) / float(maxi(unit["max_hp"], 1))
		power += float(unit["defense"]) * 0.5
	return power


func get_terrain_modifier(plane: int, pos: Vector2i) -> float:
	var tile = WorldMap.get_tile(plane, pos.x, pos.y)
	if tile == null:
		return 1.0
	match tile.terrain:
		Constants.TERRAIN_HILLS:
			return 1.25  # defender advantage on hills
		Constants.TERRAIN_FOREST:
			return 1.15  # slight cover
		Constants.TERRAIN_MOUNTAIN:
			return 1.5   # strong defensive position
		Constants.TERRAIN_SWAMP:
			return 0.9   # bad for both sides
		_:
			return 1.0


# ---------------------------------------------------------------------------
# Internal combat mechanics
# ---------------------------------------------------------------------------

func _get_combat_units(unit_ids: Array) -> Array:
	var result: Array = []
	for uid in unit_ids:
		var unit = UnitManager.get_unit(uid)
		if not unit.is_empty():
			result.append(unit.duplicate(true))
	return result


func _combat_phase(attackers: Array, defenders: Array, modifier: float) -> void:
	for atk in attackers:
		if defenders.is_empty():
			break
		# Pick random target
		var target = defenders[rng.randi_range(0, defenders.size() - 1)]

		# Per-figure attack: each figure rolls to hit
		var figures: int = atk.get("figures", 1)
		var attack_value: int = atk.get("melee_attack", 3)

		# Use ranged if available and positive
		if atk.get("ranged_attack", 0) > 0:
			attack_value = maxi(attack_value, atk["ranged_attack"])

		var total_damage: int = 0
		for _f in range(figures):
			# Roll attack: each point of attack is a chance to hit (d10 <= attack)
			var hits: int = 0
			for _a in range(attack_value):
				if rng.randi_range(1, 10) <= 5:  # 50% base hit chance
					hits += 1

			# Defender blocks: each point of defense can block a hit (d10 <= defense)
			var defense_value: int = int(float(target.get("defense", 2)) * modifier)
			var blocks: int = 0
			for _d in range(mini(hits, defense_value)):
				if rng.randi_range(1, 10) <= 4:  # 40% block chance
					blocks += 1

			total_damage += maxi(0, hits - blocks)

		target["current_hp"] -= total_damage
		# Reduce figures proportionally
		if target["current_hp"] > 0 and target["max_hp"] > 0:
			var hp_ratio: float = float(target["current_hp"]) / float(target["max_hp"])
			var base_figs: int = target.get("figures", 1)
			target["figures"] = maxi(1, int(ceil(float(base_figs) * hp_ratio)))


func _remove_dead(units: Array) -> void:
	var i = units.size() - 1
	while i >= 0:
		if units[i]["current_hp"] <= 0:
			units.remove_at(i)
		i -= 1


func _apply_combat_results(atk_id: int, def_id: int, result: Dictionary,
		surviving_atk: Array, surviving_def: Array) -> void:
	# Despawn dead units
	for uid in result["attacker_losses"]:
		UnitManager.despawn_unit(uid)
	for uid in result["defender_losses"]:
		UnitManager.despawn_unit(uid)

	# Apply HP changes to survivors
	for cu in surviving_atk:
		var uid: int = cu["unit_id"]
		if UnitManager.all_units.has(uid):
			UnitManager.all_units[uid]["current_hp"] = cu["current_hp"]
			UnitManager.all_units[uid]["figures"] = cu["figures"]

	for cu in surviving_def:
		var uid: int = cu["unit_id"]
		if UnitManager.all_units.has(uid):
			UnitManager.all_units[uid]["current_hp"] = cu["current_hp"]
			UnitManager.all_units[uid]["figures"] = cu["figures"]

	# Award XP to winning side survivors
	var xp_per_unit: int = maxi(1, result["xp_gained"] / maxi(1, (surviving_atk.size() + surviving_def.size())))
	var winners = surviving_atk if result["winner"] == UnitManager.get_army(atk_id).get("owner", -1) else surviving_def
	for cu in winners:
		UnitManager.apply_xp(cu["unit_id"], xp_per_unit)

	# Destroy empty armies
	if UnitManager.all_armies.has(atk_id):
		var army = UnitManager.get_army(atk_id)
		if army.get("unit_ids", []).is_empty():
			UnitManager.all_armies.erase(atk_id)
	if UnitManager.all_armies.has(def_id):
		var army = UnitManager.get_army(def_id)
		if army.get("unit_ids", []).is_empty():
			UnitManager.all_armies.erase(def_id)
