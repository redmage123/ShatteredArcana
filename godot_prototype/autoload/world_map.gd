extends Node
## World generation and tile data storage.
## Each plane has a 2D grid of TileData. X wraps, Y does not.

# ---------------------------------------------------------------------------
# Tile data class
# ---------------------------------------------------------------------------
class TileData:
	var terrain: int = Constants.TERRAIN_OCEAN
	var owner: int = -1
	var city_id: int = -1
	var resource: String = ""       # "gold_mine", "iron_mine", "mana_node", etc.
	var has_road: bool = false
	var portal_id: int = -1
	var fog_explored: int = 0       # bitmask — bit N = wizard N has explored
	var fog_visible: int = 0        # bitmask — bit N = wizard N can see right now

	func duplicate_tile() -> TileData:
		var t := TileData.new()
		t.terrain = terrain
		t.owner = owner
		t.city_id = city_id
		t.resource = resource
		t.has_road = has_road
		t.portal_id = portal_id
		t.fog_explored = fog_explored
		t.fog_visible = fog_visible
		return t


# ---------------------------------------------------------------------------
# Storage: planes[plane_index] = Array of rows, each row = Array of TileData
# ---------------------------------------------------------------------------
var planes: Array = []  # planes[p][y][x] -> TileData

# City name pools per plane
var _city_names: Dictionary = {
	0: ["Aurelion", "Solhaven", "Dawnkeep", "Brighthollow", "Goldspire"],
	1: ["Umbrath", "Gloomreach", "Nighthollow", "Duskfang", "Voidmere"],
	2: ["Verdantia", "Mosshollow", "Thornwall", "Ferndeep", "Oakshade"],
	3: ["Pyrefell", "Ashvault", "Cinderhold", "Flamecrest", "Emberspire"],
	4: ["Frostholme", "Glaciara", "Rimegate", "Snowpeak", "Icehold"],
	5: ["Aetherveil", "Shimmergate", "Misthollow", "Dreamspire", "Fadereach"],
	6: ["Deepmurk", "Undergloom", "Shadowpith", "Darkhollow", "Blightcore"],
	7: ["Celestine", "Starhold", "Heavenreach", "Radiantkeep", "Halogate"],
}

# Per-plane terrain distribution weights  [ocean, grass, forest, hills, mountain, special1, special2]
# special slots map to plane-specific terrains
var _terrain_weights: Dictionary = {
	0: {Constants.TERRAIN_GRASSLAND: 40, Constants.TERRAIN_FOREST: 20, Constants.TERRAIN_HILLS: 15, Constants.TERRAIN_MOUNTAIN: 10, Constants.TERRAIN_OCEAN: 15},
	1: {Constants.TERRAIN_SHADOW: 30, Constants.TERRAIN_FOREST: 20, Constants.TERRAIN_CORRUPTED: 15, Constants.TERRAIN_MOUNTAIN: 10, Constants.TERRAIN_OCEAN: 25},
	2: {Constants.TERRAIN_FOREST: 40, Constants.TERRAIN_GRASSLAND: 20, Constants.TERRAIN_SWAMP: 15, Constants.TERRAIN_HILLS: 10, Constants.TERRAIN_OCEAN: 15},
	3: {Constants.TERRAIN_DESERT: 30, Constants.TERRAIN_VOLCANIC: 20, Constants.TERRAIN_HILLS: 15, Constants.TERRAIN_MOUNTAIN: 15, Constants.TERRAIN_OCEAN: 20},
	4: {Constants.TERRAIN_TUNDRA: 35, Constants.TERRAIN_MOUNTAIN: 20, Constants.TERRAIN_HILLS: 15, Constants.TERRAIN_OCEAN: 20, Constants.TERRAIN_GRASSLAND: 10},
	5: {Constants.TERRAIN_CRYSTAL: 30, Constants.TERRAIN_GRASSLAND: 25, Constants.TERRAIN_FOREST: 15, Constants.TERRAIN_HILLS: 10, Constants.TERRAIN_OCEAN: 20},
	6: {Constants.TERRAIN_SHADOW: 35, Constants.TERRAIN_CORRUPTED: 25, Constants.TERRAIN_SWAMP: 15, Constants.TERRAIN_MOUNTAIN: 10, Constants.TERRAIN_OCEAN: 15},
	7: {Constants.TERRAIN_GRASSLAND: 35, Constants.TERRAIN_CRYSTAL: 20, Constants.TERRAIN_FOREST: 15, Constants.TERRAIN_HILLS: 10, Constants.TERRAIN_OCEAN: 20},
}


func _ready() -> void:
	pass


# ---------------------------------------------------------------------------
# World generation
# ---------------------------------------------------------------------------

func generate_world(rng: RandomNumberGenerator) -> void:
	planes.clear()

	for p in range(Constants.NUM_PLANES):
		var plane_grid: Array = []
		for y in range(Constants.MAP_HEIGHT):
			var row: Array = []
			for x in range(Constants.MAP_WIDTH):
				var tile := TileData.new()
				tile.terrain = _pick_terrain(rng, p, x, y)
				row.append(tile)
			plane_grid.append(row)
		planes.append(plane_grid)

	# Place features after base terrain
	for p in range(Constants.NUM_PLANES):
		_place_cities(rng, p)
		_place_mana_nodes(rng, p)
		_place_resources(rng, p)
		_place_portals(rng, p)


func _pick_terrain(rng: RandomNumberGenerator, plane: int, x: int, y: int) -> int:
	# Simple noise-ish: use rng + edge ocean bias
	var edge_dist := mini(mini(y, Constants.MAP_HEIGHT - 1 - y), 4)
	if edge_dist == 0:
		return Constants.TERRAIN_OCEAN

	var weights: Dictionary = _terrain_weights.get(plane, _terrain_weights[0])
	# Increase ocean chance near edges
	var ocean_boost: int = maxi(0, 3 - edge_dist) * 15

	var total: int = ocean_boost
	for w in weights.values():
		total += w

	var roll: int = rng.randi_range(0, total - 1)
	# Check ocean boost first
	if roll < ocean_boost:
		return Constants.TERRAIN_OCEAN
	roll -= ocean_boost

	for terrain_id in weights:
		roll -= weights[terrain_id]
		if roll < 0:
			return terrain_id
	return Constants.TERRAIN_GRASSLAND


func _place_cities(rng: RandomNumberGenerator, plane: int) -> void:
	var count: int = rng.randi_range(3, 5)
	var names: Array = _city_names.get(plane, ["City"])
	var placed: int = 0
	var attempts: int = 0

	while placed < count and attempts < 200:
		attempts += 1
		var x: int = rng.randi_range(2, Constants.MAP_WIDTH - 3)
		var y: int = rng.randi_range(2, Constants.MAP_HEIGHT - 3)
		var tile: TileData = planes[plane][y][x]

		# Only place on habitable terrain
		if tile.terrain in [Constants.TERRAIN_GRASSLAND, Constants.TERRAIN_FOREST, Constants.TERRAIN_HILLS]:
			if tile.city_id == -1 and _no_city_nearby(plane, x, y, 5):
				var city_name: String = names[placed % names.size()]
				var city_id: int = CityManager.create_city(city_name, plane, Vector2i(x, y), -1)
				tile.city_id = city_id
				placed += 1


func _no_city_nearby(plane: int, cx: int, cy: int, radius: int) -> bool:
	for dy in range(-radius, radius + 1):
		for dx in range(-radius, radius + 1):
			var nx: int = wrap_x(cx + dx)
			var ny: int = cy + dy
			if ny < 0 or ny >= Constants.MAP_HEIGHT:
				continue
			if planes[plane][ny][nx].city_id != -1:
				return false
	return true


func _place_mana_nodes(rng: RandomNumberGenerator, plane: int) -> void:
	var count: int = rng.randi_range(2, 3)
	var placed: int = 0
	var attempts: int = 0
	while placed < count and attempts < 100:
		attempts += 1
		var x: int = rng.randi_range(1, Constants.MAP_WIDTH - 2)
		var y: int = rng.randi_range(1, Constants.MAP_HEIGHT - 2)
		var tile: TileData = planes[plane][y][x]
		if tile.terrain != Constants.TERRAIN_OCEAN and tile.resource == "":
			tile.resource = "mana_node"
			placed += 1


func _place_resources(rng: RandomNumberGenerator, plane: int) -> void:
	var resource_types: Array = ["gold_mine", "iron_mine", "gem_deposit", "adamantium_ore"]
	for _i in range(4):
		var attempts: int = 0
		while attempts < 50:
			attempts += 1
			var x: int = rng.randi_range(1, Constants.MAP_WIDTH - 2)
			var y: int = rng.randi_range(1, Constants.MAP_HEIGHT - 2)
			var tile: TileData = planes[plane][y][x]
			if tile.terrain in [Constants.TERRAIN_HILLS, Constants.TERRAIN_MOUNTAIN] and tile.resource == "":
				tile.resource = resource_types[_i % resource_types.size()]
				break


func _place_portals(rng: RandomNumberGenerator, plane: int) -> void:
	if plane >= Constants.NUM_PLANES - 1:
		return  # last plane has no outgoing portal
	var attempts: int = 0
	while attempts < 100:
		attempts += 1
		var x: int = rng.randi_range(3, Constants.MAP_WIDTH - 4)
		var y: int = rng.randi_range(3, Constants.MAP_HEIGHT - 4)
		var tile: TileData = planes[plane][y][x]
		if tile.terrain != Constants.TERRAIN_OCEAN and tile.resource == "" and tile.city_id == -1:
			tile.portal_id = plane + 1  # leads to next plane
			break


# ---------------------------------------------------------------------------
# Tile access
# ---------------------------------------------------------------------------

func get_tile(plane: int, x: int, y: int) -> TileData:
	if plane < 0 or plane >= planes.size():
		return null
	var wx: int = wrap_x(x)
	if y < 0 or y >= Constants.MAP_HEIGHT:
		return null
	return planes[plane][y][wx]


func set_tile_owner(plane: int, x: int, y: int, wizard_id: int) -> void:
	var tile := get_tile(plane, x, y)
	if tile:
		tile.owner = wizard_id


func wrap_x(x: int) -> int:
	return posmod(x, Constants.MAP_WIDTH)


func is_passable(plane: int, x: int, y: int, movement_type: String = "walk") -> bool:
	var tile := get_tile(plane, x, y)
	if tile == null:
		return false
	match movement_type:
		"fly":
			return true  # flyers cross everything
		"swim":
			return tile.terrain == Constants.TERRAIN_OCEAN
		_:  # walk
			return tile.terrain != Constants.TERRAIN_OCEAN
