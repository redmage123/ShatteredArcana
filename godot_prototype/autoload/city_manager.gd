extends Node
## City production and management subsystem.
## Handles city creation, build queues, income, and population growth.

var all_cities: Dictionary = {}   # city_id -> city dict
var _next_city_id: int = 1


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# City lifecycle
# ---------------------------------------------------------------------------

func create_city(city_name: String, plane: int, pos: Vector2i, owner: int) -> int:
	var cid := _next_city_id
	_next_city_id += 1
	var city: Dictionary = {
		"city_id": cid,
		"name": city_name,
		"owner": owner,
		"plane": plane,
		"pos": pos,
		"population": 3,
		"buildings": [],
		"build_queue": [],
		"food": 0,
		"unrest": 0,
	}
	all_cities[cid] = city

	# Mark tile ownership
	if owner >= 0:
		WorldMap.set_tile_owner(plane, pos.x, pos.y, owner)
		var tile := WorldMap.get_tile(plane, pos.x, pos.y)
		if tile:
			tile.city_id = cid

	EventBus.city_founded.emit(cid)
	return cid


func get_city(city_id: int) -> Dictionary:
	return all_cities.get(city_id, {})


func get_cities_for_wizard(wizard_id: int) -> Array:
	var result: Array = []
	for city in all_cities.values():
		if city["owner"] == wizard_id:
			result.append(city)
	return result


func capture_city(city_id: int, new_owner: int) -> void:
	if not all_cities.has(city_id):
		return
	var city := all_cities[city_id] as Dictionary
	city["owner"] = new_owner
	city["unrest"] = mini(city["population"], 5)  # captured cities have unrest
	city["build_queue"].clear()
	WorldMap.set_tile_owner(city["plane"], city["pos"].x, city["pos"].y, new_owner)


# ---------------------------------------------------------------------------
# Build queue
# ---------------------------------------------------------------------------

func add_to_build_queue(city_id: int, build_type: String, spec_id: String) -> void:
	if not all_cities.has(city_id):
		return
	var city := all_cities[city_id] as Dictionary
	if city["build_queue"].size() >= Constants.MAX_BUILD_QUEUE:
		return
	var cost: int = _get_build_cost(build_type, spec_id)
	city["build_queue"].append({
		"build_type": build_type,  # "unit" or "building"
		"spec_id": spec_id,
		"cost": cost,
		"progress": 0,
	})


func clear_build_queue(city_id: int) -> void:
	if all_cities.has(city_id):
		all_cities[city_id]["build_queue"].clear()


# ---------------------------------------------------------------------------
# Per-turn production processing
# ---------------------------------------------------------------------------

func process_production() -> void:
	for city in all_cities.values():
		if city["owner"] < 0:
			continue  # neutral city, no production

		var income := get_city_income(city["city_id"])

		# Add gold and mana to wizard
		GameState.add_gold(city["owner"], income["gold"])
		GameState.add_mana(city["owner"], income["mana"])

		# Food -> population growth
		city["food"] += income["food"]
		var food_needed: int = city["population"] * Constants.BASE_FOOD_REQUIREMENT * 2
		if city["food"] >= food_needed and city["population"] < Constants.MAX_CITY_POPULATION:
			city["population"] += 1
			city["food"] = 0

		# Unrest decay
		if city["unrest"] > 0:
			city["unrest"] -= 1

		# Build queue advancement
		if city["build_queue"].is_empty():
			continue

		var item := city["build_queue"][0] as Dictionary
		var production_power: int = income["production"]
		# Unrest reduces production
		production_power = maxi(1, production_power - city["unrest"])
		item["progress"] += production_power

		if item["progress"] >= item["cost"]:
			_complete_build(city, item)
			city["build_queue"].pop_front()


func get_city_income(city_id: int) -> Dictionary:
	if not all_cities.has(city_id):
		return {"gold": 0, "mana": 0, "food": 0, "production": 0}
	var city := all_cities[city_id] as Dictionary
	var pop: int = city["population"]

	var gold: int = pop * Constants.BASE_GOLD_PER_POP
	var mana: int = 0
	var food: int = pop  # 1 food per pop base
	var production: int = pop * Constants.BASE_PRODUCTION_PER_POP

	# Building bonuses
	if "marketplace" in city["buildings"]:
		gold += pop
	if "library" in city["buildings"]:
		mana += 2
	if "granary" in city["buildings"]:
		food += pop
	if "smithy" in city["buildings"]:
		production += 3
	if "temple" in city["buildings"]:
		mana += 3
	if "barracks" in city["buildings"]:
		production += 2
	if "shrine" in city["buildings"]:
		mana += 1

	# Check for nearby mana node
	var tile := WorldMap.get_tile(city["plane"], city["pos"].x, city["pos"].y)
	if tile and tile.resource == "mana_node":
		mana += Constants.MANA_PER_NODE

	# Check for resource tiles nearby
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			if dx == 0 and dy == 0:
				continue
			var nearby := WorldMap.get_tile(city["plane"], city["pos"].x + dx, city["pos"].y + dy)
			if nearby and nearby.resource == "gold_mine":
				gold += 3
			elif nearby and nearby.resource == "iron_mine":
				production += 2

	return {"gold": gold, "mana": mana, "food": food, "production": production}


# ---------------------------------------------------------------------------
# Internal
# ---------------------------------------------------------------------------

func _get_build_cost(build_type: String, spec_id: String) -> int:
	if build_type == "unit":
		var spec := DataLoader.get_unit_spec(spec_id)
		return spec.get("cost", 40)
	elif build_type == "building":
		var spec := DataLoader.get_building_spec(spec_id)
		return spec.get("cost", 60)
	return 50


func _complete_build(city: Dictionary, item: Dictionary) -> void:
	if item["build_type"] == "unit":
		var uid := UnitManager.spawn_unit(item["spec_id"], city["plane"], city["pos"], city["owner"])
		# Auto-create army for the unit or add to existing
		var armies_here := UnitManager.get_armies_at(city["plane"], city["pos"])
		var added := false
		for army in armies_here:
			if army["owner"] == city["owner"] and army["unit_ids"].size() < Constants.MAX_UNITS_PER_ARMY:
				UnitManager.add_unit_to_army(uid, army["army_id"])
				added = true
				break
		if not added:
			var aid := UnitManager.create_army(city["owner"], city["plane"], city["pos"])
			UnitManager.add_unit_to_army(uid, aid)
		EventBus.city_production_complete.emit(city["city_id"], item["spec_id"])

	elif item["build_type"] == "building":
		if item["spec_id"] not in city["buildings"]:
			city["buildings"].append(item["spec_id"])
		EventBus.city_production_complete.emit(city["city_id"], item["spec_id"])
