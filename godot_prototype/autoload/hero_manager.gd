extends Node
## Simplified hero management.
## Heroes are special units with stats, levels, equipment slots, and combat bonuses.

var hero_data: Dictionary = {}  # hero_id -> hero dict
var _next_hero_id: int = 1

# Hero class templates
const HERO_CLASSES: Dictionary = {
	"warrior": {
		"base_hp": 20,
		"base_melee": 8,
		"base_ranged": 0,
		"base_defense": 5,
		"base_resistance": 4,
		"movement": 3,
		"abilities": ["leadership"],
		"combat_bonus": 1.15,
	},
	"ranger": {
		"base_hp": 15,
		"base_melee": 4,
		"base_ranged": 7,
		"base_defense": 3,
		"base_resistance": 5,
		"movement": 4,
		"abilities": ["scout", "first_strike"],
		"combat_bonus": 1.1,
	},
	"mage": {
		"base_hp": 12,
		"base_melee": 2,
		"base_ranged": 3,
		"base_defense": 2,
		"base_resistance": 8,
		"movement": 2,
		"abilities": ["caster", "arcane_power"],
		"combat_bonus": 1.2,
	},
	"paladin": {
		"base_hp": 18,
		"base_melee": 6,
		"base_ranged": 0,
		"base_defense": 6,
		"base_resistance": 6,
		"movement": 3,
		"abilities": ["holy_armor", "leadership"],
		"combat_bonus": 1.15,
	},
}

const HERO_NAMES: Array = [
	"Aldric", "Branwen", "Corvus", "Daria", "Elowen",
	"Fenris", "Gwendolyn", "Hadrian", "Isolde", "Jasper",
	"Kaelith", "Lyanna", "Mordecai", "Nyx", "Orin",
	"Petra", "Quinn", "Rowan", "Seraphina", "Theron",
]


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Hero lifecycle
# ---------------------------------------------------------------------------

func create_hero(hero_class: String, owner: int) -> int:
	var template: Dictionary = HERO_CLASSES.get(hero_class, HERO_CLASSES["warrior"])
	var hid := _next_hero_id
	_next_hero_id += 1

	var rng := RandomNumberGenerator.new()
	rng.randomize()

	var hero: Dictionary = {
		"hero_id": hid,
		"name": HERO_NAMES[rng.randi_range(0, HERO_NAMES.size() - 1)],
		"hero_class": hero_class,
		"owner": owner,
		"level": 1,
		"xp": 0,
		"hp": template["base_hp"],
		"max_hp": template["base_hp"],
		"melee_attack": template["base_melee"],
		"ranged_attack": template["base_ranged"],
		"defense": template["base_defense"],
		"resistance": template["base_resistance"],
		"movement": template["movement"],
		"abilities": template["abilities"].duplicate(),
		"combat_bonus_mult": template["combat_bonus"],
		"equipment": {"weapon": "", "armor": "", "accessory": ""},
	}
	hero_data[hid] = hero
	return hid


func get_hero(hero_id: int) -> Dictionary:
	return hero_data.get(hero_id, {})


func get_heroes_for_wizard(wizard_id: int) -> Array:
	var result: Array = []
	for hero in hero_data.values():
		if hero["owner"] == wizard_id:
			result.append(hero)
	return result


func remove_hero(hero_id: int) -> void:
	hero_data.erase(hero_id)


# ---------------------------------------------------------------------------
# Combat bonus
# ---------------------------------------------------------------------------

func get_hero_combat_bonus(hero_id: int) -> float:
	var hero := get_hero(hero_id)
	if hero.is_empty():
		return 1.0

	var bonus: float = hero.get("combat_bonus_mult", 1.0)

	# Level scaling: +2% per level beyond 1
	bonus += float(hero.get("level", 1) - 1) * 0.02

	# Leadership ability boosts army
	if "leadership" in hero.get("abilities", []):
		bonus += 0.05

	return bonus


# ---------------------------------------------------------------------------
# XP and leveling
# ---------------------------------------------------------------------------

func add_hero_xp(hero_id: int, amount: int) -> void:
	if not hero_data.has(hero_id):
		return
	var hero := hero_data[hero_id] as Dictionary
	hero["xp"] += amount

	while hero["xp"] >= Constants.XP_PER_LEVEL and hero["level"] < Constants.MAX_LEVEL:
		hero["xp"] -= Constants.XP_PER_LEVEL
		hero["level"] += 1
		hero["max_hp"] += 3
		hero["hp"] = hero["max_hp"]
		hero["melee_attack"] += 1
		hero["defense"] += 1
		hero["resistance"] += 1
