extends Node
## Central game state and turn processing.
## Orchestrates the turn loop and holds per-wizard data.

var current_turn: int = 0
var current_wizard: int = 0
var num_wizards: int = 4
var wizards: Array[Dictionary] = []
var turn_order: Array[int] = []
var game_over: bool = false
var winner_id: int = -1

# Wizard template used by initialize_game
const _WIZARD_TEMPLATE: Dictionary = {
	"id": 0,
	"name": "",
	"realm": "",
	"is_human": false,
	"is_eliminated": false,
	"gold": 100,
	"mana": 50,
	"fame": 0,
}

# Default wizard names for AI
const _AI_NAMES: Array = [
	"Kael the Wise", "Morvath Shadowbind", "Elara Sunforge",
	"Grimjaw Stonecaller", "Veyra Frostwhisper", "Zorath Pyremane",
	"Illuna Starweaver", "Drekthar Ashblood",
]

# Default realms cycled for AI
const _REALMS: Array = [
	"arcane", "nature", "shadow", "fire", "ice",
]


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Initialization
# ---------------------------------------------------------------------------

func initialize_game(num_humans: int, num_ai: int) -> void:
	num_wizards = num_humans + num_ai
	wizards.clear()
	turn_order.clear()
	current_turn = 1
	current_wizard = 0
	game_over = false
	winner_id = -1

	for i in range(num_wizards):
		var wiz = _WIZARD_TEMPLATE.duplicate(true)
		wiz["id"] = i
		wiz["is_human"] = i < num_humans
		if wiz["is_human"]:
			wiz["name"] = "Player %d" % (i + 1)
		else:
			wiz["name"] = _AI_NAMES[i % _AI_NAMES.size()]
		wiz["realm"] = _REALMS[i % _REALMS.size()]
		wizards.append(wiz)
		turn_order.append(i)


# ---------------------------------------------------------------------------
# Turn flow
# ---------------------------------------------------------------------------

func end_player_turn() -> void:
	if game_over:
		return

	# Advance to next non-eliminated wizard
	var start = current_wizard
	current_wizard = _next_active_wizard(current_wizard)

	# If we wrapped around back to or past the first player, full turn ends
	if current_wizard <= start:
		process_full_turn()

	if is_human_turn():
		EventBus.turn_started.emit(current_turn)
	else:
		# AI takes its turn immediately, then advance again
		AIController.process_ai_turn(current_wizard)
		end_player_turn()


func process_full_turn() -> void:
	# Subsystem per-turn processing order
	CityManager.process_production()
	MagicSystem.process_turn()
	CombatResolver.resolve_all_encounters()
	FogOfWar.update_all_vision()
	QuestSystem.process_turn(current_turn)
	_check_eliminations()
	_check_victory()

	current_turn += 1
	EventBus.turn_ended.emit(current_turn - 1)
	EventBus.turn_started.emit(current_turn)


func start_game() -> void:
	WorldMap.generate_world(RandomNumberGenerator.new())
	FogOfWar.update_all_vision()
	EventBus.turn_started.emit(current_turn)


# ---------------------------------------------------------------------------
# Queries
# ---------------------------------------------------------------------------

func is_human_turn() -> bool:
	if current_wizard < 0 or current_wizard >= wizards.size():
		return false
	return wizards[current_wizard].get("is_human", false)


func get_current_wizard() -> Dictionary:
	if current_wizard < 0 or current_wizard >= wizards.size():
		return {}
	return wizards[current_wizard]


func get_wizard(wizard_id: int) -> Dictionary:
	if wizard_id < 0 or wizard_id >= wizards.size():
		return {}
	return wizards[wizard_id]


func eliminate_wizard(wizard_id: int) -> void:
	if wizard_id < 0 or wizard_id >= wizards.size():
		return
	wizards[wizard_id]["is_eliminated"] = true
	EventBus.wizard_eliminated.emit(wizard_id)


# ---------------------------------------------------------------------------
# Resource helpers
# ---------------------------------------------------------------------------

func add_gold(wizard_id: int, amount: int) -> void:
	if wizard_id >= 0 and wizard_id < wizards.size():
		wizards[wizard_id]["gold"] = maxi(0, wizards[wizard_id]["gold"] + amount)


func add_mana(wizard_id: int, amount: int) -> void:
	if wizard_id >= 0 and wizard_id < wizards.size():
		wizards[wizard_id]["mana"] = maxi(0, wizards[wizard_id]["mana"] + amount)


func add_fame(wizard_id: int, amount: int) -> void:
	if wizard_id >= 0 and wizard_id < wizards.size():
		wizards[wizard_id]["fame"] += amount


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

func _next_active_wizard(from: int) -> int:
	var idx = from
	for _i in range(num_wizards):
		idx = (idx + 1) % num_wizards
		if not wizards[idx].get("is_eliminated", false):
			return idx
	return from  # everyone eliminated — should not happen


func _check_eliminations() -> void:
	for wiz in wizards:
		if wiz["is_eliminated"]:
			continue
		var has_cities = CityManager.get_cities_for_wizard(wiz["id"]).size() > 0
		var has_armies = UnitManager.get_armies_for_wizard(wiz["id"]).size() > 0
		if not has_cities and not has_armies:
			eliminate_wizard(wiz["id"])


func _check_victory() -> void:
	# Conquest: only one wizard remains
	var alive: Array[int] = []
	for wiz in wizards:
		if not wiz["is_eliminated"]:
			alive.append(wiz["id"])
	if alive.size() == 1:
		game_over = true
		winner_id = alive[0]
		EventBus.victory_achieved.emit(winner_id, Constants.VICTORY_CONQUEST)
		return

	# Score victory at turn limit
	if current_turn >= Constants.SCORE_TURN_LIMIT:
		var best_id: int = -1
		var best_fame: int = -1
		for wiz in wizards:
			if not wiz["is_eliminated"] and wiz["fame"] > best_fame:
				best_fame = wiz["fame"]
				best_id = wiz["id"]
		game_over = true
		winner_id = best_id
		EventBus.victory_achieved.emit(winner_id, Constants.VICTORY_SCORE)
