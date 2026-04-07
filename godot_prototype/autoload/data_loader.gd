extends Node
## Loads all JSON data files from res://data/ at startup.
## Provides lookup helpers for spells, units, buildings, etc.

var spells: Array = []
var unit_specs: Dictionary = {}
var building_specs: Dictionary = {}
var terrain_types: Dictionary = {}

var _data_loaded: bool = false


func _ready() -> void:
	_load_all_data()
	_data_loaded = true


func _load_all_data() -> void:
	spells = _load_json("res://data/spells.json", [])
	unit_specs = _load_json("res://data/units.json", {})
	building_specs = _load_json("res://data/buildings.json", {})
	terrain_types = _load_json("res://data/terrain.json", {})


func _load_json(path: String, fallback: Variant) -> Variant:
	if not FileAccess.file_exists(path):
		push_warning("DataLoader: missing data file %s — using fallback" % path)
		return fallback
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		push_warning("DataLoader: cannot open %s" % path)
		return fallback
	var text := file.get_as_text()
	file.close()
	var json := JSON.new()
	var err := json.parse(text)
	if err != OK:
		push_error("DataLoader: JSON parse error in %s — %s" % [path, json.get_error_message()])
		return fallback
	return json.data


# ---------------------------------------------------------------------------
# Lookup helpers
# ---------------------------------------------------------------------------

func get_spell(id: String) -> Dictionary:
	for s in spells:
		if s.get("id", "") == id:
			return s
	push_warning("DataLoader: spell '%s' not found" % id)
	return {}


func get_spells_by_realm(realm: String) -> Array:
	var result: Array = []
	for s in spells:
		if s.get("realm", "") == realm:
			result.append(s)
	return result


func get_unit_spec(id: String) -> Dictionary:
	if unit_specs.has(id):
		return unit_specs[id]
	push_warning("DataLoader: unit spec '%s' not found" % id)
	return {}


func get_building_spec(id: String) -> Dictionary:
	if building_specs.has(id):
		return building_specs[id]
	push_warning("DataLoader: building spec '%s' not found" % id)
	return {}


func get_terrain(id: int) -> Dictionary:
	var key := str(id)
	if terrain_types.has(key):
		return terrain_types[key]
	return {}
