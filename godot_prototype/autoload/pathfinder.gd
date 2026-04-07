extends Node
## A* pathfinding with wrap-X support.
## Uses a simple sorted-Array priority queue for GDScript compatibility.

# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func find_path(plane: int, from: Vector2i, to: Vector2i, movement_type: String = "walk") -> Array[Vector2i]:
	## Returns ordered array of positions from `from` to `to` (inclusive).
	## Empty array if no path found.
	if from == to:
		return [from]

	var open: Array = []  # [{pos, g, f, parent}]
	var closed: Dictionary = {}  # pos_key -> true
	var came_from: Dictionary = {}  # pos_key -> parent pos_key
	var g_scores: Dictionary = {}  # pos_key -> float

	var start_key := _pos_key(from)
	var goal_key := _pos_key(to)

	g_scores[start_key] = 0.0
	open.append({
		"pos": from,
		"g": 0.0,
		"f": _heuristic(from, to),
		"key": start_key,
	})

	var iterations: int = 0
	var max_iterations: int = Constants.MAP_WIDTH * Constants.MAP_HEIGHT * 2

	while not open.is_empty() and iterations < max_iterations:
		iterations += 1

		# Find node with lowest f
		var best_idx: int = 0
		for i in range(1, open.size()):
			if open[i]["f"] < open[best_idx]["f"]:
				best_idx = i
		var current: Dictionary = open[best_idx]
		open.remove_at(best_idx)

		if current["key"] == goal_key:
			return _reconstruct_path(came_from, start_key, goal_key, from, to)

		closed[current["key"]] = true

		# Expand neighbors (4-directional)
		for neighbor_pos in _get_neighbors(current["pos"]):
			var nkey := _pos_key(neighbor_pos)
			if closed.has(nkey):
				continue
			if not WorldMap.is_passable(plane, neighbor_pos.x, neighbor_pos.y, movement_type):
				continue

			var move_cost := get_move_cost(plane, neighbor_pos, movement_type)
			var tentative_g: float = current["g"] + move_cost

			if not g_scores.has(nkey) or tentative_g < g_scores[nkey]:
				g_scores[nkey] = tentative_g
				came_from[nkey] = current["key"]
				var f: float = tentative_g + _heuristic(neighbor_pos, to)

				# Check if already in open with higher f
				var found_in_open := false
				for i in range(open.size()):
					if open[i]["key"] == nkey:
						if open[i]["f"] > f:
							open[i]["g"] = tentative_g
							open[i]["f"] = f
						found_in_open = true
						break
				if not found_in_open:
					open.append({
						"pos": neighbor_pos,
						"g": tentative_g,
						"f": f,
						"key": nkey,
					})

	# No path found
	return []


func get_move_cost(plane: int, pos: Vector2i, movement_type: String) -> float:
	if movement_type == "fly":
		return 1.0  # flyers pay 1 for everything

	var tile := WorldMap.get_tile(plane, pos.x, pos.y)
	if tile == null:
		return 99.0

	var base_cost: int = Constants.MOVE_COST.get(tile.terrain, 2)

	# Roads reduce cost
	if tile.has_road:
		return maxf(0.5, float(base_cost) * 0.5)

	return float(base_cost)


# ---------------------------------------------------------------------------
# Heuristic
# ---------------------------------------------------------------------------

func _heuristic(a: Vector2i, b: Vector2i) -> float:
	## Manhattan distance with X-wrap.
	var dx: int = absi(a.x - b.x)
	dx = mini(dx, Constants.MAP_WIDTH - dx)
	var dy: int = absi(a.y - b.y)
	return float(dx + dy)


# ---------------------------------------------------------------------------
# Neighbors (4-directional, wrap X)
# ---------------------------------------------------------------------------

func _get_neighbors(pos: Vector2i) -> Array[Vector2i]:
	var result: Array[Vector2i] = []
	# Right
	result.append(Vector2i(WorldMap.wrap_x(pos.x + 1), pos.y))
	# Left
	result.append(Vector2i(WorldMap.wrap_x(pos.x - 1), pos.y))
	# Down
	if pos.y + 1 < Constants.MAP_HEIGHT:
		result.append(Vector2i(pos.x, pos.y + 1))
	# Up
	if pos.y - 1 >= 0:
		result.append(Vector2i(pos.x, pos.y - 1))
	return result


# ---------------------------------------------------------------------------
# Path reconstruction
# ---------------------------------------------------------------------------

func _reconstruct_path(came_from: Dictionary, start_key: String, goal_key: String,
		start_pos: Vector2i, goal_pos: Vector2i) -> Array[Vector2i]:
	var path: Array[Vector2i] = []
	var current_key := goal_key

	# Build reverse path from keys
	var key_path: Array[String] = [goal_key]
	while current_key != start_key and came_from.has(current_key):
		current_key = came_from[current_key]
		key_path.append(current_key)

	key_path.reverse()

	# Convert keys back to positions
	for k in key_path:
		path.append(_key_to_pos(k))

	return path


# ---------------------------------------------------------------------------
# Key utilities
# ---------------------------------------------------------------------------

func _pos_key(pos: Vector2i) -> String:
	return "%d,%d" % [pos.x, pos.y]


func _key_to_pos(key: String) -> Vector2i:
	var parts := key.split(",")
	return Vector2i(int(parts[0]), int(parts[1]))
