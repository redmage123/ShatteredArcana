extends TextureRect
## Minimap rendering and click-to-navigate.
## Draws a small overview of the current plane with terrain, cities, and armies.
## Click on the minimap to jump the camera to that world position.

signal camera_jump_requested(world_pos: Vector2)

const MINIMAP_WIDTH: int = 200
const MINIMAP_HEIGHT: int = 125

# Terrain color mapping (terrain type int -> color)
const TERRAIN_COLORS: Dictionary = {
	0:  Color(0.10, 0.25, 0.50),  # ocean - deep blue
	1:  Color(0.30, 0.60, 0.25),  # grassland - green
	2:  Color(0.18, 0.42, 0.18),  # forest - dark green
	3:  Color(0.55, 0.50, 0.35),  # hills - tan
	4:  Color(0.50, 0.50, 0.55),  # mountain - grey
	5:  Color(0.82, 0.75, 0.50),  # desert - sand
	6:  Color(0.30, 0.40, 0.30),  # swamp - murky green
	7:  Color(0.85, 0.88, 0.92),  # tundra - pale white
	8:  Color(0.25, 0.15, 0.30),  # shadow - dark purple
	9:  Color(0.45, 0.20, 0.25),  # corrupted - dark red
	10: Color(0.70, 0.30, 0.10),  # volcanic - orange red
	11: Color(0.60, 0.70, 0.90),  # crystal - light blue
}

const FOG_COLOR := Color(0.05, 0.05, 0.08)
const CITY_COLOR := Color(1.0, 1.0, 1.0)

var _current_plane: int = 0
var _player_id: int = 0
var _image: Image
var _needs_update: bool = true


func _ready() -> void:
	custom_minimum_size = Vector2(MINIMAP_WIDTH, MINIMAP_HEIGHT)
	stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	mouse_filter = Control.MOUSE_FILTER_STOP

	# Initialize image
	_image = Image.create(MINIMAP_WIDTH, MINIMAP_HEIGHT, false, Image.FORMAT_RGBA8)
	_image.fill(Color.BLACK)
	texture = ImageTexture.create_from_image(_image)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

func set_plane(plane: int) -> void:
	_current_plane = plane
	_needs_update = true


func set_player_id(pid: int) -> void:
	_player_id = pid


func refresh() -> void:
	_needs_update = true
	_render_minimap()


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

func _render_minimap() -> void:
	_needs_update = false
	_image.fill(FOG_COLOR)

	var map_w: int = Constants.MAP_WIDTH
	var map_h: int = Constants.MAP_HEIGHT

	# Scale factors: how many pixels per tile
	var sx: float = float(MINIMAP_WIDTH) / float(map_w)
	var sy: float = float(MINIMAP_HEIGHT) / float(map_h)

	# Draw terrain
	for ty in range(map_h):
		for tx in range(map_w):
			# Fog check
			if not FogOfWar.is_visible(_player_id, _current_plane, Vector2i(tx, ty)):
				continue

			var tile := WorldMap.get_tile(_current_plane, tx, ty)
			if tile == null:
				continue

			var terrain: int = tile.terrain
			var color: Color = TERRAIN_COLORS.get(terrain, Color.MAGENTA)

			# Draw the pixel(s) for this tile
			var px_start := int(float(tx) * sx)
			var py_start := int(float(ty) * sy)
			var px_end := int(float(tx + 1) * sx)
			var py_end := int(float(ty + 1) * sy)

			for py in range(py_start, mini(py_end, MINIMAP_HEIGHT)):
				for px in range(px_start, mini(px_end, MINIMAP_WIDTH)):
					_image.set_pixel(px, py, color)

	# Draw cities as white dots (2x2)
	for city in CityManager.all_cities.values():
		if city.get("plane", -1) != _current_plane:
			continue
		var cpos: Vector2i = city.get("pos", Vector2i.ZERO)
		if not FogOfWar.is_visible(_player_id, _current_plane, cpos):
			continue

		var cx := int(float(cpos.x) * sx)
		var cy := int(float(cpos.y) * sy)
		var owner_id: int = city.get("owner", -1)
		var dot_color: Color = CITY_COLOR
		if owner_id >= 0:
			dot_color = GameState.get_wizard_color(owner_id) if GameState.has_method("get_wizard_color") else CITY_COLOR

		for dy in range(3):
			for dx in range(3):
				var px := cx + dx - 1
				var py := cy + dy - 1
				if px >= 0 and px < MINIMAP_WIDTH and py >= 0 and py < MINIMAP_HEIGHT:
					_image.set_pixel(px, py, dot_color)

	# Draw armies as colored dots
	for army in UnitManager.all_armies.values():
		if army.get("plane", -1) != _current_plane:
			continue
		var apos: Vector2i = army.get("pos", Vector2i.ZERO)
		if not FogOfWar.is_visible(_player_id, _current_plane, apos):
			continue

		var ax := int(float(apos.x) * sx)
		var ay := int(float(apos.y) * sy)
		var owner_id: int = army.get("owner", -1)
		var army_color: Color = Color.RED
		if owner_id >= 0:
			army_color = GameState.get_wizard_color(owner_id) if GameState.has_method("get_wizard_color") else Color.RED

		for dy in range(2):
			for dx in range(2):
				var px := ax + dx
				var py := ay + dy
				if px >= 0 and px < MINIMAP_WIDTH and py >= 0 and py < MINIMAP_HEIGHT:
					_image.set_pixel(px, py, army_color)

	# Update the texture
	if texture is ImageTexture:
		(texture as ImageTexture).update(_image)
	else:
		texture = ImageTexture.create_from_image(_image)


# ---------------------------------------------------------------------------
# Input -- click to jump camera
# ---------------------------------------------------------------------------

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var local_pos := event.position
		# Convert minimap pixel to world tile
		var map_w: int = Constants.MAP_WIDTH
		var map_h: int = Constants.MAP_HEIGHT
		var tile_x := int(float(local_pos.x) / float(MINIMAP_WIDTH) * float(map_w))
		var tile_y := int(float(local_pos.y) / float(MINIMAP_HEIGHT) * float(map_h))
		tile_x = clampi(tile_x, 0, map_w - 1)
		tile_y = clampi(tile_y, 0, map_h - 1)

		# Convert tile to world position (using world TILE_SIZE from world.gd)
		var world_tile_size := 128  # matches world.gd TILE_SIZE
		var world_pos := Vector2(
			float(tile_x * world_tile_size + world_tile_size / 2),
			float(tile_y * world_tile_size + world_tile_size / 2)
		)
		camera_jump_requested.emit(world_pos)
