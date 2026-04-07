extends Node
## Simplified diplomacy system.
## Tracks pairwise relations, war/peace/alliance state, and reputation.

var relations: Dictionary = {}  # pair_key -> relation dict


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Initialization
# ---------------------------------------------------------------------------

func initialize_relations(num_wizards: int) -> void:
	relations.clear()
	for a in range(num_wizards):
		for b in range(a + 1, num_wizards):
			var key := _pair_key(a, b)
			relations[key] = {
				"wizard_a": mini(a, b),
				"wizard_b": maxi(a, b),
				"reputation": 0,
				"at_war": false,
				"allied": false,
				"peace_turns": 0,  # cooldown after peace
			}


# ---------------------------------------------------------------------------
# Queries
# ---------------------------------------------------------------------------

func get_relation(a: int, b: int) -> Dictionary:
	if a == b:
		return {"reputation": Constants.RELATION_MAX, "at_war": false, "allied": true, "peace_turns": 0}
	var key := _pair_key(a, b)
	return relations.get(key, {})


func are_at_war(a: int, b: int) -> bool:
	var rel := get_relation(a, b)
	return rel.get("at_war", false)


func are_allied(a: int, b: int) -> bool:
	var rel := get_relation(a, b)
	return rel.get("allied", false)


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

func declare_war(attacker: int, defender: int) -> void:
	if attacker == defender:
		return
	var key := _pair_key(attacker, defender)
	if not relations.has(key):
		return
	var rel := relations[key] as Dictionary
	rel["at_war"] = true
	rel["allied"] = false
	rel["reputation"] = maxi(Constants.RELATION_MIN, rel["reputation"] - 30)
	EventBus.war_declared.emit(attacker, defender)


func propose_peace(proposer: int, target: int) -> bool:
	if proposer == target:
		return false
	var key := _pair_key(proposer, target)
	if not relations.has(key):
		return false
	var rel := relations[key] as Dictionary
	if not rel["at_war"]:
		return true  # already at peace

	# AI acceptance: accept if reputation > -30 or random chance
	var accept := rel["reputation"] > -30
	if not accept:
		# Small random chance even with bad relations
		var rng := RandomNumberGenerator.new()
		rng.randomize()
		accept = rng.randf() < 0.15

	if accept:
		rel["at_war"] = false
		rel["peace_turns"] = 10  # cooldown
		rel["reputation"] += 10
		EventBus.peace_made.emit(proposer, target)
		return true
	return false


func propose_alliance(proposer: int, target: int) -> bool:
	if proposer == target:
		return false
	var key := _pair_key(proposer, target)
	if not relations.has(key):
		return false
	var rel := relations[key] as Dictionary
	if rel["at_war"]:
		return false  # must be at peace first
	if rel["reputation"] < Constants.RELATION_ALLIANCE_THRESHOLD:
		return false  # not friendly enough
	rel["allied"] = true
	return true


func break_alliance(a: int, b: int) -> void:
	var key := _pair_key(a, b)
	if not relations.has(key):
		return
	var rel := relations[key] as Dictionary
	rel["allied"] = false
	rel["reputation"] -= 20


# ---------------------------------------------------------------------------
# Reputation
# ---------------------------------------------------------------------------

func modify_reputation(a: int, b: int, delta: int, reason: String) -> void:
	if a == b:
		return
	var key := _pair_key(a, b)
	if not relations.has(key):
		return
	var rel := relations[key] as Dictionary
	rel["reputation"] = clampi(rel["reputation"] + delta, Constants.RELATION_MIN, Constants.RELATION_MAX)

	# Auto-declare war if reputation drops below threshold
	if rel["reputation"] <= Constants.RELATION_WAR_THRESHOLD and not rel["at_war"]:
		declare_war(a, b)


# ---------------------------------------------------------------------------
# Per-turn processing
# ---------------------------------------------------------------------------

func process_turn() -> void:
	for rel in relations.values():
		# Peace cooldown
		if rel["peace_turns"] > 0:
			rel["peace_turns"] -= 1

		# Slow reputation drift toward neutral
		if rel["reputation"] > 0:
			rel["reputation"] -= 1
		elif rel["reputation"] < 0:
			rel["reputation"] += 1


# ---------------------------------------------------------------------------
# Internal
# ---------------------------------------------------------------------------

func _pair_key(a: int, b: int) -> String:
	return "%d_%d" % [mini(a, b), maxi(a, b)]
