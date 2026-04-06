# Handoff: S3-T2 — Per-Plane Terrain Distribution

**From:** Gameplay Dev (`mythforge-gameplay-dev`)
**To:** QA Lead, Lead Engineer, PM
**Date:** 2026-04-06
**Status:** COMPLETE — unblocks S3-T3 (Underdark) and S3-T4 (Underwater)

---

## What Was Built

### New File: `Source/CoMCore/WorldGen/TerrainDistribution/CoMTerrainDistributionDataAsset.h/.cpp`

**`UCoMTerrainDistributionDataAsset`** (`UPrimaryDataAsset`, type `"CoMTerrainDistribution"`)

Top-level registry that bundles one `UCoMTerrainWeightDataAsset` per `ECoMPlane`.
One instance of this asset is passed to the generator at world-gen time.

Key API:
- `PlaneWeights` — `TArray<TObjectPtr<UCoMTerrainWeightDataAsset>>` (one per plane)
- `GetWeightsForPlane(ECoMPlane)` — returns the asset for a plane or null
- `BuildPlaneWeightsArray()` — builds the raw ptr array for `DistributeTerrain()`
- `IsDataValid(TArray<FString>& OutErrors)` — validates:
  - No null entries
  - All surface weight arrays non-empty
  - No weight < 0 in any layer
  - Surface/Underdark/Underwater weight sums within 0.001 of 1.0
- `ComputeWeightSum(TArray<FCoMTerrainWeightEntry>)` — static helper; sum == 1.0 invariant
- `CreateDefaults(UObject*)` — static factory producing all 8 planes with design-accurate tables

### Per-Plane Weight Tables (via `CreateDefaults`)

All 8 planes implemented with distinct Surface + Underdark + Underwater tables.
Each layer's weights sum to exactly 1.0. Terrain types are plane-appropriate:

| Plane | Dominant Surface | Underdark | Underwater |
|-------|-----------------|-----------|-----------|
| Aurelith | Grassland/Forest/Plains | CavernFloor/FungalGrove | CoralReef/KelpForest |
| Noctharion | ObsidianPlains/ShadowForest | ShadowCavern/VoidPocket | AbyssalPlain/DeepTrench |
| Verdantis | MegaJungle/FungalForest | RootTunnel/MyceliumNet | KelpForest/CoralReef |
| Infernyx | LavaFields/ObsidianSpires/IronFortresses | MagmaChamber/SulfurPit | ThermalVent/UnderwaterVolcano |
| Aethermist | CloudPlains/CrystalSpires/FloatingIslands | PhaseCavern/CrystalResonance | CrystalGrotto/CoralReef |
| Abyssal | BonePlains/ScreamingChasms/BloodRivers | ChasmFloor/BoneLabyrinth | AbyssalPlain/DeepTrench |
| Ethereal | CrystallizedMemories/MemoryLandscape | MemoryPool/PhaseLabyrinth | OceanFloor/AbyssalPlain |
| Feywild | EternalForest/MushroomRings/TwilightMeadows | FeyRootLair/MushroomCave | CoralReef/KelpForest |

Latitude/altitude range filters are used on surface entries to enforce biome bands
(e.g. Tundra only appears at latitude > 0.8, Mountains only at altitude > 0.82).

### Generator Integration

**`UCoMWorldGenerator::GenerateWorld(Map, Seed, TerrainDist)`** — new overload added:
- Header: `Source/CoMCore/WorldGen/CoMWorldGenerator.h`
- Impl: `Source/CoMCore/WorldGen/CoMWorldGenerator.cpp` (lines ~36–65)
- When `TerrainDist` is non-null: calls `DistributeTerrain(Map, HeightMaps, TerrainDist->BuildPlaneWeightsArray())`
- When null: falls back to empty array (existing hardcoded logic, no breakage)

Existing `GenerateWorld(Map, Seed)` is unchanged — zero regression risk.

### Tests

**`Source/CoMCore/WorldGen/TerrainDistribution/Tests/CoMTerrainDistributionTests.cpp`**

10 tests under `CoM.WorldGen.TerrainDist.*`:

| # | Test Name | What It Verifies |
|---|-----------|-----------------|
| 1 | `WeightSumsToOnePerPlane` | All 8 planes' Surface weights sum = 1.0 (±0.001) |
| 2 | `AllPlanesHaveValidAsset` | CreateDefaults() produces non-null entry for every ECoMPlane |
| 3 | `UnderdarkWeightsSumToOne` | Underdark weights sum = 1.0 per plane |
| 4 | `UnderwaterWeightsSumToOne` | Underwater weights sum = 1.0 per plane |
| 5 | `InvalidWeightRejected` | IsDataValid returns false + error for negative weight |
| 6 | `NullEntryRejected` | IsDataValid returns false + error for null PlaneWeights entry |
| 7 | `GeneratorReadsDataAsset` | GenerateWorld(Dist) succeeds + Aurelith tiles match asset terrain types |
| 8 | `DistinctPlanesDistinctTerrain` | Aurelith≠Infernyx, Verdantis≠Noctharion dominant terrain |
| 9 | `GetWeightsForPlane` | Correct asset returned for each of the 8 planes |
| 10 | `NullDistributionFallsBack` | Null Dist → hardcoded path, bIsValid=true, no crash |

Existing tests in `CoMTerrainDistTests.cpp` (CoM.TerrainDist.*, 6 tests) are unchanged.

---

## Files Changed

| File | Action |
|------|--------|
| `Source/CoMCore/WorldGen/TerrainDistribution/CoMTerrainDistributionDataAsset.h` | NEW |
| `Source/CoMCore/WorldGen/TerrainDistribution/CoMTerrainDistributionDataAsset.cpp` | NEW |
| `Source/CoMCore/WorldGen/TerrainDistribution/Tests/CoMTerrainDistributionTests.cpp` | NEW |
| `Source/CoMCore/WorldGen/CoMWorldGenerator.h` | MODIFIED (fwd decl + new overload) |
| `Source/CoMCore/WorldGen/CoMWorldGenerator.cpp` | MODIFIED (new overload impl + include) |
| `kanban/board.md` | UPDATED (S3-T2 → DONE) |

---

## Design Decisions

1. **Separate registry class** (`UCoMTerrainDistributionDataAsset`) rather than passing a raw array
   into `GenerateWorld`. Gives Blueprint-accessible, designer-editable single reference point.

2. **Weights sum to 1.0 enforced by IsDataValid** — raw entries are relative (not normalized
   internally), but the design contract requires they sum to 1.0 so intent is explicit in the
   asset. `PickWeightedTerrain` normalizes at sample time regardless.

3. **Latitude/altitude bands** on per-plane entries mirror the structure of the original
   hardcoded `GetSurfaceLandTerrain` switch, so DataAsset terrain distribution is faithful
   to the existing biome design.

4. **Zero regression** — original `GenerateWorld(Map, Seed)` is untouched and still delegates
   to the empty-PlaneWeights path. New overload is additive.

---

## For S3-T3 (Underdark) and S3-T4 (Underwater)

- **S3-T3**: Use `CreateDefaults()` as the baseline for Underdark weights; the tables are
  already in place (UnderdarkWeights populated for all 8 planes). The Underdark generation
  task should add zone-stamping logic on top of the per-tile weighted assignment.

- **S3-T4**: UnderwaterWeights are populated for all 8 planes. The Underwater zone task
  should build zone-placement logic (seabed, reef, trench, etc.) reading from these tables.

Both tasks can pass the same `UCoMTerrainDistributionDataAsset*` to `GenerateWorld` and
access layer-specific weights via `GetWeightsForPlane(Plane)->UnderdarkWeights` / `->UnderwaterWeights`.
