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

	/** Reputation score limits. */
	static constexpr int32 REPUTATION_MAX = 1000;
	static constexpr int32 REPUTATION_MIN = -1000;

	/** Per-turn reputation decay toward zero. */
	static constexpr int32 REPUTATION_DECAY_PER_TURN = 1;

	/** Reputation bonus when a treaty is accepted. */
	static constexpr int32 TREATY_ACCEPT_REP_BONUS = 50;

	/** Reputation penalty when a treaty is rejected. */
	static constexpr int32 TREATY_REJECT_REP_PENALTY = -10;

	/** Reputation penalty for declaring war. */
	static constexpr int32 WAR_DECLARATION_REP_PENALTY = -200;

	/** Extra bilateral penalty for unprovoked war (no casus belli). */
	static constexpr int32 UNPROVOKED_WAR_EXTRA_PENALTY = -100;

	/** Global reputation penalty applied to all observers for unprovoked war. */
	static constexpr int32 UNPROVOKED_WAR_GLOBAL_PENALTY = -50;

	/** Global reputation penalty when a treaty is broken. */
	static constexpr int32 TREATY_BROKEN_GLOBAL_PENALTY = -30;

	/** Number of turns before an unanswered treaty proposal expires. */
	static constexpr int32 PROPOSAL_EXPIRY_TURNS = 5;

	/** Maximum grievance entries retained per diplomatic relation. */
	static constexpr int32 GRIEVANCE_HISTORY_LIMIT = 20;

	/** Passive reputation gain per turn from active trade agreements. */
	static constexpr int32 TRADE_PASSIVE_REP_PER_TURN = 2;

	/** Reputation bonus applied when a vassalage relationship is established. */
	static constexpr int32 VASSALAGE_REP_BONUS = 200;

	/** Reputation penalty when a vassal rebels. */
	static constexpr int32 VASSALAGE_REBELLION_PENALTY = -100;

	/** Multiplier for computing gift reputation value from resource value. */
	static constexpr int32 GIFT_RESOURCE_MULTIPLIER = 10;

	/** Diminishing-returns factor applied to successive gifts. */
	static constexpr float GIFT_DIMINISHING_FACTOR = 0.2f;

	/** Maximum reputation gain from a single gift. */
	static constexpr int32 GIFT_MAX_REP_PER_GIFT = 100;

	/** Reputation threshold below which casus belli is granted automatically. */
	static constexpr int32 DEEP_ENMITY_THRESHOLD = -300;

	/** Extra penalty per repeat offense (stacking). */
	static constexpr int32 REPEAT_OFFENDER_PENALTY = 25;

	// ─────────────────────────────────────────────────────────────────────────
	// Espionage
	// ─────────────────────────────────────────────────────────────────────────

	/** Base detection chance (percent) for a spy mission. */
	static constexpr int32 BASE_DETECTION_CHANCE = 30;

	/** Detection bonus per counter-spy agent assigned. */
	static constexpr int32 COUNTERSPY_DETECTION_BONUS = 8;

	/** Minimum success chance for any spy mission (percent). */
	static constexpr int32 MIN_SUCCESS_CHANCE = 5;

	/** Maximum success chance for any spy mission (percent). */
	static constexpr int32 MAX_SUCCESS_CHANCE = 95;

	/** Experience points required per spy level. */
	static constexpr int32 XP_PER_LEVEL = 50;

	/** Maximum mission history entries before trimming. */
	static constexpr int32 MISSION_HISTORY_LIMIT = 200;

	/** Number of entries removed during a trim batch. */
	static constexpr int32 MISSION_HISTORY_TRIM_BATCH = 50;

	/** Skill penalty applied to a ransomed agent. */
	static constexpr int32 RANSOM_SKILL_PENALTY = 10;

	/** Base success chance for turning an enemy agent (percent). */
	static constexpr int32 TURN_AGENT_SUCCESS_CHANCE = 30;

	/** Default security level for a wizard with no counter-intel budget. */
	static constexpr int32 BASE_SECURITY_LEVEL = 20;

	// ─────────────────────────────────────────────────────────────────────────
	// Magic
	// ─────────────────────────────────────────────────────────────────────────

	/** Base mana income per turn (before nodes/books). */
	static constexpr int32 BASE_MANA_PER_TURN = 5;

	/** Mana income bonus per controlled mana node. */
	static constexpr int32 MANA_PER_NODE = 5;

	/** Mana maintenance cost per active enchantment per turn. */
	static constexpr int32 ENCHANTMENT_MAINTENANCE_COST = 5;

	/** Mana cost to inscribe a rune on an item. */
	static constexpr int32 RUNE_INSCRIPTION_COST = 20;

	/** Percentage of mana refunded when cancelling a spell in progress. */
	static constexpr int32 SPELL_CANCEL_REFUND_PCT = 50;

	/** Placeholder research cost for spells without data-asset overrides. */
	static constexpr int32 PLACEHOLDER_RESEARCH_COST = 100;

	/** Placeholder ritual mana cost for rituals without data-asset overrides. */
	static constexpr int32 PLACEHOLDER_RITUAL_COST = 500;

	/** Casting skill contribution multiplier for spell power calculations. */
	static constexpr float CASTING_SKILL_MULTIPLIER = 0.022f;

	/** Bonus mana income per spell book owned. */
	static constexpr int32 SPELL_BOOK_MANA_BONUS = 1;

	/** Base spell power before skill/book modifiers. */
	static constexpr int32 BASE_SPELL_POWER = 10;

	/** Random range added to dispel power rolls. */
	static constexpr int32 DISPEL_RAND_RANGE = 50;

	// ─────────────────────────────────────────────────────────────────────────
	// Siege
	// ─────────────────────────────────────────────────────────────────────────

	/** Default food reserves for a city when a siege begins. */
	static constexpr int32 DEFAULT_FOOD_RESERVES = 50;

	/** Default morale for a city garrison at siege start. */
	static constexpr int32 DEFAULT_MORALE = 100;

	/** Default wall hit points at siege start. */
	static constexpr int32 DEFAULT_WALL_HP = 100;

	/** Normal starvation rate (food consumed per turn). */
	static constexpr int32 STARVATION_RATE_NORMAL = 1;

	/** Starvation rate when supply lines are cut. */
	static constexpr int32 STARVATION_RATE_CUT = 3;

	/** Turns into siege before morale begins to decay. */
	static constexpr int32 MORALE_DECAY_THRESHOLD_TURNS = 5;

	/** Morale points lost per turn once decay threshold is reached. */
	static constexpr int32 MORALE_DECAY_RATE = 2;

	// ─────────────────────────────────────────────────────────────────────────
	// Naval
	// ─────────────────────────────────────────────────────────────────────────

	/** Turns before a slain sea monster respawns. */
	static constexpr int32 SEA_MONSTER_RESPAWN_DELAY = 20;

	/** Combat power contributed per ship in a fleet. */
	static constexpr int32 FLEET_COMBAT_POWER_PER_SHIP = 10;

	// ─────────────────────────────────────────────────────────────────────────
	// Tile size for map rendering (used by CoMRendering, not CoMCore)
	// ─────────────────────────────────────────────────────────────────────────

	/** World-space size of a single map tile in Unreal units (cm). */
	constexpr float TILE_WORLD_SIZE_CM = 200.0f;

// City
static constexpr int32 MIN_CITY_DISTANCE = 3;

} // namespace CoM
