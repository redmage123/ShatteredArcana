// Copyright Mythforge Studios. All Rights Reserved.
// CoMConstants.h — Global compile-time constants for Shattered Arcana
// Sourced from game-design/master-plan.md
// Platform-agnostic: no OS/platform-specific headers

#pragma once
#include "CoreMinimal.h"

namespace CoM
{
	// ─────────────────────────────────────────────────────────────────────────
	// Map Dimensions
	// ─────────────────────────────────────────────────────────────────────────

	/** Per-layer map width (surface & Underdark). Original MoM was 60; we use 160. */
	constexpr int32 MAP_WIDTH  = 160;

	/** Per-layer map height (surface & Underdark). Original MoM was 40; we use 100. */
	constexpr int32 MAP_HEIGHT = 100;

	/** Total tiles per map layer (surface or Underdark). */
	constexpr int32 MAP_TILES_PER_LAYER = MAP_WIDTH * MAP_HEIGHT; // 16,000

	/** Number of planes (Aurelith, Noctharion, Verdantis, Infernyx, Aethermist, Abyssal, Ethereal, Feywild).
	 *  NOTE (2026-04-04): Infernal merged into Infernyx — plane count was temporarily 7.
	 *  NOTE (2026-04-04): Feywild added as Plane 8. Magic school: Glamour.
	 *  Infernyx has 2 magic schools (Magma + Binding); all other planes have 1. */
	constexpr int32 NUM_PLANES = 8;

	/** Named layers per plane: Surface + Underdark = 2 full-tile layers.
	 *  Underwater is a zone network beneath ocean tiles, not a full grid. */
	constexpr int32 FULL_LAYERS_PER_PLANE = 2;

	/** Total full-tile map layers across all planes. */
	constexpr int32 TOTAL_MAP_LAYERS = NUM_PLANES * FULL_LAYERS_PER_PLANE; // 16 (8 planes × 2)

	/** Total full-tile tiles across all layers. */
	constexpr int32 TOTAL_MAP_TILES = TOTAL_MAP_LAYERS * MAP_TILES_PER_LAYER; // 256,000

	/** Map wraps horizontally (east/west). Does NOT wrap vertically. */
	constexpr bool MAP_WRAP_X = true;

	// ─────────────────────────────────────────────────────────────────────────
	// Army & Unit Limits
	// ─────────────────────────────────────────────────────────────────────────

	/** Maximum units in a single army stack (faithful to CoM/MoM). */
	constexpr int32 MAX_ARMY_SIZE = 9;

	/** Maximum heroes a single wizard may have in service. */
	constexpr int32 MAX_HEROES_PER_WIZARD = 6;

	/** Maximum items a hero may carry. */
	constexpr int32 MAX_HERO_ITEMS = 3;

	/** Maximum rune slots on a weapon item. */
	constexpr int32 MAX_ITEM_RUNE_SLOTS_WEAPON = 4;

	/** Maximum rune slots on an armor item. */
	constexpr int32 MAX_ITEM_RUNE_SLOTS_ARMOR = 3;

	/** Maximum rune slots on an accessory item. */
	constexpr int32 MAX_ITEM_RUNE_SLOTS_ACCESSORY = 2;

	/** Maximum enchantments on a unit (non-hero). */
	constexpr int32 MAX_UNIT_ENCHANTMENTS = 4;

	/** Maximum enchantments on a hero unit. */
	constexpr int32 MAX_HERO_ENCHANTMENTS = 6;

	// ─────────────────────────────────────────────────────────────────────────
	// Wizard & Game Structure
	// ─────────────────────────────────────────────────────────────────────────

	/** Maximum number of wizards (human + AI) in a single game. */
	constexpr int32 MAX_WIZARDS = 14;

	/** Wizard index used to represent "no wizard / unowned". */
	constexpr int32 WIZARD_INDEX_NONE = -1;

	/** Maximum spell books a wizard may allocate. */
	constexpr int32 MAX_SPELL_BOOKS = 11;

	/** Maximum retorts a wizard may select at game start. */
	constexpr int32 MAX_RETORTS = 3;

	/** Number of spell schools available. */
	constexpr int32 NUM_SPELL_REALMS = 9; // Life, Death, Chaos, Nature, Sorcery, Arcane, Binding, Spirit, Glamour

	// ─────────────────────────────────────────────────────────────────────────
	// Fog of War
	// ─────────────────────────────────────────────────────────────────────────

	/** Each tile stores two uint32 bitmasks — one bit per wizard (up to 32).
	 *  FogRevealed = ever seen; CurrentVision = currently visible. */
	constexpr int32 FOG_BITMASK_COUNT = 2;

	// ─────────────────────────────────────────────────────────────────────────
	// Turn & Season
	// ─────────────────────────────────────────────────────────────────────────

	/** Turns per season (default). 4 seasons × 12 turns = 48-turn year. */
	constexpr int32 DEFAULT_TURNS_PER_SEASON = 12;

	/** Number of seasons in a cycle (all planes have a 4-step cycle). */
	constexpr int32 NUM_SEASONS = 4;

	// ─────────────────────────────────────────────────────────────────────────
	// Cities
	// ─────────────────────────────────────────────────────────────────────────

	/** Maximum population size (in 1,000s) a city can reach without specific buildings. */
	constexpr int32 MAX_CITY_POPULATION = 25;

	/** Maximum global city enchantments a single city may have active. */
	constexpr int32 MAX_CITY_ENCHANTMENTS = 6;

	/** Maximum wall HP (base, before rune/enchantment bonuses). */
	constexpr int32 MAX_WALL_HP_BASE = 100;

	// ─────────────────────────────────────────────────────────────────────────
	// Ley Lines
	// ─────────────────────────────────────────────────────────────────────────

	/** Minimum ley lines generated per plane. */
	constexpr int32 LEY_LINES_PER_PLANE_MIN = 15;

	/** Maximum ley lines generated per plane. */
	constexpr int32 LEY_LINES_PER_PLANE_MAX = 25;

	/** Minimum ley line intersections per plane. */
	constexpr int32 LEY_INTERSECTIONS_PER_PLANE_MIN = 5;

	/** Maximum ley line intersections per plane. */
	constexpr int32 LEY_INTERSECTIONS_PER_PLANE_MAX = 10;

	// ─────────────────────────────────────────────────────────────────────────
	// Portals
	// ─────────────────────────────────────────────────────────────────────────

	/** Natural surface portals per plane (min). */
	constexpr int32 SURFACE_PORTALS_PER_PLANE_MIN = 3;

	/** Natural surface portals per plane (max). */
	constexpr int32 SURFACE_PORTALS_PER_PLANE_MAX = 6;

	/** Underdark entrances per plane (min). */
	constexpr int32 UNDERDARK_ENTRANCES_PER_PLANE_MIN = 8;

	/** Underdark entrances per plane (max). */
	constexpr int32 UNDERDARK_ENTRANCES_PER_PLANE_MAX = 15;

	// ─────────────────────────────────────────────────────────────────────────
	// Dragons
	// ─────────────────────────────────────────────────────────────────────────

	/** Dragon Lords per plane surface (min). */
	constexpr int32 DRAGON_LORDS_PER_PLANE_SURFACE_MIN = 2;

	/** Dragon Lords per plane surface (max). */
	constexpr int32 DRAGON_LORDS_PER_PLANE_SURFACE_MAX = 4;

	/** Dragon Lords per Underdark (min). */
	constexpr int32 DRAGON_LORDS_PER_UNDERDARK_MIN = 1;

	/** Dragon Lords per Underdark (max). */
	constexpr int32 DRAGON_LORDS_PER_UNDERDARK_MAX = 2;

	// ─────────────────────────────────────────────────────────────────────────
	// World Events
	// ─────────────────────────────────────────────────────────────────────────

	/** Average turns between major world events (min). */
	constexpr int32 WORLD_EVENT_INTERVAL_MIN = 30;

	/** Average turns between major world events (max). */
	constexpr int32 WORLD_EVENT_INTERVAL_MAX = 50;

	// ─────────────────────────────────────────────────────────────────────────
	// Diplomacy
	// ─────────────────────────────────────────────────────────────────────────

	/** Relation score range. */
	constexpr int32 RELATION_SCORE_MIN  = -200;
	constexpr int32 RELATION_SCORE_MAX  =  200;

	// ─────────────────────────────────────────────────────────────────────────
	// Tile size for map rendering (used by CoMRendering, not CoMCore)
	// ─────────────────────────────────────────────────────────────────────────

	/** World-space size of a single map tile in Unreal units (cm). */
	constexpr float TILE_WORLD_SIZE_CM = 200.0f;

} // namespace CoM
