extends Node
## Simplified quest system.
## Generates quests for wizards based on game state and tracks completion.

var all_quests: Dictionary = {}  # quest_id -> quest dict
var _next_quest_id: int = 1

# Quest templates
const QUEST_TYPES: Array = [
	{
		"type": "explore",
		"title": "Explore the Unknown",
		"description": "Explore %d unexplored tiles on plane %d.",
		"reward_gold": 50,
		"reward_fame": 5,
		"target_count": 20,
	},
	{
		"type": "conquer_city",
		"title": "Conquer a City",
		"description": "Capture an enemy city.",
		"reward_gold": 100,
		"reward_fame": 10,
		"target_count": 1,
	},
	{
		"type": "build_army",
		"title": "Raise an Army",
		"description": "Have at least %d units in your armies.",
		"reward_gold": 75,
		"reward_fame": 5,
		"target_count": 10,
	},
	{
		"type": "research_spell",
		"title": "Arcane Research",
		"description": "Research %d spells.",
		"reward_gold": 60,
		"reward_fame": 8,
		"target_count": 2,
	},
	{
		"type": "win_battles",
		"title": "Prove Your Might",
		"description": "Win %d battles.",
		"reward_gold": 80,
		"reward_fame": 10,
		"target_count": 3,
	},
	{
		"type": "claim_nodes",
		"title": "Harness the Ley Lines",
		"description": "Control %d mana nodes.",
		"reward_gold": 70,
		"reward_fame": 8,
		"target_count": 2,
	},
]


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Quest generation
# ---------------------------------------------------------------------------

func generate_quest(wizard_id: int, turn: int) -> int:
	var rng = RandomNumberGenerator.new()
	rng.seed = wizard_id * 1000 + turn

	var template: Dictionary = QUEST_TYPES[rng.randi_range(0, QUEST_TYPES.size() - 1)]
	var qid = _next_quest_id
	_next_quest_id += 1

	var quest: Dictionary = {
		"quest_id": qid,
		"wizard_id": wizard_id,
		"type": template["type"],
		"title": template["title"],
		"description": template["description"],
		"reward_gold": template["reward_gold"],
		"reward_fame": template["reward_fame"],
		"target_count": template["target_count"],
		"current_count": 0,
		"status": "available",  # available, accepted, completed, failed
		"turn_offered": turn,
		"turn_deadline": turn + 30,  # 30 turns to complete
	}
	all_quests[qid] = quest
	EventBus.quest_available.emit(qid)
	return qid


# ---------------------------------------------------------------------------
# Quest actions
# ---------------------------------------------------------------------------

func accept_quest(quest_id: int, wizard_id: int) -> void:
	if not all_quests.has(quest_id):
		return
	var quest = all_quests[quest_id] as Dictionary
	if quest["wizard_id"] != wizard_id:
		return
	if quest["status"] != "available":
		return
	quest["status"] = "accepted"


func abandon_quest(quest_id: int) -> void:
	if not all_quests.has(quest_id):
		return
	all_quests[quest_id]["status"] = "failed"
	EventBus.quest_failed.emit(quest_id)


func get_quests_for_wizard(wizard_id: int) -> Array:
	var result: Array = []
	for quest in all_quests.values():
		if quest["wizard_id"] == wizard_id:
			result.append(quest)
	return result


func get_active_quests(wizard_id: int) -> Array:
	var result: Array = []
	for quest in all_quests.values():
		if quest["wizard_id"] == wizard_id and quest["status"] == "accepted":
			result.append(quest)
	return result


# ---------------------------------------------------------------------------
# Progress checking
# ---------------------------------------------------------------------------

func check_quest_progress(wizard_id: int) -> void:
	for quest in all_quests.values():
		if quest["wizard_id"] != wizard_id or quest["status"] != "accepted":
			continue

		match quest["type"]:
			"build_army":
				var total_units: int = 0
				for army in UnitManager.get_armies_for_wizard(wizard_id):
					total_units += army["unit_ids"].size()
				quest["current_count"] = total_units

			"conquer_city":
				var cities = CityManager.get_cities_for_wizard(wizard_id)
				quest["current_count"] = cities.size()

			"research_spell":
				var state = MagicSystem.get_magic_state(wizard_id)
				if not state.is_empty():
					quest["current_count"] = state["known_spells"].size()

			"claim_nodes":
				var state = MagicSystem.get_magic_state(wizard_id)
				if not state.is_empty():
					quest["current_count"] = state["controlled_nodes"].size()

			"win_battles":
				# Tracked externally via signal; increment in _on_combat_resolved
				pass

			"explore":
				# Count explored tiles (simplified — count total explored on any plane)
				var count: int = 0
				for plane_grid in WorldMap.planes:
					for row in plane_grid:
						for tile in row:
							if (tile.fog_explored & (1 << wizard_id)) != 0:
								count += 1
				quest["current_count"] = count

		# Check completion
		if quest["current_count"] >= quest["target_count"]:
			_complete_quest(quest)


func _complete_quest(quest: Dictionary) -> void:
	quest["status"] = "completed"
	var wizard_id: int = quest["wizard_id"]
	GameState.add_gold(wizard_id, quest["reward_gold"])
	GameState.add_fame(wizard_id, quest["reward_fame"])
	EventBus.quest_completed.emit(quest["quest_id"])


# ---------------------------------------------------------------------------
# Per-turn processing
# ---------------------------------------------------------------------------

func process_turn(turn: int) -> void:
	# Check progress for all wizards
	for wiz in GameState.wizards:
		if wiz["is_eliminated"]:
			continue
		check_quest_progress(wiz["id"])

	# Expire overdue quests
	for quest in all_quests.values():
		if quest["status"] == "accepted" and turn > quest["turn_deadline"]:
			quest["status"] = "failed"
			EventBus.quest_failed.emit(quest["quest_id"])

	# Offer new quests every 10 turns
	if turn % 10 == 0:
		for wiz in GameState.wizards:
			if wiz["is_eliminated"]:
				continue
			# Only offer if wizard has fewer than 3 active quests
			var active = get_active_quests(wiz["id"])
			if active.size() < 3:
				generate_quest(wiz["id"], turn)

				# AI auto-accepts quests
				if not wiz["is_human"]:
					var quests = get_quests_for_wizard(wiz["id"])
					for q in quests:
						if q["status"] == "available":
							accept_quest(q["quest_id"], wiz["id"])
