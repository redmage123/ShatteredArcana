extends Node2D
## Draws fog of war overlay for tiles not visible to the current wizard.

const TILE_SIZE := 128

var _fog_hidden_color = Color(0, 0, 0, 0.85)
var _fog_explored_color = Color(0, 0, 0, 0.5)

func refresh() -> void:
	queue_redraw()

func _draw() -> void:
	if not is_instance_valid(get_parent()):
		return
	
	var cam: Camera2D = get_parent().get_node_or_null("WorldCamera")
	if cam == null:
		return
	
	var rect: Rect2i = cam.get_visible_tile_rect()
	var wiz_id: int = GameState.current_wizard
	var plane: int = get_parent().current_plane
	var map_w: int = Constants.MAP_WIDTH
	var map_h: int = Constants.MAP_HEIGHT
	
	for ty in range(rect.position.y, rect.position.y + rect.size.y):
		if ty < 0 or ty >= map_h:
			continue
		for tx in range(rect.position.x, rect.position.x + rect.size.x):
			# Wrap X for cylindrical world
			var wrapped_x = tx % map_w
			if wrapped_x < 0:
				wrapped_x += map_w
			
			var tile_pos = Vector2i(wrapped_x, ty)
			var draw_pos = Vector2(tx * TILE_SIZE, ty * TILE_SIZE)
			
			if FogOfWar.is_visible(wiz_id, plane, tile_pos):
				continue  # fully visible — no overlay
			elif FogOfWar.is_explored(wiz_id, plane, tile_pos):
				draw_rect(Rect2(draw_pos, Vector2(TILE_SIZE, TILE_SIZE)), _fog_explored_color)
			else:
				draw_rect(Rect2(draw_pos, Vector2(TILE_SIZE, TILE_SIZE)), _fog_hidden_color)
