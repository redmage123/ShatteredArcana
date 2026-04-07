extends Node
## Port of CoMFogOfWarSubsystem.
## Tracks explored and visible tiles per wizard using bitmasks on TileData.

func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# Vision update cycle
# ---------------------------------------------------------------------------

func clear_vision() -> void:
	## Clear current-turn visibility (but keep explored state).
	for plane_grid in WorldMap.planes:
		for row in plane_grid:
			for tile in row:
				tile.fog_visible = 0


func update_all_vision() -> void:
	clear_vision()
	for wiz in GameState.wizards:
		if not wiz["is_eliminated"]:
			update_vision_for_wizard(wiz["id"])


func update_vision_for_wizard(wizard_id: int) -> void:
	var bit: int = 1 << wizard_id

	# Vision from armies
	for army in UnitManager.get_armies_for_wizard(wizard_id):
		var sight: int = _get_army_sight_range(army)
		reveal_area(wizard_id, army["plane"], army["pos"], sight)

	# Vision from cities
	for city in CityManager.get_cities_for_wizard(wizard_id):
		reveal_area(wizard_id, city["plane"], city["pos"], Constants.CITY_SIGHT_RANGE)


# ---------------------------------------------------------------------------
# Reveal
# ---------------------------------------------------------------------------

func reveal_area(wizard_id: int, plane: int, center: Vector2i, radius: int) -> void:
	var bit: int = 1 << wizard_id
	for dy in range(-radius, radius + 1):
		for dx in range(-radius, radius + 1):
			if dx * dx + dy * dy > radius * radius:
				continue  # circular reveal
			var x: int = WorldMap.wrap_x(center.x + dx)
			var y: int = center.y + dy
			if y < 0 or y >= Constants.MAP_HEIGHT:
				continue
			var tile := WorldMap.get_tile(plane, x, y)
			if tile:
				tile.fog_visible |= bit
				tile.fog_explored |= bit


# ---------------------------------------------------------------------------
# Queries
# ---------------------------------------------------------------------------

func is_visible(wizard_id: int, plane: int, pos: Vector2i) -> bool:
	var tile := WorldMap.get_tile(plane, pos.x, pos.y)
	if tile == null:
		return false
	return (tile.fog_visible & (1 << wizard_id)) != 0


func is_explored(wizard_id: int, plane: int, pos: Vector2i) -> bool:
	var tile := WorldMap.get_tile(plane, pos.x, pos.y)
	if tile == null:
		return false
	return (tile.fog_explored & (1 << wizard_id)) != 0


func set_all_visible(wizard_id: int) -> void:
	## Debug / spell effect: reveal everything for a wizard.
	var bit: int = 1 << wizard_id
	for plane_grid in WorldMap.planes:
		for row in plane_grid:
			for tile in row:
				tile.fog_visible |= bit
				tile.fog_explored |= bit


# ---------------------------------------------------------------------------
# Internal
# ---------------------------------------------------------------------------

func _get_army_sight_range(army: Dictionary) -> int:
	var best: int = Constants.DEFAULT_SIGHT_RANGE
	for uid in army.get("unit_ids", []):
		var unit := UnitManager.get_unit(uid)
		if unit.is_empty():
			continue
		if "scout" in unit.get("abilities", []):
			best = maxi(best, Constants.SCOUT_SIGHT_RANGE)
	# Heroes give +1 sight
	if army.get("hero_id", -1) >= 0:
		best += 1
	return best
