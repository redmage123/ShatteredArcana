# create_terrain_weight_assets.py
# Run via: UnrealEditor ShatteredArcana.uproject -ExecutePythonScript="Scripts/create_terrain_weight_assets.py"
#
# Creates UCoMTerrainWeightDataAsset instances for all 8 planes in
# Content/Data/WorldGen/TerrainWeights/ with weights that match the hardcoded
# palettes from Stage2_TerrainDistribution (GetSurfaceLandTerrain / GetUnderdarkTerrain).
#
# Each entry's MinAltitude/MaxAltitude corresponds to the height thresholds used in the
# original switch-case (height is in [0,999]; we normalise to [0,1]).
# Latitude ranges use the same normalisation ([0,100] -> [0,1]).
#
# After running this script the Asset Manager can load assets by primary type "CoMTerrainWeight".

import unreal

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

ASSET_CLASS = unreal.CoMTerrainWeightDataAsset
PACKAGE_PATH = "/Game/Data/WorldGen/TerrainWeights"

def make_entry(terrain_name, weight,
               min_lat=0.0, max_lat=1.0,
               min_alt=0.0, max_alt=1.0):
    """Create an FCoMTerrainWeightEntry struct."""
    e = unreal.CoMTerrainWeightEntry()
    e.set_editor_property("terrain", unreal.ECoMTerrain[terrain_name])
    e.set_editor_property("weight", float(weight))
    e.set_editor_property("min_latitude",  float(min_lat))
    e.set_editor_property("max_latitude",  float(max_lat))
    e.set_editor_property("min_altitude",  float(min_alt))
    e.set_editor_property("max_altitude",  float(max_alt))
    return e

def make_ud_entry(terrain_name, weight):
    """Underdark entries have no lat/alt constraint — they always match."""
    return make_entry(terrain_name, weight, 0.0, 1.0, 0.0, 1.0)

def create_asset(plane_enum_name, plane_enum_value, surface_entries, underdark_entries):
    asset_name = "TW_" + plane_enum_name
    full_path   = PACKAGE_PATH + "/" + asset_name

    # Create or load existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    existing    = unreal.load_asset(full_path)
    if existing:
        asset = existing
        print(f"[create_terrain_weight_assets] Updating existing asset: {full_path}")
    else:
        asset = asset_tools.create_asset(
            asset_name, PACKAGE_PATH,
            unreal.CoMTerrainWeightDataAsset,
            unreal.AssetToolsHelpers.get_asset_tools().create_asset_with_dialog
            if False else unreal.CoMTerrainWeightDataAsset.static_class()
        )
        if not asset:
            # Fallback: use factory
            factory = unreal.DataAssetFactory()
            factory.set_editor_property("supported_class", ASSET_CLASS.static_class())
            asset = asset_tools.create_asset(asset_name, PACKAGE_PATH, None, factory)
        print(f"[create_terrain_weight_assets] Created new asset: {full_path}")

    if not asset:
        print(f"[create_terrain_weight_assets] ERROR: failed to create {full_path}")
        return

    # Set plane enum
    asset.set_editor_property("plane", unreal.ECoMPlane[plane_enum_name])
    asset.set_editor_property("surface_weights",    surface_entries)
    asset.set_editor_property("underdark_weights",  underdark_entries)
    asset.set_editor_property("underwater_weights", [])  # reserved for future sprint

    # Mark dirty and save
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    print(f"[create_terrain_weight_assets] Saved: {full_path}")

# ---------------------------------------------------------------------------
# Per-plane weight tables (mirror of GetSurfaceLandTerrain hardcoded logic)
# Alt thresholds (normalised, [0,1] = [0,999]):
#   >0.820 -> mountain  >0.710 -> hills   <0.530..0.560 -> swamp/wetland
#   lat>0.70..0.80 -> tundra/polar    lat<0.12..0.20 -> tropical/desert
# ---------------------------------------------------------------------------

PLANE_DEFS = [
    # ── Aurelith ─────────────────────────────────────────────────────────────
    ("Aurelith", [
        make_entry("Mountains", 1.0,  min_alt=0.821),
        make_entry("Hills",     1.0,  min_alt=0.711, max_alt=0.820),
        make_entry("Tundra",    1.0,  min_lat=0.80),
        make_entry("Jungle",    1.0,  max_lat=0.12,  max_alt=0.640),
        make_entry("Desert",    1.0,  max_lat=0.20,  max_alt=0.600),
        make_entry("Swamp",     1.0,  max_alt=0.555),
        make_entry("Forest",    1.0,  min_alt=0.660, max_alt=0.820),
        make_entry("Plains",    1.0,  max_alt=0.580),
        make_entry("Grassland", 2.0),  # default catch-all gets higher weight
    ], [
        make_ud_entry("CavernFloor", 1.0),
    ]),

    # ── Noctharion ───────────────────────────────────────────────────────────
    ("Noctharion", [
        make_entry("DarkMountains",  1.0, min_alt=0.821),
        make_entry("TwilightHills",  1.0, min_alt=0.701, max_alt=0.820),
        make_entry("GloomTundra",    1.0, min_lat=0.70),
        make_entry("CorruptedSwamp", 1.0, max_alt=0.560),
        make_entry("ShadowForest",   1.0, min_alt=0.640, max_alt=0.820),
        make_entry("CrystalDesert",  1.0, max_lat=0.20),
        make_entry("ObsidianPlains", 2.0),
    ], [
        make_ud_entry("ShadowCavern", 1.0),
    ]),

    # ── Verdantis ────────────────────────────────────────────────────────────
    ("Verdantis", [
        make_entry("RootMountains", 1.0, min_alt=0.821),
        make_entry("VineHills",     1.0, min_alt=0.701, max_alt=0.820),
        make_entry("LivingSwamp",   1.0, max_alt=0.530),
        make_entry("SporeDesert",   1.0, max_lat=0.15),
        make_entry("FungalForest",  1.0, min_alt=0.640, max_alt=0.820),
        make_entry("PollenPlains",  1.0, max_alt=0.575),
        make_entry("MegaJungle",    2.0),
    ], [
        make_ud_entry("RootTunnel", 1.0),
    ]),

    # ── Infernyx ─────────────────────────────────────────────────────────────
    ("Infernyx", [
        make_entry("BasaltMountains", 1.0, min_alt=0.821),
        make_entry("CinderHills",     1.0, min_alt=0.701, max_alt=0.820),
        make_entry("AshDesert",       1.0, min_lat=0.75),
        make_entry("LavaFields",      1.0, max_alt=0.530),
        make_entry("ScorchedForest",  1.0, min_alt=0.640, max_alt=0.820),
        make_entry("EmberPlains",     1.0, max_alt=0.575),
        make_entry("ObsidianSpires",  2.0),
    ], [
        make_ud_entry("MagmaChamber", 1.0),
    ]),

    # ── Aethermist ───────────────────────────────────────────────────────────
    ("Aethermist", [
        make_entry("ResonancePeaks",  1.0, min_alt=0.821),
        make_entry("CrystalSpires",   1.0, min_alt=0.701, max_alt=0.820),
        make_entry("FloatingIslands", 1.0, min_alt=0.640, max_alt=0.820),
        make_entry("StarlightTundra", 1.0, min_lat=0.70),
        make_entry("EtherMarshes",    1.0, max_alt=0.550),
        make_entry("DreamMeadows",    1.0, max_alt=0.580),
        make_entry("CloudPlains",     2.0),
    ], [
        make_ud_entry("PhaseCavern", 1.0),
    ]),

    # ── Abyssal ──────────────────────────────────────────────────────────────
    ("Abyssal", [
        make_entry("ScreamingChasms", 1.0, min_alt=0.821),
        make_entry("DemonPillars",    1.0, min_alt=0.701, max_alt=0.820),
        make_entry("CarrionDesert",   1.0, min_lat=0.70),
        make_entry("GoreMarshes",     1.0, max_alt=0.540),
        make_entry("LivingWalls",     1.0, min_alt=0.640, max_alt=0.820),
        make_entry("BloodRivers",     1.0, max_alt=0.580),
        make_entry("BonePlains",      2.0),
    ], [
        make_ud_entry("ChasmFloor", 1.0),
    ]),

    # ── Ethereal ─────────────────────────────────────────────────────────────
    ("Ethereal", [
        make_entry("ThoughtStorms",       1.0, min_alt=0.821),
        make_entry("EchoRuins",           1.0, min_alt=0.701, max_alt=0.820),
        make_entry("FadeZones",           1.0, min_lat=0.70),
        make_entry("VoidWhisperFields",   1.0, max_alt=0.550),
        make_entry("SpiritArchipelagos",  1.0, min_alt=0.640, max_alt=0.820),
        make_entry("DreamDeserts",        1.0, max_alt=0.580),
        make_entry("CrystallizedMemories",2.0),
    ], [
        make_ud_entry("MemoryPool", 1.0),
    ]),

    # ── Feywild ──────────────────────────────────────────────────────────────
    ("Feywild", [
        make_entry("BlossomPeaks",     1.0, min_alt=0.821),
        make_entry("ThorncraftHollow", 1.0, min_alt=0.701, max_alt=0.820),
        make_entry("FeyWastes",        1.0, min_lat=0.75),
        make_entry("ShiftingGlades",   1.0, max_alt=0.540),
        make_entry("EternalForest",    1.0, min_alt=0.640, max_alt=0.820),
        make_entry("SilverMistPlains", 1.0, max_alt=0.580),
        make_entry("MushroomRings",    2.0),
    ], [
        make_ud_entry("FeyRootLair", 1.0),
    ]),
]

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

print("[create_terrain_weight_assets] Starting DataAsset creation for 8 planes...")

for plane_name, surface_entries, underdark_entries in PLANE_DEFS:
    create_asset(plane_name, None, surface_entries, underdark_entries)

print("[create_terrain_weight_assets] Done.")
