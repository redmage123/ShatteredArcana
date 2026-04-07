extends Node2D
## Virtual tilemap renderer — creates Sprite2D nodes only for tiles within
## the camera viewport plus a small buffer.  Sprites are pooled and recycled.

const TILE_SIZE := 128
const MAX_SPRITES := 300

var _plane: int = 0
var _tile_sprites: Dictionary = {}   # Vector2i -> Sprite2D
var _sprite_pool: Array[Sprite2D] = []
var _prev_rect := Rect2i()

# Terrain colour fallbacks (used when PNG textures are not yet present)
var _terrain_colors: Dictionary = {
	0:  Color(0.10, 0.23, 0.36),  # ocean
	1:  Color(0.29, 0.54, 0.69),  # coast
	2:  Color(0.29, 0.54, 0.23),  # grassland
	3:  Color(0.16, 0.35, 0.16),  # forest
	4:  Color(0.54, 0.48, 0.35),  # hills
	5:  Color(0.42, 0.42, 0.48),  # mountain
	6:  Color(0.85, 0.82, 0.70),  # desert
	7:  Color(0.76, 0.70, 0.50),  # savanna
	8:  Color(0.20, 0.40, 0.22),  # swamp
	9:  Color(0.88, 0.92, 0.96),  # tundra
	10: Color(0.45, 0.30, 0.20),  # volcanic
	11: Color(0.50, 0.38, 0.55),  # corrupted
	12: Color(0.28, 0.18, 0.28),  # shadow_forest
	13: Color(0.30, 0.25, 0.40),  # dark_hills
	14: Color(0.22, 0.22, 0.30),  # dark_mountain
	15: Color(0.15, 0.12, 0.25),  # abyss
	16: Color(0.60, 0.50, 0.70),  # crystal_waste
	17: Color(0.05, 0.80, 0.65),  # mana_spring
	18: Color(0.55, 0.45, 0.65),  # enchanted_forest
	19: Color(0.70, 0.65, 0.80),  # arcane_ruins
}

var _terrain_textures: Dictionary = {}  # "plane_terrain" -> Texture2D
var _plane_names: Array[String] = ["aurelith", "umbraxis", "aethermere"]
var _terrain_names: Array[String] = [
	"ocean", "coast", "grassland", "forest", "hills",
	"mountain", "desert", "savanna", "swamp", "tundra",
	"volcanic", "corrupted", "shadow_forest", "dark_hills",
	"dark_mountain", "abyss", "crystal_waste", "mana_spring",
	"enchanted_forest", "arcane_ruins",
]

func _ready() -> void:
	_load_terrain_textures()

func _load_terrain_textures() -> void:
	for plane_name in _plane_names:
		for terrain_name in _terrain_names:
			var path := "res://assets/terrain/%s/%s.png" % [plane_name, terrain_name]
			if ResourceLoader.exists(path):
				var tex: Texture2D = load(path)
				_terrain_textures["%s_%s" % [plane_name, terrain_name]] = tex

func set_plane(plane_index: int) -> void:
	_plane = plane_index
	# Force full rebuild on plane change
	_clear_all()
	_prev_rect = Rect2i()

func _process(_delta: float) -> void:
	var cam: Camera2D = get_parent().get_node_or_null("WorldCamera")
	if cam == null:
		return
	var rect: Rect2i = cam.get_visible_tile_rect()
	if rect == _prev_rect:
		return
	_update_visible_tiles(rect)
	_prev_rect = rect

func _update_visible_tiles(rect: Rect2i) -> void:
	var map_w: int = WorldMap.get_width()
	var map_h: int = WorldMap.get_height()
	
	# Determine which tiles should be visible
	var needed: Dictionary = {}
	for ty in range(rect.position.y, rect.position.y + rect.size.y):
		if ty < 0 or ty >= map_h:
			continue
		for tx in range(rect.position.x, rect.position.x + rect.size.x):
			var wrapped_x := tx % map_w
			if wrapped_x < 0:
				wrapped_x += map_w
			# Use raw tx for drawing position (keeps scrolling smooth)
			needed[Vector2i(tx, ty)] = wrapped_x
	
	# Remove sprites that are no longer needed
	var to_remove: Array[Vector2i] = []
	for key in _tile_sprites:
		if not needed.has(key):
			to_remove.append(key)
	for key in to_remove:
		_recycle_sprite(_tile_sprites[key])
		_tile_sprites.erase(key)
	
	# Add sprites for newly visible tiles
	for key in needed:
		if _tile_sprites.has(key):
			continue
		var wrapped_x: int = needed[key]
		var terrain: int = WorldMap.get_terrain(_plane, Vector2i(wrapped_x, key.y))
		var sprite := _get_sprite()
		sprite.texture = _get_terrain_texture(terrain)
		sprite.position = Vector2(key.x * TILE_SIZE + TILE_SIZE / 2, key.y * TILE_SIZE + TILE_SIZE / 2)
		sprite.visible = true
		_tile_sprites[key] = sprite

func _get_terrain_texture(terrain: int) -> Texture2D:
	# Try to use a real PNG first
	if _plane >= 0 and _plane < _plane_names.size():
		var plane_name: String = _plane_names[_plane]
		if terrain >= 0 and terrain < _terrain_names.size():
			var key := "%s_%s" % [plane_name, _terrain_names[terrain]]
			if _terrain_textures.has(key):
				return _terrain_textures[key]
	
	# Fallback: procedural solid-colour tile
	return _make_color_texture(terrain)

var _color_texture_cache: Dictionary = {}

func _make_color_texture(terrain: int) -> Texture2D:
	if _color_texture_cache.has(terrain):
		return _color_texture_cache[terrain]
	
	var color: Color = _terrain_colors.get(terrain, Color.MAGENTA)
	var img := Image.create(TILE_SIZE, TILE_SIZE, false, Image.FORMAT_RGBA8)
	img.fill(color)
	# Draw a subtle border to distinguish tiles
	var border_color := color.darkened(0.25)
	for i in range(TILE_SIZE):
		img.set_pixel(i, 0, border_color)
		img.set_pixel(i, TILE_SIZE - 1, border_color)
		img.set_pixel(0, i, border_color)
		img.set_pixel(TILE_SIZE - 1, i, border_color)
	
	var tex := ImageTexture.create_from_image(img)
	_color_texture_cache[terrain] = tex
	return tex

# ── Sprite pool ──────────────────────────────────────────────────────────────

func _get_sprite() -> Sprite2D:
	if _sprite_pool.size() > 0:
		return _sprite_pool.pop_back()
	var s := Sprite2D.new()
	s.z_index = 0
	add_child(s)
	return s

func _recycle_sprite(s: Sprite2D) -> void:
	s.visible = false
	_sprite_pool.append(s)

func _clear_all() -> void:
	for key in _tile_sprites:
		_recycle_sprite(_tile_sprites[key])
	_tile_sprites.clear()
