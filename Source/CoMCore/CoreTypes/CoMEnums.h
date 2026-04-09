// Copyright Mythforge Studios. All Rights Reserved.
// CoMEnums.h — All UENUM declarations for Shattered Arcana
// Sourced from game-design/master-plan.md
// Platform-agnostic: no OS/platform-specific headers

#pragma once
#include "CoreMinimal.h"
#include "CoMEnums.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STRUCTURE
// ─────────────────────────────────────────────────────────────────────────────

/** The eight planes of existence.
 *  NOTE (2026-04-04): Infernal plane merged into Infernyx.
 *  Infernyx now contains both volcanic/elemental terrain AND devil/iron-city terrain.
 *  Infernyx has two magic schools: Chaos (primary) + Binding/Pact (secondary).
 *  NOTE (2026-04-04): Feywild added as Plane 8. Magic school: Glamour. */
UENUM(BlueprintType)
enum class ECoMPlane : uint8
{
	// ── Original Five ────────────────────────────────────────────────────────
	Aurelith    UMETA(DisplayName = "Aurelith"),    // Golden high-fantasy realm
	Noctharion  UMETA(DisplayName = "Noctharion"),  // Dark arcane shadow realm
	Verdantis   UMETA(DisplayName = "Verdantis"),   // Primal nature realm
	Infernyx    UMETA(DisplayName = "Infernyx"),    // Fire/iron dual realm — volcanoes + iron cities; devils, demons, fire elementals
	Aethermist  UMETA(DisplayName = "Aethermist"),  // Celestial spirit realm

	// ── New Planes ───────────────────────────────────────────────────────────
	Abyssal     UMETA(DisplayName = "Abyssal"),     // Demon realm — chaotic, destructive
	Ethereal    UMETA(DisplayName = "Ethereal"),    // Spirit realm — alien, unknowable
	Feywild     UMETA(DisplayName = "Feywild"),     // Fey realm — ever-shifting, glamour magic, two courts

	MAX UMETA(Hidden)
};

/** Map layers available per plane. */
UENUM(BlueprintType)
enum class ECoMMapLayer : uint8
{
	Surface    UMETA(DisplayName = "Surface"),    // Main overworld (160x100)
	Underdark  UMETA(DisplayName = "Underdark"),  // Subterranean layer (160x100)
	Underwater UMETA(DisplayName = "Underwater"), // Beneath ocean tiles (zone network)

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// TERRAIN
// ─────────────────────────────────────────────────────────────────────────────

/** All terrain types across all planes and layers. */
UENUM(BlueprintType)
enum class ECoMTerrain : uint8
{
	// ── Aurelith Surface ─────────────────────────────────────────────────────
	Grassland, Forest, Hills, Mountains, Desert, Tundra, Swamp, Ocean,
	River, Shore, Volcano, Plains, Savanna, Badlands, Jungle,
	EnchantedForest, CrystalLake, AncientHighway,

	// ── Noctharion Surface ──────────────────────────────────────────────────────
	ShadowForest, ObsidianPlains, CrystalDesert, DarkMountains, VoidOcean,
	CorruptedSwamp, AshWastes, TwilightHills, GloomTundra,
	ShadowRift, FloatingObsidian, ManaGeyser,

	// ── Verdantis Surface ───────────────────────────────────────────────────
	MegaJungle, FungalForest, LivingSwamp, RootMountains, PollenPlains,
	SporeDesert, AmberCoast, VineHills, BioluminescentDeep,
	WorldTree, SentientTerrain, MyceliumNetwork,

	// ── Infernyx Surface ────────────────────────────────────────────────────
	LavaFields, ObsidianSpires, AshDesert, SulfurSeas, MagmaRivers,
	CinderHills, ScorchedForest, EmberPlains, BasaltMountains,
	FireGeyser, LavaTube_Surface, VolcanicChain,

	// ── Aethermist Surface ──────────────────────────────────────────────────
	CloudPlains, CrystalSpires, VoidRifts, FloatingIslands, MistOcean,
	DreamMeadows, ResonancePeaks, EtherMarshes, StarlightTundra,
	GravityAnomaly, PhaseShiftingTerrain, AuroraField,

	// ── Aurelith Underdark — "The Deep Halls" ────────────────────────────────
	CavernFloor, FungalGrove, UndergroundLake, CrystalCavern_Arc,
	DwarvenRuins, CollapsedTunnel, LavaTube_Arc, DeepChasm,
	MineralVein, UndergroundRiver,

	// ── Noctharion Underdark — "The Shadow Deep" ────────────────────────────────
	ShadowCavern, VoidPocket, BoneOssuary, ObsidianLabyrinth,
	SoulWell, CorruptedCrystal, WebbedTunnels, EchoChamber,
	DarkRiver, NightmareRealm,

	// ── Verdantis Underdark — "The Root World" ──────────────────────────────
	RootTunnel, MyceliumNet_Verd, UndergroundGarden, AmberPool,
	InsectHive, PetrifiedForest, MudCavern, CocoonChamber,
	SapRiver, SeedVault,

	// ── Infernyx Underdark — "The Molten Depths" ────────────────────────────
	MagmaChamber, ObsidianCavern, SulfurPit, ForgeCavern,
	CinderTunnel, LavaRiver_Inf, DemonPit, CrystalForge,
	EmberShelf, SmokeVent,

	// ── Aethermist Underdark — "The Inverse" ────────────────────────────────
	GravityWell, PhaseCavern, CrystalResonance, VoidBubble,
	DreamGrotto, TemporalEddy, MirrorCavern, StarlightPool,
	ThoughtCorridor, ParadoxChamber,

	// ── Abyssal Surface — "The Screaming Wastes" ─────────────────────────────
	BonePlains, BloodRivers, ScreamingChasms, LivingWalls, DemonPillars,
	FleshCitadels, VoidFissures, ChaosStormfield, AbyssalMires,
	ChasmEdge, CarrionDesert, GoreMarshes,

	// ── Abyssal Underdark — "The Endless Chasms" ─────────────────────────────
	ChasmFloor, BoneLabyrinth, DemonNest, FleshCorridor,
	BloodPool_Aby, ChaosCore, VoidPocket_Aby, SoulMeatCavern,

	// ── Infernyx Iron Quarter (merged from Infernal, 2026-04-04) ─────────────
	// Devil/iron-city terrain now coexists with volcanic terrain on Infernyx.
	// Internal faction conflict: lawful devils vs chaotic fire elementals/demons.
	IronFortresses, SoulForges, FireLakes, BrimstoneHighways,
	ContractSpires, HellgateMarkets, TormentFields,

	// ── Infernyx Iron Depths (merged Infernal Underdark) ─────────────────────
	SoulForgeCavern, ContractVault, FireLakeCavern, HellmarketCave,

	// ── Ethereal Surface — "The Veil Between" ────────────────────────────────
	MistSeas, CrystallizedMemories, EchoRuins, ThoughtStorms,
	PhaseShores, DreamDeserts, SpiritArchipelagos, ReflectionMirrors,
	VoidWhisperFields, MemoryLandscape, FadeZones,

	// ── Ethereal Underdark — "The Memory Deep" ───────────────────────────────
	MemoryPool, EchoTunnel, PhaseLabyrinth, DreamChamber,
	ThoughtCorridor_Eth, SpiritWell, MistCavern, ForgottenRealm,

	// ── Feywild Surface — "The Ever-Shifting Realm" ───────────────────────────
	EternalForest, ShiftingGlades, MushroomRings, MirrorLakes,
	FeyMarketCrossroads, ThorncraftHollow, AncientStandingStones, BlossomPeaks,
	SilverMistPlains, TwilightMeadows, WildHuntGrounds, FeyWastes,
	SeelieCourt, UnseelieHollow, ErlingPath,

	// ── Feywild Underdark — "The Roots of Wonder" ────────────────────────────
	FeyRootLair, FeyUndermarket, MushroomCave_Fey, CrystalSpring_Fey,
	AncientBurrow, CobwebHalls, GlowwormDen, FeyVault,

	// ── Underwater ──────────────────────────────────────────────────────────
	OceanFloor, CoralReef, KelpForest, DeepTrench,
	UnderwaterVolcano, SunkenRuins, CrystalGrotto_UW,
	AbyssalPlain, Seamount, ThermalVent,

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// WIZARD CLASSES (added 2026-04-04)
// ─────────────────────────────────────────────────────────────────────────────

/** Wizard class — chosen at game start, shapes the entire magic system for that playthrough.
 *  Any wizard from any plane can take any class.
 *  Wizard = standard mana/research/retorts.
 *  Psyker = willpower resource, innate psychic abilities, no spellbook, no summoning.
 *  Warlock = reduced mana + Pact Debt meter, entity bargaining system, soul economy. */
UENUM(BlueprintType)
enum class ECoMWizardClass : uint8
{
	Wizard   UMETA(DisplayName = "Wizard"),         // Mana + spellbooks + research + retorts
	Psyker   UMETA(DisplayName = "Psyker"),         // Willpower + innate psychic abilities, no spellbook
	Warlock  UMETA(DisplayName = "Warlock/Witch"),  // Pact Debt + entity bargaining + soul harvest

	None     UMETA(DisplayName = "None"),
	MAX UMETA(Hidden)
};

/** Psyker ability identifiers. Psykers have a fixed ability set — no research, no spellbook. */
UENUM(BlueprintType)
enum class ECoMPsykerAbility : uint8
{
	Telepathy          UMETA(DisplayName = "Telepathy"),           // Passive: see first enemy command before resolution
	Telekinesis        UMETA(DisplayName = "Telekinesis"),         // Move army without movement cost (1×/turn)
	MindControl        UMETA(DisplayName = "Mind Control"),        // Permanently dominate enemy unit
	PsychicBlast       UMETA(DisplayName = "Psychic Blast"),       // Combat: 6d6 AoE, ignores physical armor
	Precognition       UMETA(DisplayName = "Precognition"),        // See next 3 world events + turn numbers
	MemoryManipulation UMETA(DisplayName = "Memory Manipulation"), // Alter diplomatic memory (erase grudge / plant casus belli)
	PsychicShield      UMETA(DisplayName = "Psychic Shield"),      // Passive: +25% mind resistance, charm immunity
	MassSuggestion     UMETA(DisplayName = "Mass Suggestion"),     // City transfers control for 5 turns

	MAX UMETA(Hidden)
};

/** Warlock pact grantor types. Each grants different spell tiers and term categories. */
UENUM(BlueprintType)
enum class ECoMPactGrantor : uint8
{
	DemonLord     UMETA(DisplayName = "Demon Lord"),     // Abyssal: Chaos tier 5-8
	Archdevil     UMETA(DisplayName = "Archdevil"),      // Infernyx: Binding tier 5-8
	FeyCourtLord  UMETA(DisplayName = "Fey Court Lord"), // Feywild: Glamour tier 4-8 (Seelie or Unseelie)
	SpiritElder   UMETA(DisplayName = "Spirit Elder"),   // Ethereal: Spirit tier 5-8
	AncientDragon UMETA(DisplayName = "Ancient Dragon"), // Any plane: Draconic tier 4-7
	VoidEntity    UMETA(DisplayName = "Void Entity"),    // Aethermist: Sorcery+Void tier 6-9

	MAX UMETA(Hidden)
};

/** Warlock pact term categories. Mixed when generating pact terms at deal-making time. */
UENUM(BlueprintType)
enum class ECoMPactTermType : uint8
{
	Sacrifice    UMETA(DisplayName = "Sacrifice"),    // Periodically sacrifice units/cities
	Restriction  UMETA(DisplayName = "Restriction"),  // Cannot perform certain actions
	Tribute      UMETA(DisplayName = "Tribute"),      // Periodic gold/soul fragment payment
	Behavioral   UMETA(DisplayName = "Behavioral"),   // Must/must-not behave in specific ways

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// MAGIC / SPELL SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

/** The nine spell schools / magic realms.
 *  NOTE: 8 planes, but 9 spell schools — Infernyx has two (Chaos + Binding). */
UENUM(BlueprintType)
enum class ECoMSpellRealm : uint8
{
	// ── Original Six ─────────────────────────────────────────────────────────
	Life    UMETA(DisplayName = "Life"),     // Healing, holy, paladins — Aurelith bonus
	Death   UMETA(DisplayName = "Death"),    // Undead, necrotic, shadow — Noctharion bonus
	Chaos   UMETA(DisplayName = "Chaos"),    // Destruction, fire, mutation — Infernyx/Abyssal bonus
	Nature  UMETA(DisplayName = "Nature"),   // Summoning, terrain, growth — Verdantis bonus
	Sorcery UMETA(DisplayName = "Sorcery"),  // Illusions, enchantment, planar — Aethermist bonus
	Arcane  UMETA(DisplayName = "Arcane"),   // Universal / ritual / rune spells (no plane bonus)

	// ── New Schools (Three-Plane Expansion) ──────────────────────────────────
	Binding UMETA(DisplayName = "Binding"),  // Soul contracts, domination, compulsion — Infernyx second school (alongside Chaos)
	Spirit  UMETA(DisplayName = "Spirit"),   // Ghosts, dreams, illusion, thought — Ethereal bonus

	// ── Feywild School ───────────────────────────────────────────────────────
	Glamour UMETA(DisplayName = "Glamour"),  // Illusions that are real, true naming, time manipulation — Feywild bonus

	None UMETA(DisplayName = "None"),        // Used where no alignment applies

	MAX UMETA(Hidden)
};

/** Distinguishes plane-native spell schools from universal ones.
 *  Universal schools: Life, Death, Nature, Arcane — no plane bonus/penalty.
 *  PlaneNative schools: Chaos, Sorcery, Binding, Spirit, Glamour
 *  — cheaper on home plane, expensive cross-plane (see FCoMPlaneSpellData). */
UENUM(BlueprintType)
enum class ECoMSpellSchoolType : uint8
{
	Universal   UMETA(DisplayName = "Universal"),    // Life, Death, Nature, Arcane — no plane restriction
	PlaneNative UMETA(DisplayName = "Plane-Native"), // Chaos, Sorcery, Binding, Spirit, Glamour

	MAX UMETA(Hidden)
};

/** Spell rarity tiers (affects research cost and power). */
UENUM(BlueprintType)
enum class ECoMSpellRarity : uint8
{
	Common    UMETA(DisplayName = "Common"),
	Uncommon  UMETA(DisplayName = "Uncommon"),
	Rare      UMETA(DisplayName = "Rare"),
	VeryRare  UMETA(DisplayName = "Very Rare"),

	MAX UMETA(Hidden)
};

/** Spell/ability cast scope. */
UENUM(BlueprintType)
enum class ECoMSpellScope : uint8
{
	Global    UMETA(DisplayName = "Global"),     // Cast from wizard fortress, any range
	Overworld UMETA(DisplayName = "Overworld"),  // Targets map tile or city
	Combat    UMETA(DisplayName = "Combat"),      // Cast during tactical battle
	Ritual    UMETA(DisplayName = "Ritual"),      // Multi-caster cooperative
	Enchantment UMETA(DisplayName = "Enchantment"), // Persistent effect on object

	MAX UMETA(Hidden)
};

/** Spell targeting type — used by the spell targeting widget to determine input mode. */
UENUM(BlueprintType)
enum class ECoMSpellTargetType : uint8
{
	NoTarget    UMETA(DisplayName = "No Target"),     // Instant cast (global spells, self-buffs)
	TileTarget  UMETA(DisplayName = "Tile Target"),   // Click a map tile
	UnitTarget  UMETA(DisplayName = "Unit Target"),   // Click a unit
	CityTarget  UMETA(DisplayName = "City Target"),   // Click a city
	ArmyTarget  UMETA(DisplayName = "Army Target"),   // Click an army group

	MAX UMETA(Hidden)
};

/** Ritual spell types. */
UENUM(BlueprintType)
enum class ECoMRitualType : uint8
{
	Summoning    UMETA(DisplayName = "Summoning"),
	Enchantment  UMETA(DisplayName = "Enchantment"),
	Divination   UMETA(DisplayName = "Divination"),
	Transformation UMETA(DisplayName = "Transformation"),
	Warding      UMETA(DisplayName = "Warding"),
	Cataclysm    UMETA(DisplayName = "Cataclysm"),
	Planar       UMETA(DisplayName = "Planar"),
	Binding      UMETA(DisplayName = "Binding"),

	MAX UMETA(Hidden)
};

/** Objects that can have runes inscribed. */
UENUM(BlueprintType)
enum class ECoMRuneTarget : uint8
{
	Tile      UMETA(DisplayName = "Tile"),      // Trap, ward, buff zone on map tile
	Unit      UMETA(DisplayName = "Unit"),      // Permanent buff on a unit
	Building  UMETA(DisplayName = "Building"),  // Enhanced building effect
	Item      UMETA(DisplayName = "Item"),      // Enchantment on crafted item
	Mine      UMETA(DisplayName = "Mine"),      // Enhanced mine output
	CityWall  UMETA(DisplayName = "City Wall"), // Defensive wall rune
	Gate      UMETA(DisplayName = "Gate"),      // Rune on planar gate or portal

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// WEATHER
// ─────────────────────────────────────────────────────────────────────────────

/** Dynamic weather types — each plane has its own unique types alongside shared ones. */
UENUM(BlueprintType)
enum class ECoMWeatherType : uint8
{
	Clear,
	Rain,
	HeavyRain,
	Thunderstorm,
	Snow,
	Blizzard,
	Fog,
	DenseFog,
	Sandstorm,
	Drought,
	Monsoon,

	// ── Infernyx-specific ────────────────────────────────────────────────────
	AshStorm         UMETA(DisplayName = "Ash Storm"),
	FireRain         UMETA(DisplayName = "Fire Rain"),
	VolcanicEruption UMETA(DisplayName = "Volcanic Eruption"),

	// ── Aethermist-specific ──────────────────────────────────────────────────
	ManaStorm     UMETA(DisplayName = "Mana Storm"),
	EtherealWind  UMETA(DisplayName = "Ethereal Wind"),
	VoidRift      UMETA(DisplayName = "Void Rift"),

	// ── Verdantis-specific ───────────────────────────────────────────────────
	SporeCloud   UMETA(DisplayName = "Spore Cloud"),
	LivingStorm  UMETA(DisplayName = "Living Storm"),

	// ── Noctharion-specific ──────────────────────────────────────────────────────
	ShadowVeil   UMETA(DisplayName = "Shadow Veil"),
	CrystalRain  UMETA(DisplayName = "Crystal Rain"),

	// ── Abyssal-specific ─────────────────────────────────────────────────────
	ChaosStorm   UMETA(DisplayName = "Chaos Storm"),   // Wild magic surges + terrain damage
	BloodRain    UMETA(DisplayName = "Blood Rain"),     // Visibility -3, corruption spread
	FleshGale    UMETA(DisplayName = "Flesh Gale"),     // Unit morale damage

	// ── Infernyx Iron Quarter-specific ───────────────────────────────────────
	AshFall_Inf  UMETA(DisplayName = "Ash Fall"),       // Movement -1 for non-fire-immune
	SoulSmog     UMETA(DisplayName = "Soul Smog"),       // Binding magic +25%
	HellGust     UMETA(DisplayName = "Hell Gust"),       // Ranged attacks -2

	// ── Ethereal-specific ────────────────────────────────────────────────────
	ThoughtStorm UMETA(DisplayName = "Thought Storm"),   // Reveals enemy plans/positions
	MistRoll     UMETA(DisplayName = "Mist Roll"),       // Visibility 0, phase movement free
	EchoSquall   UMETA(DisplayName = "Echo Squall"),     // Spirit magic +25%, all other schools -10%

	// ── Feywild-specific ─────────────────────────────────────────────────────
	FeyMist      UMETA(DisplayName = "Fey Mist"),        // Glamour +25%, visibility -2, terrain counts as magical
	WildHunt     UMETA(DisplayName = "Wild Hunt"),        // Units drawn toward Hunt Lord; combat chance each turn
	BlossomGale  UMETA(DisplayName = "Blossom Gale"),    // All units +2 HP regeneration/turn, movement free
	TrueNamingStorm UMETA(DisplayName = "True Naming Storm"), // All named units revealed on full map

	None UMETA(Hidden),
	MAX  UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// SEASONS
// ─────────────────────────────────────────────────────────────────────────────

/** Standard four-season cycle (Aurelith). Each plane maps its unique cycle to these slots. */
UENUM(BlueprintType)
enum class ECoMSeason : uint8
{
	/** Spring / Bloom / Simmer / Convergence depending on plane. */
	Season1 UMETA(DisplayName = "Season 1 (Spring/Bloom/Simmer/Convergence)"),

	/** Summer / Zenith / Eruption / Divergence. */
	Season2 UMETA(DisplayName = "Season 2 (Summer/Zenith/Eruption/Divergence)"),

	/** Autumn / Decay / Cooling / Resonance. */
	Season3 UMETA(DisplayName = "Season 3 (Autumn/Decay/Cooling/Resonance)"),

	/** Winter / Dormancy / Dormant / Null. */
	Season4 UMETA(DisplayName = "Season 4 (Winter/Dormancy/Dormant/Null)"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// PORTALS
// ─────────────────────────────────────────────────────────────────────────────

/** Every type of inter-location connection in the game. */
UENUM(BlueprintType)
enum class ECoMPortalType : uint8
{
	// ── Intra-plane surface portals ──────────────────────────────────────────
	NaturalGateway  UMETA(DisplayName = "Natural Gateway"),   // Always active
	ArcaneWaypoint  UMETA(DisplayName = "Arcane Waypoint"),   // Needs activation
	FeyCircle       UMETA(DisplayName = "Fey Circle"),        // Verdantis: seasonal
	LavaTube        UMETA(DisplayName = "Lava Tube"),         // Infernyx volcanic
	MistGate        UMETA(DisplayName = "Mist Gate"),         // Aethermist shifting
	ShadowPath      UMETA(DisplayName = "Shadow Path"),       // Noctharion: night only
	ChasmTear       UMETA(DisplayName = "Chasm Tear"),        // Abyssal: demonic rift, violent
	ContractGate    UMETA(DisplayName = "Contract Gate"),     // Infernyx (devil quarter): pact-sealed, restricted
	MistVeil        UMETA(DisplayName = "Mist Veil"),         // Ethereal: shifting mist portal
	FeySanctuary    UMETA(DisplayName = "Fey Sanctuary Gate"), // Feywild: glamour-shrouded, only visible to true-seers or Warlocks with Fey pact

	// ── Surface <-> Underdark ────────────────────────────────────────────────
	CaveEntrance    UMETA(DisplayName = "Cave Entrance"),
	Sinkhole        UMETA(DisplayName = "Sinkhole"),
	DeepMine        UMETA(DisplayName = "Deep Mine Entrance"),
	AncientStairway UMETA(DisplayName = "Ancient Stairway"),
	RootTunnelPortal UMETA(DisplayName = "Root Tunnel"),      // Verdantis
	LavaTunnelPortal UMETA(DisplayName = "Lava Tunnel"),      // Infernyx
	VoidWell        UMETA(DisplayName = "Void Well"),         // Aethermist
	ShadowGate      UMETA(DisplayName = "Shadow Gate"),       // Noctharion
	AbyssalMaw      UMETA(DisplayName = "Abyssal Maw"),       // Abyssal underdark entrance
	IronHatch       UMETA(DisplayName = "Iron Hatch"),        // Infernyx (devil quarter): locked by contract
	EchoPool_Portal UMETA(DisplayName = "Echo Pool"),         // Ethereal: memory-reflection pool
	RootSinkholePortal UMETA(DisplayName = "Root Sinkhole"),  // Feywild: root-lined burrow to Roots of Wonder

	// ── Inter-plane ──────────────────────────────────────────────────────────
	TowerOfWizardry UMETA(DisplayName = "Tower of Wizardry"),
	PlanarRift      UMETA(DisplayName = "Planar Rift"),       // Spell-created

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// RESOURCES
// ─────────────────────────────────────────────────────────────────────────────

/** All mineable / harvestable resource types across all planes and layers. */
UENUM(BlueprintType)
enum class ECoMResource : uint8
{
	// ── Universal (all planes) ───────────────────────────────────────────────
	Iron, Coal, GoldOre, Gems, Silver, Horses, Nightshade,

	// ── Aurelith-specific ─────────────────────────────────────────────────────
	Mithril, Quartz,

	// ── Noctharion-specific ──────────────────────────────────────────────────────
	Adamantium, Orichalcon, ShadowQuartz,

	// ── Verdantis-specific ───────────────────────────────────────────────────
	Lifewood, AmberEssence, Moonstone, LivingCrystal,

	// ── Infernyx-specific ────────────────────────────────────────────────────
	Fireglass, Brimstone, MagmaCore, ChaosOre,

	// ── Aethermist-specific ──────────────────────────────────────────────────
	Aetherium, SpiritGlass, Voidstone, Dreamweave,

	// ── Abyssal-specific ─────────────────────────────────────────────────────
	DemonBlood, ChaosCrystal_Aby, BoneShards, VoidEssence_Aby,

	// ── Infernyx Iron Quarter-specific (merged from Infernal) ────────────────
	SoulSteel, HellgoldAlloy, PactStone, AshenIron,

	// ── Ethereal-specific ────────────────────────────────────────────────────
	MemoryCrystal, SpiritDust, PhaseGlass, DreamThread,

	// ── Feywild-specific ─────────────────────────────────────────────────────
	FeyWood, MoonbloomPetal, FaeDust, TimewornAmber,

	// ── Underdark-specific (all planes) ──────────────────────────────────────
	Deepstone, Glowmoss, ShadowOre, EmberCrystal, PhaseMetal,
	Darkwood, Brimite, Thoughtstone, Bloodite, AncientAlloy,

	// ── Underwater-specific ───────────────────────────────────────────────────
	Pearls, DeepCoral, AbyssalIron, PressureCrystals,

	None UMETA(Hidden),
	MAX  UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// PLANAR CORRUPTION
// ─────────────────────────────────────────────────────────────────────────────

/** Types of corruption/purification that can spread across tiles. */
UENUM(BlueprintType)
enum class ECoMCorruptionType : uint8
{
	None                UMETA(DisplayName = "None"),                  // No corruption — default for uncorrupted tiles
	InfernyxCorruption  UMETA(DisplayName = "Infernyx Corruption"),  // Lava/ash/soulfire spread, fire dmg; devil-taint variant converts cities to pact-bound (tribute to nearest Archdevil)
	ShadowCorruption    UMETA(DisplayName = "Shadow Corruption"),    // Terrain darkens, undead spawn
	WildGrowth          UMETA(DisplayName = "Wild Growth"),          // Jungle spreads, entangle
	EtherealBleed       UMETA(DisplayName = "Ethereal Bleed"),       // Phase terrain, random effects
	HolyPurification    UMETA(DisplayName = "Holy Purification"),    // Life cleanses to grassland
	ChaosMutation       UMETA(DisplayName = "Chaos Mutation"),       // Terrain warps randomly
	VoidErosion         UMETA(DisplayName = "Void Erosion"),         // Anti-magic spreads

	// ── New-Plane Corruption Types ────────────────────────────────────────────
	AbyssalCorruption   UMETA(DisplayName = "Abyssal Corruption"),   // Spreads fastest, hardest to remove, spawns chaos monsters
	// NOTE (2026-04-04): InfernalCorruption removed — pact-city mechanic folded into InfernyxCorruption above
	EtherealCorruption  UMETA(DisplayName = "Ethereal Corruption"),  // Terrain becomes phase-unstable; all non-Spirit units move at -1
	FeyCorruption       UMETA(DisplayName = "Fey Corruption"),       // Terrain becomes glamour-enchanted; movement costs halved but Willpower/Sanity drains each turn

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// WORLD EVENTS
// ─────────────────────────────────────────────────────────────────────────────

/** All major world event categories that can be triggered. */
UENUM(BlueprintType)
enum class ECoMWorldEventType : uint8
{
	// ── Celestial ────────────────────────────────────────────────────────────
	SolarEclipse, LunarAlignment, CometAppearance, StarFall,

	// ── Planar ───────────────────────────────────────────────────────────────
	PlanarConvergence, PlanarBleeding, PlanarSealWeakening, AetherTide,

	// ── Dragon ───────────────────────────────────────────────────────────────
	DragonMatingFlight, DragonConvocation, DragonAwakening,

	// ── Nature ───────────────────────────────────────────────────────────────
	GlobalBloom, GreatDrought, Plague, MassExtinction,

	// ── Chaos ────────────────────────────────────────────────────────────────
	MagicStorm, VolcanicAge, DimensionalRift, RealityFracture,

	// ── Death ────────────────────────────────────────────────────────────────
	UndeathRising, SoulHarvest, AncientLichAwakens,

	// ── Social ───────────────────────────────────────────────────────────────
	OrganizationWar, MerchantCrisis, HeroicAge, ProphecyFulfilled,

	// ── Underdark ────────────────────────────────────────────────────────────
	GreatUpheaval, DeepQuake, UnderdarkFlood,

	// ── Abyssal-specific ─────────────────────────────────────────────────────
	ChaosStorm UMETA(DisplayName = "Chaos Storm"),  // Abyssal: random devastation, corruption spread

	None UMETA(DisplayName = "None"),  // No event — default sentinel
	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// DRAGONS
// ─────────────────────────────────────────────────────────────────────────────

/** Dragon classification: Chromatic (aggressive), Metallic (noble), Planar (plane-native). */
UENUM(BlueprintType)
enum class ECoMDragonClassification : uint8
{
	Chromatic UMETA(DisplayName = "Chromatic"), // Chaos/Death aligned
	Metallic  UMETA(DisplayName = "Metallic"),  // Life/Sorcery aligned
	Planar    UMETA(DisplayName = "Planar"),    // Native to specific plane

	MAX UMETA(Hidden)
};

/** Dragon age stages — affects all stats multiplicatively. */
UENUM(BlueprintType)
enum class ECoMDragonAge : uint8
{
	Young   UMETA(DisplayName = "Young"),   // Turns 0-50,   stat ×1.0
	Adult   UMETA(DisplayName = "Adult"),   // Turns 51-150, stat ×1.5
	Ancient UMETA(DisplayName = "Ancient"), // Turns 151-300, stat ×2.0
	Elder   UMETA(DisplayName = "Elder"),   // Turns 300+,   stat ×2.5

	MAX UMETA(Hidden)
};

/** Dragon's current role in the world. */
UENUM(BlueprintType)
enum class ECoMDragonRole : uint8
{
	Wild         UMETA(DisplayName = "Wild"),          // Wanders, guards lair, attacks opportunistically
	Summon       UMETA(DisplayName = "Summoned"),      // Called by wizard spell/ritual
	DomainRuler  UMETA(DisplayName = "Domain Ruler"),  // Controls territory as minor AI power
	DungeonBoss  UMETA(DisplayName = "Dungeon Boss"),  // Guards explorable site
	Mercenary    UMETA(DisplayName = "Mercenary"),     // Hireable
	Companion    UMETA(DisplayName = "Companion"),     // Bonded to hero
	Enslaved     UMETA(DisplayName = "Enslaved"),      // Bound by ritual

	MAX UMETA(Hidden)
};

/** Dragon breath weapon type. */
UENUM(BlueprintType)
enum class ECoMBreathType : uint8
{
	Fire, Lightning, Poison, Acid, Cold, Psychic, Explosive, Sand,
	Death, Void, Prismatic, Ash, Nature, Radiant, Paralyzing, Repulsion,
	Sleep, Force, HellFire, Pollen, Sonic,
	Time   UMETA(DisplayName = "Time Breath"),  // Ages/rejuvenates targets
	None,

	MAX UMETA(Hidden)
};

/** Treaty types available between a Dragon Lord and a wizard. */
UENUM(BlueprintType)
enum class ECoMDragonTreatyType : uint8
{
	None,
	NonAggression  UMETA(DisplayName = "Non-Aggression"),
	Tribute        UMETA(DisplayName = "Tribute"),
	Alliance       UMETA(DisplayName = "Alliance"),
	Vassalage      UMETA(DisplayName = "Vassalage"),
	WarTarget      UMETA(DisplayName = "War Target"),
	CompanionBond  UMETA(DisplayName = "Companion Bond"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// UNITS & HEROES
// ─────────────────────────────────────────────────────────────────────────────

/** Unit movement capability categories. */
UENUM(BlueprintType)
enum class ECoMMovementType : uint8
{
	Walking    UMETA(DisplayName = "Walking"),
	Flying     UMETA(DisplayName = "Flying"),
	Swimming   UMETA(DisplayName = "Swimming"),
	Burrowing  UMETA(DisplayName = "Burrowing"),
	PhaseWalk  UMETA(DisplayName = "Phase Walk"),  // Pass through terrain
	Sailing    UMETA(DisplayName = "Sailing"),      // Ship-borne movement

	MAX UMETA(Hidden)
};

/** Attack type categories for damage resolution. */
UENUM(BlueprintType)
enum class ECoMAttackType : uint8
{
	Melee,
	Ranged,
	Magical,
	Gaze,
	Thrown,
	Breath,
	Psionic,
	Siege UMETA(DisplayName = "Siege Weapon"),

	MAX UMETA(Hidden)
};

/** Hero personality traits (primary and secondary). */
UENUM(BlueprintType)
enum class ECoMPersonalityTrait : uint8
{
	Brave, Cautious, Greedy, Loyal, Ambitious, Scholarly, Devout,
	Chaotic, Honorable, Treacherous, Compassionate, Ruthless,
	Curious, Protective,

	MAX UMETA(Hidden)
};

/** Hero/unit relationship types. */
UENUM(BlueprintType)
enum class ECoMHeroRelationType : uint8
{
	Neutral,
	Friendly,
	Rivalry        UMETA(DisplayName = "Rivalry"),
	Hatred         UMETA(DisplayName = "Hatred"),
	Romance        UMETA(DisplayName = "Romance"),
	MentorApprentice UMETA(DisplayName = "Mentor/Apprentice"),
	BloodOath      UMETA(DisplayName = "Blood Oath"),
	Nemesis        UMETA(DisplayName = "Nemesis"),
	DragonBond     UMETA(DisplayName = "Dragon Bond"),
	OrgLoyalty     UMETA(DisplayName = "Organization Loyalty"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// CITIES & BUILDINGS
// ─────────────────────────────────────────────────────────────────────────────

/** City tier — affects max buildings, production capacity. */
UENUM(BlueprintType)
enum class ECoMCitySize : uint8
{
	Hamlet    UMETA(DisplayName = "Hamlet"),    // Population 1-2
	Village   UMETA(DisplayName = "Village"),   // Population 3-4
	Town      UMETA(DisplayName = "Town"),      // Population 5-8
	City      UMETA(DisplayName = "City"),      // Population 9-14
	Capital   UMETA(DisplayName = "Capital"),   // Population 15-25

	MAX UMETA(Hidden)
};

/** City layer type — affects available buildings and bonuses. */
UENUM(BlueprintType)
enum class ECoMCityLayer : uint8
{
	Surface   UMETA(DisplayName = "Surface"),    // Standard surface city
	Underdark UMETA(DisplayName = "Underdark"),  // Subterranean — no farms, +mining
	Underwater UMETA(DisplayName = "Underwater"), // Aquatic — requires water breathing

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// TRADE
// ─────────────────────────────────────────────────────────────────────────────

/** All tradeable goods. */
UENUM(BlueprintType)
enum class ECoMTradeGood : uint8
{
	// ── Basic goods (produced by buildings) ──────────────────────────────────
	Grain, Lumber, Stone, Cloth, Pottery,

	// ── Refined goods ────────────────────────────────────────────────────────
	Steel, Wine, Jewelry, EnchantedCloth, AlchemicalReagents,

	// ── Luxury goods ─────────────────────────────────────────────────────────
	Spices, Silk, MagicalArtifacts, ExoticBeasts, AncientRelics,

	// ── Plane-specific trade goods ───────────────────────────────────────────
	ShadowEssence, LivingSap, MoltenGlass, AetherDust, DragonScale,

	None UMETA(Hidden),
	MAX  UMETA(Hidden)
};

/** Trade route status. */
UENUM(BlueprintType)
enum class ECoMTradeRouteStatus : uint8
{
	Active     UMETA(DisplayName = "Active"),
	Disrupted  UMETA(DisplayName = "Disrupted"), // Raided / warband
	Blockaded  UMETA(DisplayName = "Blockaded"), // Naval blockade

	MAX UMETA(Hidden)
};

/** Trade agreement type. */
UENUM(BlueprintType)
enum class ECoMTradeAgreementType : uint8
{
	Open      UMETA(DisplayName = "Open Trade"),
	Exclusive UMETA(DisplayName = "Exclusive"),
	Embargo   UMETA(DisplayName = "Embargo"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// DIPLOMACY
// ─────────────────────────────────────────────────────────────────────────────

/** All diplomatic treaty types available between wizards. */
UENUM(BlueprintType)
enum class ECoMTreatyType : uint8
{
	None,
	War                 UMETA(DisplayName = "War"),
	NonAggression       UMETA(DisplayName = "Non-Aggression Pact"),
	OpenBorders         UMETA(DisplayName = "Open Borders"),
	TradeAgreement      UMETA(DisplayName = "Trade Agreement"),
	DefensivePact       UMETA(DisplayName = "Defensive Pact"),
	MilitaryAlliance    UMETA(DisplayName = "Military Alliance"),
	WizardsPact         UMETA(DisplayName = "Wizard's Pact"),     // Shared spell research
	RuneExchange        UMETA(DisplayName = "Rune Exchange"),
	ResourcePact        UMETA(DisplayName = "Resource Pact"),
	VassalTreaty        UMETA(DisplayName = "Vassal Treaty"),
	Confederation       UMETA(DisplayName = "Confederation"),
	MarriageOfConvenience UMETA(DisplayName = "Marriage of Convenience"),

	MAX UMETA(Hidden)
};

/** All diplomatic actions a wizard can perform. */
UENUM(BlueprintType)
enum class ECoMDiplomacyAction : uint8
{
	ProposeTreaty, BreakTreaty, DeclareWar, OfferPeace,
	DemandTribute, RequestSpellTrade, RequestRuneTrade, OfferGift,
	ThreatenWar, RequestOpenBorders, ProposeJointWar,
	SponsorOrganization, IssueUltimatum, ProposeConfederation,
	OfferVassalage, RequestHeroExchange, EstablishEmbassy,
	ExpelAmbassador, NegotiateBorderAdjustment, ProposeResourceTrade,

	MAX UMETA(Hidden)
};

/** Wizard moral alignment (Life-leaning vs Chaos/Death-leaning). */
UENUM(BlueprintType)
enum class ECoMAlignment : uint8
{
	Virtuous   UMETA(DisplayName = "Virtuous"),   // Life / Nature dominated
	Neutral    UMETA(DisplayName = "Neutral"),
	Malevolent UMETA(DisplayName = "Malevolent"), // Death / Chaos dominated

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// NAVAL
// ─────────────────────────────────────────────────────────────────────────────

/** Fleet formation types. */
UENUM(BlueprintType)
enum class ECoMFleetFormation : uint8
{
	Line      UMETA(DisplayName = "Line"),
	Wedge     UMETA(DisplayName = "Wedge"),
	Defensive UMETA(DisplayName = "Defensive"),
	Raiding   UMETA(DisplayName = "Raiding"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// SIEGE
// ─────────────────────────────────────────────────────────────────────────────

/** Multi-turn siege phases. */
UENUM(BlueprintType)
enum class ECoMSiegePhase : uint8
{
	Approach      UMETA(DisplayName = "Approach"),
	Encirclement  UMETA(DisplayName = "Encirclement"),
	Bombardment   UMETA(DisplayName = "Bombardment"),
	Assault       UMETA(DisplayName = "Assault"),
	Breakthrough  UMETA(DisplayName = "Breakthrough"),
	Surrender     UMETA(DisplayName = "Surrender"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// EXPLORATION & DUNGEONS
// ─────────────────────────────────────────────────────────────────────────────

/** Explorable site types found on the overworld. */
UENUM(BlueprintType)
enum class ECoMSiteType : uint8
{
	AncientRuins, ForgottenTemple, BurialMound, TowerOfWizardry,
	ManaNode, DragonLair, DemonGate, FeyGlade, CrystalCavern,
	VolcanicForge, SunkenCity, FlyingCitadel, LibraryOfTheLost,
	ColiseumOfChampions, MerchantVault, OraclesSpire,

	MAX UMETA(Hidden)
};

/** Dungeon reward types. */
UENUM(BlueprintType)
enum class ECoMRewardType : uint8
{
	Gold, Mana, Spell, Item, RuneKnowledge, UnitRecruitment,
	ResourceCache, SpellBookPage, RetortFragment,
	OrganizationFavor, TerritoryMap,

	MAX UMETA(Hidden)
};

/** Artifact location type (where to find or unlock it). */
UENUM(BlueprintType)
enum class ECoMArtifactLocation : uint8
{
	InDungeon,
	InDragonHoard,
	CarriedByWarband,
	HeldByOrganization,
	FragmentedAcrossPlanes,
	HiddenBySpell,
	InUnderwaterRuin,

	MAX UMETA(Hidden)
};

/** Artifact quest step types. */
UENUM(BlueprintType)
enum class ECoMArtifactQuestType : uint8
{
	VisitLocation, DefeatEnemy, CastSpellAtLocation, GatherFragments,
	NegotiateWithDragon, JoinOrganization, CompleteRitual,
	ReachHeroLevel, DiscoverLore, ForgeAtLocation,

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// WARBANDS
// ─────────────────────────────────────────────────────────────────────────────

/** Named warband archetypes. */
UENUM(BlueprintType)
enum class ECoMWarbandType : uint8
{
	Bandits, BeastPack, UndeadHorde, ElementalSwarm,
	DemonicRaiders, SpiritHost, DragonFlight, MercenaryCompany,
	PirateFleet, CultistCell, FeyWarparty, GolemArmy,

	MAX UMETA(Hidden)
};

/** What a warband is currently doing. */
UENUM(BlueprintType)
enum class ECoMWarbandBehavior : uint8
{
	Patrol, Raid, Migrate, HuntPlayer, GuardSite, SeekArtifact, Conquer,

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// ORGANIZATIONS
// ─────────────────────────────────────────────────────────────────────────────

/** Independent organization / society types. */
UENUM(BlueprintType)
enum class ECoMOrgType : uint8
{
	MerchantGuild  UMETA(DisplayName = "Merchant Guild"),
	ThievesGuild   UMETA(DisplayName = "Thieves Guild"),
	MagesCircle    UMETA(DisplayName = "Mages Circle"),
	HolyOrder      UMETA(DisplayName = "Holy Order"),
	DarkCult       UMETA(DisplayName = "Dark Cult"),
	DruidicCircle  UMETA(DisplayName = "Druidic Circle"),
	ElementalLodge UMETA(DisplayName = "Elemental Lodge"),
	RunemastersOrder UMETA(DisplayName = "Runemasters Order"),
	MercenaryLeague UMETA(DisplayName = "Mercenary League"),
	ExplorersSociety UMETA(DisplayName = "Explorers Society"),
	PropheticOrder UMETA(DisplayName = "Prophetic Order"),
	PirateFleet    UMETA(DisplayName = "Pirate Fleet"),

	MAX UMETA(Hidden)
};

/** Agent mission types. */
UENUM(BlueprintType)
enum class ECoMAgentMission : uint8
{
	Spy, Sabotage, Assassinate, Steal, Recruit,
	TradeManipulation, GuardAsset, InfiltrateCity,
	CorruptOfficial, PropagandaCampaign,

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// MIGRATION & DISEASE
// ─────────────────────────────────────────────────────────────────────────────

/** Why a population is migrating. */
UENUM(BlueprintType)
enum class ECoMMigrationReason : uint8
{
	War, Famine, Plague, Corruption, Unrest,
	TaxOppression, ReligiousFlight, DragonTerror,
	Prosperity, Opportunity, Conquest, VolcanicDisaster, UnderdarkRaid,

	MAX UMETA(Hidden)
};

/** Disease types that can spread through cities. */
UENUM(BlueprintType)
enum class ECoMDiseaseType : uint8
{
	CommonPlague  UMETA(DisplayName = "Common Plague"),
	RedPox        UMETA(DisplayName = "Red Pox"),
	ShadowBlight  UMETA(DisplayName = "Shadow Blight"),
	SporeLung     UMETA(DisplayName = "Spore Lung"),
	FireFever     UMETA(DisplayName = "Fire Fever"),
	VoidSickness  UMETA(DisplayName = "Void Sickness"),
	UndeathPlague UMETA(DisplayName = "Undeath Plague"),
	CursedBlood   UMETA(DisplayName = "Cursed Blood"),

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// TURN SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

/** Major game phases in the per-turn state machine. */
UENUM(BlueprintType)
enum class ECoMGamePhase : uint8
{
	WorldProcessing  UMETA(DisplayName = "World Processing"),   // Start-of-turn subsystem ticks
	PlayerTurn       UMETA(DisplayName = "Player Turn"),        // Human wizard input
	AITurn           UMETA(DisplayName = "AI Turn"),            // AI wizard decisions
	CombatResolution UMETA(DisplayName = "Combat Resolution"),  // Queued battles resolved
	EndOfTurn        UMETA(DisplayName = "End of Turn"),        // Cleanup and next-turn prep

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// VICTORY CONDITIONS
// ─────────────────────────────────────────────────────────────────────────────

/**
 * How a wizard wins the game.
 * Evaluated each turn by UCoMTurnSubsystem::EvaluateVictoryConditions().
 * COM-056c: Pact and Dissolution added for Three Planes expansion.
 */
UENUM(BlueprintType)
enum class ECoMVictoryType : uint8
{
	None        UMETA(DisplayName = "None"),              // No win yet
	Conquest    UMETA(DisplayName = "Conquest"),          // Eliminate all other wizards (last man standing)
	Domination  UMETA(DisplayName = "Domination"),       // Control all capital cities
	Magical     UMETA(DisplayName = "Magical Supremacy"), // Research and cast the Spell of Mastery
	Diplomatic  UMETA(DisplayName = "Diplomatic Victory"), // Alliance with all surviving wizards (min 4)
	Economic    UMETA(DisplayName = "Economic Domination"), // Control >=70% of world gold income for 20 turns
	Cultural    UMETA(DisplayName = "Cultural Hegemony"), // All cities choose your culture voluntarily
	Pact        UMETA(DisplayName = "Pact Victory"),      // COM-056c: hold contracts with all 3 Archdevils simultaneously
	Dissolution UMETA(DisplayName = "Dissolution Victory"), // COM-056c: destabilize 3 planes + slay Sael-Thrix

	MAX UMETA(Hidden)
};

/**
 * Per-race victory condition affinity.
 * Drives which win conditions a race's city production and abilities favor.
 * Sourced from game-design/spells/three-planes-unit-rosters.md race entries.
 */
UENUM(BlueprintType)
enum class ECoMVictoryAffinity : uint8
{
	None      UMETA(DisplayName = "None"),
	Conquest  UMETA(DisplayName = "Conquest"),
	Economic  UMETA(DisplayName = "Economic"),
	Magic     UMETA(DisplayName = "Magic"),
	Cultural  UMETA(DisplayName = "Cultural"),

	MAX UMETA(Hidden)
};

/**
 * Classifies an Apex Ruler (Dragon Lord equivalent for the Three Planes).
 * Determines which AI behavior tree, diplomacy rules, and domain logic apply.
 */
UENUM(BlueprintType)
enum class ECoMApexRulerClass : uint8
{
	Dragon        UMETA(DisplayName = "Dragon Lord"),       // Original system
	DemonPrince   UMETA(DisplayName = "Demon Prince"),      // Abyssal rulers — chaotic, high betrayal risk
	Archdevil     UMETA(DisplayName = "Archdevil"),         // Infernal rulers — contract-bound, Pact Victory targets
	EtherealWarden UMETA(DisplayName = "Ethereal Warden"),  // Ethereal curators — alien logic

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// SAVE SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

/** Save slot states. */
UENUM(BlueprintType)
enum class ECoMSaveSlotState : uint8
{
	Empty,
	Occupied,
	Autosave,
	Quicksave,

	MAX UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
/** Unit role categories for unit spec DataAssets. */
UENUM(BlueprintType)
enum class ECoMUnitCategory : uint8
{
    Infantry    UMETA(DisplayName = "Infantry"),
    Cavalry     UMETA(DisplayName = "Cavalry"),
    Ranged      UMETA(DisplayName = "Ranged"),
    Siege       UMETA(DisplayName = "Siege"),
    Naval       UMETA(DisplayName = "Naval"),
    Flying      UMETA(DisplayName = "Flying"),
    Hero        UMETA(DisplayName = "Hero"),
    Magical     UMETA(DisplayName = "Magical"),
    Dragon      UMETA(DisplayName = "Dragon"),
    Warband     UMETA(DisplayName = "Warband"),
    MAX         UMETA(Hidden)
};

/** Equipment slot categories for item template DataAssets. */
UENUM(BlueprintType)
enum class ECoMItemSlot : uint8
{
    Weapon      UMETA(DisplayName = "Weapon"),
    Offhand     UMETA(DisplayName = "Offhand / Shield"),
    Armor       UMETA(DisplayName = "Armor"),
    Helm        UMETA(DisplayName = "Helm"),
    Boots       UMETA(DisplayName = "Boots"),
    Ring        UMETA(DisplayName = "Ring"),
    Amulet      UMETA(DisplayName = "Amulet"),
    Relic       UMETA(DisplayName = "Relic"),
    Artifact    UMETA(DisplayName = "Artifact"),
    None        UMETA(DisplayName = "Unequippable"),
    MAX         UMETA(Hidden)
};

/** Ship classification for naval unit DataAssets. */
UENUM(BlueprintType)
enum class ECoMShipType : uint8
{
    Galley          UMETA(DisplayName = "Galley"),           // Light fast ship
    Warship         UMETA(DisplayName = "Warship"),          // Heavy combat vessel
    Transport       UMETA(DisplayName = "Transport"),        // Troop carrier
    Trireme         UMETA(DisplayName = "Trireme"),          // Ramming ship
    MagicVessel     UMETA(DisplayName = "Magic Vessel"),     // Enchanted hull
    Flagship        UMETA(DisplayName = "Flagship"),         // Hero/wizard ship
    DragonShip      UMETA(DisplayName = "Dragon Ship"),      // Dragon-mounted naval
    None            UMETA(DisplayName = "None"),

    MAX             UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────────────────────
// ARCHITECTURE STYLES
// ─────────────────────────────────────────────────────────────────────────────

/** Visual architectural style for city rendering.
 *  Determines which city icon textures and building art variants to use.
 *  Each race maps to one of these styles via CoMBuildingDatabase::GetArchitectureForRace(). */
UENUM(BlueprintType)
enum class ECoMArchitectureStyle : uint8
{
    Human        UMETA(DisplayName = "Human"),        // Stone castles, thatched roofs, timber frames — High Men, Nomads, Halflings, Barbarians
    Elven        UMETA(DisplayName = "Elven"),        // Organic curves, living wood, crystal spires — High Elves, Wood Elves
    Dwarven      UMETA(DisplayName = "Dwarven"),      // Stone halls, geometric, underground vaults — Dwarves, Gnomes, Deep Dwarves
    Orcish       UMETA(DisplayName = "Orcish"),       // Crude wood/bone, palisades, war tents — Orcs, Gnolls, Beastmen, Trolls
    Dark         UMETA(DisplayName = "Dark"),         // Gothic spires, obsidian, shadow-wreathed — Dark Elves, Drow, Shades, Undead
    Draconic     UMETA(DisplayName = "Draconic"),     // Volcanic stone, dragon motifs, lava channels — Draconians
    Demonic      UMETA(DisplayName = "Demonic"),      // Iron fortresses, fire, chains — Demons, Demonkin, Hellbound
    Aquatic      UMETA(DisplayName = "Aquatic"),      // Coral, shell, flowing water — Merfolk, Deep Ones, Lizardmen
    Fey          UMETA(DisplayName = "Fey"),          // Mushroom caps, crystal springs, living flowers — Sprites, Pixies, Satyrs, Dryads
    Ethereal     UMETA(DisplayName = "Ethereal"),     // Phase-shifted, translucent, echo architecture — Wraiths, Phantoms, Mistborn
    Abyssal      UMETA(DisplayName = "Abyssal"),      // Bone, flesh, organic growths — Maleficii, Gorethians, Plagueborn
    Crystalline  UMETA(DisplayName = "Crystalline"),  // Crystal formations, prismatic — Crystallines, Sylphs, Voidwalkers

    MAX UMETA(Hidden)
};
