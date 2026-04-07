extends Node
## Port of CoMUnitSubsystem — manages all units and armies.

var all_units: Dictionary = {}    # unit_id -> unit dict
var all_armies: Dictionary = {}   # army_id -> army dict
var _next_unit_id: int = 1
var _next_army_id: int = 1


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Unit creation / destruction
# ---------------------------------------------------------------------------

func spawn_unit(spec_id: String, plane: int, pos: Vector2i, owner: int) -> int:
	var spec := DataLoader.get_unit_spec(spec_id)
	var uid := _next_unit_id
	_next_unit_id += 1

	var unit: Dictionary = {
		"unit_id": uid,
		"spec_id": spec_id,
		"owner": owner,
		"plane": plane,
		"pos": pos,
		"current_hp": spec.get("hp", 10),
		"max_hp": spec.get("hp", 10),
		"xp": 0,
		"level": 1,
		"melee_attack": spec.get("melee_attack", 3),
		"ranged_attack": spec.get("ranged_attack", 0),
		"defense": spec.get("defense", 2),
		"resistance": spec.get("resistance", 3),
		"figures": spec.get("figures", 1),
		"movement": spec.get("movement", 2),
		"movement_remaining": spec.get("movement", 2),
		"abilities": spec.get("abilities", []),
		"movement_type": spec.get("movement_type", "walk"),
	}
	all_units[uid] = unit
	EventBus.unit_spawned.emit(uid)
	return uid


func despawn_unit(unit_id: int) -> void:
	if not all_units.has(unit_id):
		return
	# Remove from any army
	for army in all_armies.values():
		if unit_id in army["unit_ids"]:
			army["unit_ids"].erase(unit_id)
			break
	all_units.erase(unit_id)
	EventBus.unit_killed.emit(unit_id)


func get_unit(unit_id: int) -> Dictionary:
	return all_units.get(unit_id, {})


# ---------------------------------------------------------------------------
# Army management
# ---------------------------------------------------------------------------

func create_army(owner: int, plane: int, pos: Vector2i) -> int:
	var aid := _next_army_id
	_next_army_id += 1
	var army: Dictionary = {
		"army_id": aid,
		"owner": owner,
		"plane": plane,
		"pos": pos,
		"unit_ids": [],
		"hero_id": -1,
	}
	all_armies[aid] = army
	return aid


func add_unit_to_army(unit_id: int, army_id: int) -> bool:
	if not all_units.has(unit_id) or not all_armies.has(army_id):
		return false
	var army := all_armies[army_id] as Dictionary
	if army["unit_ids"].size() >= Constants.MAX_UNITS_PER_ARMY:
		return false
	# Remove from previous army first
	for other_army in all_armies.values():
		if unit_id in other_army["unit_ids"]:
			other_army["unit_ids"].erase(unit_id)
			break
	army["unit_ids"].append(unit_id)
	# Sync unit position
	all_units[unit_id]["plane"] = army["plane"]
	all_units[unit_id]["pos"] = army["pos"]
	return true


func remove_unit_from_army(unit_id: int, army_id: int) -> bool:
	if not all_armies.has(army_id):
		return false
	var army := all_armies[army_id] as Dictionary
	if unit_id in army["unit_ids"]:
		army["unit_ids"].erase(unit_id)
		_cleanup_empty_army(army_id)
		return true
	return false


func merge_armies(source_id: int, dest_id: int) -> bool:
	if not all_armies.has(source_id) or not all_armies.has(dest_id):
		return false
	var src := all_armies[source_id] as Dictionary
	var dst := all_armies[dest_id] as Dictionary
	if dst["unit_ids"].size() + src["unit_ids"].size() > Constants.MAX_UNITS_PER_ARMY:
		return false
	for uid in src["unit_ids"]:
		dst["unit_ids"].append(uid)
		all_units[uid]["pos"] = dst["pos"]
		all_units[uid]["plane"] = dst["plane"]
	all_armies.erase(source_id)
	return true


func split_army(army_id: int, unit_ids: Array) -> int:
	if not all_armies.has(army_id):
		return -1
	var old_army := all_armies[army_id] as Dictionary
	var new_id := create_army(old_army["owner"], old_army["plane"], old_army["pos"])
	for uid in unit_ids:
		if uid in old_army["unit_ids"]:
			old_army["unit_ids"].erase(uid)
			all_armies[new_id]["unit_ids"].append(uid)
	_cleanup_empty_army(army_id)
	return new_id


func move_army(army_id: int, destination: Vector2i) -> void:
	if not all_armies.has(army_id):
		return
	var army := all_armies[army_id] as Dictionary
	var from := army["pos"] as Vector2i

	# Calculate path cost
	var path := Pathfinder.find_path(army["plane"], from, destination, _get_army_move_type(army_id))
	if path.is_empty():
		return

	# Consume movement along the path
	var total_cost: float = 0.0
	var min_remaining := _get_army_min_movement(army_id)

	for i in range(1, path.size()):
		var step_cost := Pathfinder.get_move_cost(army["plane"], path[i], _get_army_move_type(army_id))
		if total_cost + step_cost > min_remaining:
			# Move as far as we can
			destination = path[i - 1]
			break
		total_cost += step_cost
		destination = path[i]

	army["pos"] = destination
	for uid in army["unit_ids"]:
		all_units[uid]["pos"] = destination
		all_units[uid]["movement_remaining"] -= int(total_cost)
		all_units[uid]["movement_remaining"] = maxi(0, all_units[uid]["movement_remaining"])

	EventBus.army_moved.emit(army_id, from, destination)


# ---------------------------------------------------------------------------
# Queries
# ---------------------------------------------------------------------------

func get_armies_at(plane: int, pos: Vector2i) -> Array:
	var result: Array = []
	for army in all_armies.values():
		if army["plane"] == plane and army["pos"] == pos:
			result.append(army)
	return result


func get_armies_for_wizard(wizard_id: int) -> Array:
	var result: Array = []
	for army in all_armies.values():
		if army["owner"] == wizard_id:
			result.append(army)
	return result


func get_army(army_id: int) -> Dictionary:
	return all_armies.get(army_id, {})


# ---------------------------------------------------------------------------
# Turn processing
# ---------------------------------------------------------------------------

func refresh_movement() -> void:
	for unit in all_units.values():
		unit["movement_remaining"] = unit["movement"]


func apply_xp(unit_id: int, xp_amount: int) -> void:
	if not all_units.has(unit_id):
		return
	var unit := all_units[unit_id] as Dictionary
	unit["xp"] += xp_amount
	while unit["xp"] >= Constants.XP_PER_LEVEL and unit["level"] < Constants.MAX_LEVEL:
		unit["xp"] -= Constants.XP_PER_LEVEL
		unit["level"] += 1
		# Small stat boost per level
		unit["melee_attack"] += 1
		unit["defense"] += 1
		unit["max_hp"] += 2
		unit["current_hp"] = unit["max_hp"]


# ---------------------------------------------------------------------------
# Internal
# ---------------------------------------------------------------------------

func _cleanup_empty_army(army_id: int) -> void:
	if all_armies.has(army_id):
		var army := all_armies[army_id] as Dictionary
		if army["unit_ids"].is_empty():
			all_armies.erase(army_id)


func _get_army_move_type(army_id: int) -> String:
	if not all_armies.has(army_id):
		return "walk"
	var army := all_armies[army_id] as Dictionary
	# Army moves at the speed of its slowest member, use walk unless all fly
	var all_fly := true
	for uid in army["unit_ids"]:
		if all_units.has(uid):
			if all_units[uid].get("movement_type", "walk") != "fly":
				all_fly = false
				break
	return "fly" if all_fly else "walk"


func _get_army_min_movement(army_id: int) -> float:
	if not all_armies.has(army_id):
		return 0.0
	var army := all_armies[army_id] as Dictionary
	var min_mv: float = 999.0
	for uid in army["unit_ids"]:
		if all_units.has(uid):
			min_mv = minf(min_mv, float(all_units[uid]["movement_remaining"]))
	return min_mv if min_mv < 999.0 else 0.0
