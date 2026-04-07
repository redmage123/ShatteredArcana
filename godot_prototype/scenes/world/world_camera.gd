extends Camera2D
## Handles WASD panning, mouse-wheel zoom, and edge scrolling.

const TILE_SIZE := 128

@export var camera_speed: float = 800.0
@export var zoom_speed: float = 0.1
@export var min_zoom: float = 0.2
@export var max_zoom: float = 2.0
@export var edge_scroll_margin: int = 20
@export var edge_scroll_speed: float = 600.0

var _dragging := false
var _drag_start := Vector2.ZERO

func _process(delta: float) -> void:
	var move := Vector2.ZERO
	
	# WASD / arrow keys
	if Input.is_action_pressed("ui_left"):
		move.x -= 1
	if Input.is_action_pressed("ui_right"):
		move.x += 1
	if Input.is_action_pressed("ui_up"):
		move.y -= 1
	if Input.is_action_pressed("ui_down"):
		move.y += 1
	
	# Edge scrolling
	var vp_size := get_viewport_rect().size
	var mouse := get_viewport().get_mouse_position()
	if mouse.x < edge_scroll_margin:
		move.x -= 1
	elif mouse.x > vp_size.x - edge_scroll_margin:
		move.x += 1
	if mouse.y < edge_scroll_margin:
		move.y -= 1
	elif mouse.y > vp_size.y - edge_scroll_margin:
		move.y += 1
	
	if move != Vector2.ZERO:
		position += move.normalized() * camera_speed * delta / zoom.x
	
	# Clamp Y so camera doesn't go too far off the map
	var map_height: int = WorldMap.get_height() if WorldMap != null else 100
	var max_y := map_height * TILE_SIZE
	position.y = clampf(position.y, -200, max_y + 200)
	
	# Wrap X for cylindrical world
	var map_width: int = WorldMap.get_width() if WorldMap != null else 160
	var world_width := map_width * TILE_SIZE
	if world_width > 0:
		if position.x < 0:
			position.x += world_width
		elif position.x > world_width:
			position.x -= world_width

func _unhandled_input(event: InputEvent) -> void:
	# Mouse wheel zoom
	if event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				_zoom_at_point(zoom_speed, get_global_mouse_position())
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				_zoom_at_point(-zoom_speed, get_global_mouse_position())
	
	# Middle-mouse drag
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_MIDDLE:
		if event.pressed:
			_dragging = true
			_drag_start = event.position
		else:
			_dragging = false
	
	if event is InputEventMouseMotion and _dragging:
		position -= event.relative / zoom.x

func _zoom_at_point(factor: float, target: Vector2) -> void:
	var old_zoom := zoom
	var new_val := clampf(zoom.x + factor, min_zoom, max_zoom)
	zoom = Vector2(new_val, new_val)
	# Adjust position so zoom centers on mouse
	position += (target - position) * (1.0 - old_zoom.x / zoom.x)

## Convert current mouse position to tile coordinates.
func get_tile_at_mouse() -> Vector2i:
	var world_pos := get_global_mouse_position()
	return Vector2i(int(floor(world_pos.x / TILE_SIZE)), int(floor(world_pos.y / TILE_SIZE)))

## Return the visible rectangle in tile coordinates (with buffer).
func get_visible_tile_rect() -> Rect2i:
	var vp_size := get_viewport_rect().size
	var half_vp := vp_size / (2.0 * zoom)
	var tl := (position - half_vp) / TILE_SIZE
	var br := (position + half_vp) / TILE_SIZE
	var buffer := 2
	var x0 := int(floor(tl.x)) - buffer
	var y0 := int(floor(tl.y)) - buffer
	var x1 := int(ceil(br.x)) + buffer
	var y1 := int(ceil(br.y)) + buffer
	return Rect2i(x0, y0, x1 - x0, y1 - y0)
