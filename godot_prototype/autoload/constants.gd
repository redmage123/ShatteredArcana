extends Node
## Port of CoMConstants.h — all game-wide constants for Shattered Arcana.

# ---------------------------------------------------------------------------
# Map dimensions
# ---------------------------------------------------------------------------
const MAP_WIDTH: int = 60
const MAP_HEIGHT: int = 40
const NUM_PLANES: int = 8
const TILE_SIZE: int = 64  # pixels per tile for rendering

# ---------------------------------------------------------------------------
# Plane indices
# ---------------------------------------------------------------------------
const PLANE_AURELITH: int = 0
const PLANE_NOCTHARION: int = 1
const PLANE_VERDANIA: int = 2
const PLANE_PYRATHEON: int = 3
const PLANE_GLACIUM: int = 4
const PLANE_AETHERMYST: int = 5
const PLANE_SHADOWDEEP: int = 6
const PLANE_CELESTARA: int = 7

# ---------------------------------------------------------------------------
# Terrain types
# ---------------------------------------------------------------------------
const TERRAIN_OCEAN: int = 0
const TERRAIN_GRASSLAND: int = 1
const TERRAIN_FOREST: int = 2
const TERRAIN_HILLS: int = 3
const TERRAIN_MOUNTAIN: int = 4
const TERRAIN_DESERT: int = 5
const TERRAIN_SWAMP: int = 6
const TERRAIN_TUNDRA: int = 7
const TERRAIN_SHADOW: int = 8
const TERRAIN_CORRUPTED: int = 9
const TERRAIN_VOLCANIC: int = 10
const TERRAIN_CRYSTAL: int = 11

# ---------------------------------------------------------------------------
# Movement costs per terrain
# ---------------------------------------------------------------------------
const MOVE_COST: Dictionary = {
	TERRAIN_OCEAN: 99,
	TERRAIN_GRASSLAND: 1,
	TERRAIN_FOREST: 2,
	TERRAIN_HILLS: 2,
	TERRAIN_MOUNTAIN: 4,
	TERRAIN_DESERT: 2,
	TERRAIN_SWAMP: 3,
	TERRAIN_TUNDRA: 2,
	TERRAIN_SHADOW: 2,
	TERRAIN_CORRUPTED: 3,
	TERRAIN_VOLCANIC: 3,
	TERRAIN_CRYSTAL: 1,
}

# ---------------------------------------------------------------------------
# Army / unit limits
# ---------------------------------------------------------------------------
const MAX_UNITS_PER_ARMY: int = 9
const MAX_ARMIES_PER_WIZARD: int = 20
const MAX_COMBAT_ROUNDS: int = 50
const MAX_LEVEL: int = 10
const XP_PER_LEVEL: int = 100

# ---------------------------------------------------------------------------
# City constants
# ---------------------------------------------------------------------------
const MAX_CITY_POPULATION: int = 25
const BASE_FOOD_REQUIREMENT: int = 1
const BASE_GOLD_PER_POP: int = 2
const BASE_PRODUCTION_PER_POP: int = 1
const MAX_BUILD_QUEUE: int = 5

# ---------------------------------------------------------------------------
# Magic constants
# ---------------------------------------------------------------------------
const BASE_CASTING_SKILL: int = 10
const MANA_PER_NODE: int = 5
const MAX_SPELL_BOOKS: int = 13  # total across all realms

# ---------------------------------------------------------------------------
# Magic realms
# ---------------------------------------------------------------------------
const REALM_ARCANE: String = "arcane"
const REALM_NATURE: String = "nature"
const REALM_SHADOW: String = "shadow"
const REALM_FIRE: String = "fire"
const REALM_ICE: String = "ice"

# ---------------------------------------------------------------------------
# Diplomacy
# ---------------------------------------------------------------------------
const RELATION_MIN: int = -100
const RELATION_MAX: int = 100
const RELATION_WAR_THRESHOLD: int = -50
const RELATION_ALLIANCE_THRESHOLD: int = 50

# ---------------------------------------------------------------------------
# Fog of war
# ---------------------------------------------------------------------------
const DEFAULT_SIGHT_RANGE: int = 2
const SCOUT_SIGHT_RANGE: int = 4
const CITY_SIGHT_RANGE: int = 3

# ---------------------------------------------------------------------------
# Victory conditions
# ---------------------------------------------------------------------------
const VICTORY_CONQUEST: String = "conquest"
const VICTORY_SPELL: String = "spell_of_mastery"
const VICTORY_SCORE: String = "score"
const SCORE_TURN_LIMIT: int = 300
