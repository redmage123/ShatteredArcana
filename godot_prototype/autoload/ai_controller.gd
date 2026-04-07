extends Node

## AI Controller v2 — personality-driven strategy with economic/military phases

var rng := RandomNumberGenerator.new()

func _ready():
	rng.randomize()

func process_ai_turn(wizard_id: int) -> void:
	var wizard = GameState.get_wizard(wizard_id)
	if wizard == null or wizard.get("is_eliminated", false):
		return

	var personality = wizard.get("personality", {"aggressiveness": 50, "cunning": 50, "trade": 50, "scholarly": 50})
	var cities = CityManager.get_cities_for_wizard(wizard_id)
	var armies = UnitManager.get_armies_for_wizard(wizard_id)
	var strength = _evaluate_strength(wizard_id)
	var turn = GameState.current_turn

	# Phase 1: Economy — always manage cities first
	_ai_manage_cities(wizard_id, cities, personality, turn)

	# Phase 2: Magic — research and cast
	_ai_manage_magic(wizard_id, personality)

	# Phase 3: Military — build or attack based on personality and threat
	var threat = _assess_threat(wizard_id)
	if threat > strength * 0.8:
		_ai_defend(wizard_id, armies, cities)
	elif turn < 10 or personality.get("aggressiveness", 50) < 40:
		_ai_explore_and_expand(wizard_id, armies)
	else:
		_ai_attack(wizard_id, armies, personality)

	# Phase 4: Diplomacy — personality-driven
	_ai_diplomacy(wizard_id, personality, strength)

# ── ECONOMY ──────────────────────────────────────────────────────────────────

func _ai_manage_cities(wizard_id: int, cities: Array, personality: Dictionary, turn: int) -> void:
	for city in cities:
		var cid = city.get("city_id", -1)
		if cid < 0:
			continue

		var queue = city.get("build_queue", [])
		if queue.size() > 0:
			continue  # Already building something

		var buildings = city.get("buildings", [])
		var pop = city.get("population", 1)

		# Priority build order based on personality and game phase
		var build_order: Array = []

		if turn < 8:
			# Early game: economy first
			build_order = ["granary", "barracks", "marketplace", "smithy", "library"]
		elif personality.get("scholarly", 50) > 60:
			# Scholarly: focus on magic buildings
			build_order = ["library", "wizard_tower", "oracle", "summoning_circle", "shrine", "temple"]
		elif personality.get("aggressiveness", 50) > 60:
			# Aggressive: military focus
			build_order = ["barracks", "smithy", "armory", "stable", "war_college", "city_walls"]
		elif personality.get("trade", 50) > 60:
			# Trade: economic focus
			build_order = ["marketplace", "bank", "granary", "harbor", "aqueduct"]
		else:
			# Balanced
			build_order = ["granary", "barracks", "marketplace", "library", "smithy", "shrine"]

		for building_id in build_order:
			if building_id not in buildings:
				if CityManager.can_build(cid, building_id):
					CityManager.add_to_build_queue(cid, "building", building_id)
					break

		# If no building to build, build military units
		if city.get("build_queue", []).size() == 0:
			var my_armies = UnitManager.get_armies_for_wizard(wizard_id)
			var unit_count = 0
			for army in my_armies:
				unit_count += army.get("unit_ids", []).size()

			if unit_count < cities.size() * 3:  # Want at least 3 units per city
				var unit_type = "infantry"
				if rng.randf() < 0.3:
					unit_type = "ranged"
				elif rng.randf() < 0.2:
					unit_type = "cavalry"
				CityManager.add_to_build_queue(cid, "unit", unit_type)

# ── MAGIC ────────────────────────────────────────────────────────────────────

func _ai_manage_magic(wizard_id: int, personality: Dictionary) -> void:
	var magic = MagicSystem.get_magic_state(wizard_id)
	if magic == null:
		return

	# Research: pick cheapest unknown spell in our realm
	if magic.get("research_spell", "") == "":
		var realm = GameState.get_wizard(wizard_id).get("realm", "arcane")
		var known = magic.get("known_spells", [])
		var best_spell = ""
		var best_cost = 999999

		for spell in DataLoader.spells:
			if spell.get("realm", "") == realm and spell.get("id", "") not in known:
				var cost = spell.get("research", 9999)
				if cost < best_cost:
					best_cost = cost
					best_spell = spell.get("id", "")

		if best_spell != "":
			MagicSystem.start_research(wizard_id, best_spell)

	# Set research allocation based on personality
	var scholarly = personality.get("scholarly", 50)
	var allocation = int(magic.get("mana_per_turn", 5) * scholarly / 100.0)
	magic["research_allocation"] = max(1, allocation)

	# Cast useful spells if we have enough mana
	var mana = magic.get("current_mana", 0)
	if mana > 30:
		# Try to cast combat buffs or city enchantments
		for spell in magic.get("known_spells", []):
			var spell_data = DataLoader.get_spell(spell)
			if spell_data == null:
				continue
			if spell_data.get("scope", "") == "City" and spell_data.get("cost", 999) < mana:
				var cities = CityManager.get_cities_for_wizard(wizard_id)
				if cities.size() > 0:
					MagicSystem.cast_spell(wizard_id, spell, cities[0].get("city_id", -1))
					break

# ── MILITARY ─────────────────────────────────────────────────────────────────

func _ai_explore_and_expand(wizard_id: int, armies: Array) -> void:
	for army in armies:
		var army_id = army.get("army_id", -1)
		if army_id < 0:
			continue

		var pos = army.get("pos", Vector2i.ZERO)
		var plane = army.get("plane", 0)

		# Find nearest unexplored tile
		var best_target = Vector2i(-1, -1)
		var best_dist = 9999

		for dx in range(-10, 11):
			for dy in range(-10, 11):
				var check = Vector2i(
					(pos.x + dx + Constants.MAP_WIDTH) % Constants.MAP_WIDTH,
					clampi(pos.y + dy, 0, Constants.MAP_HEIGHT - 1)
				)
				if not FogOfWar.is_explored(wizard_id, plane, check):
					var dist = abs(dx) + abs(dy)
					if dist < best_dist and dist > 0:
						best_dist = dist
						best_target = check

		if best_target != Vector2i(-1, -1):
			UnitManager.move_army(army_id, best_target)

func _ai_attack(wizard_id: int, armies: Array, personality: Dictionary) -> void:
	var aggressiveness = personality.get("aggressiveness", 50)

	for army in armies:
		var army_id = army.get("army_id", -1)
		var pos = army.get("pos", Vector2i.ZERO)
		var plane = army.get("plane", 0)
		var my_power = CombatResolver.calculate_army_power(army_id)

		# Find nearest enemy army or city
		var best_target = Vector2i(-1, -1)
		var best_dist = 9999

		# Check enemy armies
		for other_wiz in range(Constants.MAX_WIZARDS):
			if other_wiz == wizard_id:
				continue
			if not DiplomacySystem.are_at_war(wizard_id, other_wiz):
				# Maybe declare war if aggressive enough
				if aggressiveness > 70 and rng.randi_range(1, 100) < aggressiveness / 2:
					var their_strength = _evaluate_strength(other_wiz)
					var my_strength = _evaluate_strength(wizard_id)
					if my_strength > their_strength * 1.5:
						DiplomacySystem.declare_war(wizard_id, other_wiz)
				continue

			var enemy_armies = UnitManager.get_armies_for_wizard(other_wiz)
			for enemy in enemy_armies:
				if enemy.get("plane", -1) != plane:
					continue
				var epos = enemy.get("pos", Vector2i(-99, -99))
				var dist = abs(epos.x - pos.x) + abs(epos.y - pos.y)
				var enemy_power = CombatResolver.calculate_army_power(enemy.get("army_id", -1))

				# Only attack if we're stronger or aggressive
				if my_power > enemy_power * 0.8 or aggressiveness > 80:
					if dist < best_dist:
						best_dist = dist
						best_target = epos

		# Check enemy cities
		for other_wiz in range(Constants.MAX_WIZARDS):
			if other_wiz == wizard_id or not DiplomacySystem.are_at_war(wizard_id, other_wiz):
				continue
			var enemy_cities = CityManager.get_cities_for_wizard(other_wiz)
			for city in enemy_cities:
				if city.get("plane", -1) != plane:
					continue
				var cpos = city.get("pos", Vector2i(-99, -99))
				var dist = abs(cpos.x - pos.x) + abs(cpos.y - pos.y)
				if dist < best_dist:
					best_dist = dist
					best_target = cpos

		if best_target != Vector2i(-1, -1) and best_dist < 20:
			UnitManager.move_army(army_id, best_target)
		else:
			_ai_explore_and_expand(wizard_id, [army])

func _ai_defend(wizard_id: int, armies: Array, cities: Array) -> void:
	# Move armies toward cities under threat
	for army in armies:
		var army_id = army.get("army_id", -1)
		var pos = army.get("pos", Vector2i.ZERO)
		var plane = army.get("plane", 0)

		var nearest_city = Vector2i(-1, -1)
		var nearest_dist = 9999

		for city in cities:
			if city.get("plane", -1) != plane:
				continue
			var cpos = city.get("pos", Vector2i.ZERO)
			var dist = abs(cpos.x - pos.x) + abs(cpos.y - pos.y)
			if dist < nearest_dist:
				nearest_dist = dist
				nearest_city = cpos

		if nearest_city != Vector2i(-1, -1) and nearest_dist > 2:
			UnitManager.move_army(army_id, nearest_city)

# ── DIPLOMACY ────────────────────────────────────────────────────────────────

func _ai_diplomacy(wizard_id: int, personality: Dictionary, my_strength: float) -> void:
	var aggressiveness = personality.get("aggressiveness", 50)
	var trade_affinity = personality.get("trade", 50)

	for other_wiz in range(Constants.MAX_WIZARDS):
		if other_wiz == wizard_id:
			continue
		var other = GameState.get_wizard(other_wiz)
		if other == null or other.get("is_eliminated", false):
			continue

		var relation = DiplomacySystem.get_relation(wizard_id, other_wiz)
		var rep = relation.get("reputation", 0)
		var at_war = DiplomacySystem.are_at_war(wizard_id, other_wiz)

		if at_war:
			# Consider peace if losing
			var their_strength = _evaluate_strength(other_wiz)
			if my_strength < their_strength * 0.5 and aggressiveness < 60:
				DiplomacySystem.propose_peace(wizard_id, other_wiz)
		elif trade_affinity > 60 and rep > 20:
			# Consider alliance with friendly neighbors
			if not DiplomacySystem.are_allied(wizard_id, other_wiz) and rep > 50:
				# Would propose alliance here
				pass

# ── EVALUATION ───────────────────────────────────────────────────────────────

func _evaluate_strength(wizard_id: int) -> float:
	var total := 0.0
	var armies = UnitManager.get_armies_for_wizard(wizard_id)
	for army in armies:
		total += CombatResolver.calculate_army_power(army.get("army_id", -1))

	var cities = CityManager.get_cities_for_wizard(wizard_id)
	total += cities.size() * 20.0  # Cities are valuable

	return total

func _assess_threat(wizard_id: int) -> float:
	var max_threat := 0.0
	for other_wiz in range(Constants.MAX_WIZARDS):
		if other_wiz == wizard_id:
			continue
		if DiplomacySystem.are_at_war(wizard_id, other_wiz):
			var threat = _evaluate_strength(other_wiz)
			if threat > max_threat:
				max_threat = threat
	return max_threat
