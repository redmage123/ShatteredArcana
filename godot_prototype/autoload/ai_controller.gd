extends Node
## Basic AI controller for enemy wizards.
## Strategy: expand -> build economy -> build military -> attack.

var rng: RandomNumberGenerator


func _ready() -> void:
	rng = RandomNumberGenerator.new()
	rng.randomize()


# ---------------------------------------------------------------------------
# Main AI turn
# ---------------------------------------------------------------------------

func process_ai_turn(wizard_id: int) -> void:
	var wiz := GameState.get_wizard(wizard_id)
	if wiz.is_empty() or wiz.get("is_eliminated", true) or wiz.get("is_human", false):
		return

	ai_cast_spells(wizard_id)
	ai_build_in_cities(wizard_id)
	ai_move_armies(wizard_id)


# ---------------------------------------------------------------------------
# City building AI
# ---------------------------------------------------------------------------

func ai_build_in_cities(wizard_id: int) -> void:
	var cities := CityManager.get_cities_for_wizard(wizard_id)

	for city in cities:
		if not city["build_queue"].is_empty():
			continue  # already building something

		var has_barracks: bool = "barracks" in city["buildings"]
		var has_smithy: bool = "smithy" in city["buildings"]
		var has_granary: bool = "granary" in city["buildings"]
		var has_marketplace: bool = "marketplace" in city["buildings"]

		# Priority: granary -> barracks -> smithy -> marketplace -> units
		if not has_granary:
			CityManager.add_to_build_queue(city["city_id"], "building", "granary")
		elif not has_barracks:
			CityManager.add_to_build_queue(city["city_id"], "building", "barracks")
		elif not has_smithy:
			CityManager.add_to_build_queue(city["city_id"], "building", "smithy")
		elif not has_marketplace:
			CityManager.add_to_build_queue(city["city_id"], "building", "marketplace")
		else:
			# Build military units
			var army_strength := evaluate_military_strength(wizard_id)
			if army_strength < 30.0:
				CityManager.add_to_build_queue(city["city_id"], "unit", "swordsmen")
			elif rng.randf() < 0.3:
				CityManager.add_to_build_queue(city["city_id"], "unit", "cavalry")
			else:
				CityManager.add_to_build_queue(city["city_id"], "unit", "spearmen")


# ---------------------------------------------------------------------------
# Army movement AI
# ---------------------------------------------------------------------------

func ai_move_armies(wizard_id: int) -> void:
	var armies := UnitManager.get_armies_for_wizard(wizard_id)
	var my_cities := CityManager.get_cities_for_wizard(wizard_id)

	for army in armies:
		if army["unit_ids"].is_empty():
			continue

		# Decide on a target
		var target := _decide_army_target(wizard_id, army, my_cities)
		if target != army["pos"]:
			UnitManager.move_army(army["army_id"], target)


func _decide_army_target(wizard_id: int, army: Dictionary, my_cities: Array) -> Vector2i:
	var pos: Vector2i = army["pos"]
	var plane: int = army["plane"]

	# Priority 1: Attack nearby weak enemy
	var attack_target := find_attack_target(wizard_id)
	if attack_target >= 0:
		var enemy_army := UnitManager.get_army(attack_target)
		if not enemy_army.is_empty() and enemy_army["plane"] == plane:
			var dist := _distance(pos, enemy_army["pos"])
			if dist < 10:
				return enemy_army["pos"]

	# Priority 2: Capture neutral cities
	for city in CityManager.all_cities.values():
		if city["owner"] < 0 and city["plane"] == plane:
			var dist := _distance(pos, city["pos"])
			if dist < 15:
				return city["pos"]

	# Priority 3: Explore unexplored territory
	var explore_target := _find_explore_target(wizard_id, plane, pos)
	if explore_target != Vector2i(-1, -1):
		return explore_target

	# Priority 4: Expand / settle (move toward good terrain)
	var expansion := find_expansion_target(wizard_id)
	if expansion != Vector2i(-1, -1):
		return expansion

	# Default: stay put
	return pos


# ---------------------------------------------------------------------------
# Spell casting AI
# ---------------------------------------------------------------------------

func ai_cast_spells(wizard_id: int) -> void:
	var state := MagicSystem.get_magic_state(wizard_id)
	if state.is_empty():
		return

	# Start research if not researching
	if state["research_spell"] == "":
		var researchable := MagicSystem.get_researchable_spells(wizard_id)
		if not researchable.is_empty():
			MagicSystem.start_research(wizard_id, researchable[rng.randi_range(0, researchable.size() - 1)])

	# Cast available combat spells on enemies if mana is abundant
	if state["current_mana"] > state["max_mana"] * 0.5:
		var available := MagicSystem.get_available_spells(wizard_id)
		for spell_id in available:
			var spell := DataLoader.get_spell(spell_id)
			if spell.get("type", "") == "summon":
				# Summon near a city
				var cities := CityManager.get_cities_for_wizard(wizard_id)
				if not cities.is_empty():
					var city: Dictionary = cities[0]
					MagicSystem.cast_spell(wizard_id, spell_id, city["pos"])
				break  # one spell per turn for AI


# ---------------------------------------------------------------------------
# Evaluation helpers
# ---------------------------------------------------------------------------

func evaluate_military_strength(wizard_id: int) -> float:
	var total: float = 0.0
	for army in UnitManager.get_armies_for_wizard(wizard_id):
		total += CombatResolver.calculate_army_power(army["army_id"])
	return total


func find_expansion_target(wizard_id: int) -> Vector2i:
	# Find unclaimed grassland/forest tiles near owned cities
	var cities := CityManager.get_cities_for_wizard(wizard_id)
	if cities.is_empty():
		return Vector2i(-1, -1)

	var best_pos := Vector2i(-1, -1)
	var best_score: float = -1.0

	for city in cities:
		var plane: int = city["plane"]
		var cx: int = city["pos"].x
		var cy: int = city["pos"].y
		for dy in range(-8, 9):
			for dx in range(-8, 9):
				var x: int = WorldMap.wrap_x(cx + dx)
				var y: int = cy + dy
				if y < 2 or y >= Constants.MAP_HEIGHT - 2:
					continue
				var tile := WorldMap.get_tile(plane, x, y)
				if tile == null or tile.owner >= 0 or tile.terrain == Constants.TERRAIN_OCEAN:
					continue
				if tile.terrain in [Constants.TERRAIN_GRASSLAND, Constants.TERRAIN_FOREST]:
					var dist := absf(float(dx)) + absf(float(dy))
					var score: float = 10.0 - dist
					if tile.resource != "":
						score += 5.0
					if score > best_score:
						best_score = score
						best_pos = Vector2i(x, y)
	return best_pos


func find_attack_target(wizard_id: int) -> int:
	## Returns army_id of weakest enemy army nearby, or -1.
	var my_power := evaluate_military_strength(wizard_id)
	var best_target: int = -1
	var best_ratio: float = 0.0

	for army in UnitManager.all_armies.values():
		if army["owner"] == wizard_id:
			continue
		if not DiplomacySystem.are_at_war(wizard_id, army["owner"]):
			continue
		var enemy_power := CombatResolver.calculate_army_power(army["army_id"])
		if enemy_power <= 0.01:
			continue
		var ratio: float = my_power / enemy_power
		if ratio > 1.5 and ratio > best_ratio:
			best_ratio = ratio
			best_target = army["army_id"]

	return best_target


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

func _find_explore_target(wizard_id: int, plane: int, from: Vector2i) -> Vector2i:
	# Find nearest unexplored tile within range
	var best := Vector2i(-1, -1)
	var best_dist: float = 999.0
	var search_range: int = 12

	for dy in range(-search_range, search_range + 1):
		for dx in range(-search_range, search_range + 1):
			var x: int = WorldMap.wrap_x(from.x + dx)
			var y: int = from.y + dy
			if y < 0 or y >= Constants.MAP_HEIGHT:
				continue
			if not FogOfWar.is_explored(wizard_id, plane, Vector2i(x, y)):
				var tile := WorldMap.get_tile(plane, x, y)
				if tile and tile.terrain != Constants.TERRAIN_OCEAN:
					var dist := absf(float(dx)) + absf(float(dy))
					if dist < best_dist:
						best_dist = dist
						best = Vector2i(x, y)
	return best


func _distance(a: Vector2i, b: Vector2i) -> float:
	var dx: int = absi(a.x - b.x)
	# Handle wrap
	dx = mini(dx, Constants.MAP_WIDTH - dx)
	var dy: int = absi(a.y - b.y)
	return sqrt(float(dx * dx + dy * dy))
