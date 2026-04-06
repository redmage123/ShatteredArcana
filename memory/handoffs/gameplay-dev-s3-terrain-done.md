---
from: Gameplay Dev (mythforge-gameplay-dev)
to: QA Lead, Lead Engineer, Project Manager
date: 2026-04-06
sprint: 3
subject: S3-T2, S3-T3, S3-T4 — Terrain Distribution + Underdark + Underwater COMPLETE
---

## Status: COMPLETE (pending build validation)

All three tasks are implemented and committed on the dev server at `~/ShatteredArcana/`.
Full build is blocked by pre-existing UHT errors in Lead Engineer files (see Blocker below).

---

## S3-T2: Per-Plane Terrain Distribution (Config-driven DataAsset)

**File:** `Source/CoMCore/World/CoMWorldGenerator.h` + `.cpp`

### What was done
- `UCoMTerrainWeightTable` upgraded from `UDataAsset` → **`UPrimaryDataAsset`**
- `GetPrimaryAssetId()` override implemented (TypeName = `"TerrainWeightTable"`)
- `GetSurfaceWeightSum()` / `GetUnderdarkWeightSum()` helpers added (Blueprint-accessible)
- `static BuildDefaultWeightTable(ECoMPlane)` factory creates pre-normalised tables for all 5 original planes
- `AssignTerrain()` already hooked into `TerrainWeights` TMap; now also drives Underdark via S3-T3

### 5 Plane Tables (weights sum to 1.0)
| Plane | Theme | Key surface terrain |
|-------|-------|---------------------|
| Aurelith | Golden high-fantasy | Grassland 25%, Forest 20%, Hills 15% |
| Noctharion | Dark arcane shadow | ObsidianPlains 22%, ShadowForest 18%, CrystalDesert 15% |
| Verdantis | Primal nature | MegaJungle 25%, FungalForest 20%, LivingSwamp 15% |
| Infernyx | Fire/iron dual realm | LavaFields 20%, AshDesert 18%, EmberPlains 14% |
| Aethermist | Celestial spirit | CloudPlains 20%, DreamMeadows 18%, CrystalSpires 15% |

### Tests (5): 
- `CoM.WorldGen.S3T2.SurfaceWeightsSumToOne` — all 5 plane surface tables sum to ~1.0
- `CoM.WorldGen.S3T2.UnderdarkWeightsSumToOne` — all 5 plane underdark tables sum to ~1.0
- `CoM.WorldGen.S3T2.WeightsInValidRange` — no negative or >1.0 individual weights
- `CoM.WorldGen.S3T2.NoCrossPlaneTerrain` — no Noctharion/Infernyx types in Aurelith table
- `CoM.WorldGen.S3T2.DataAssetPrimaryId` — `GetPrimaryAssetId()` returns `"TerrainWeightTable"`

---

## S3-T3: Underdark Generation (Layers 8–12)

**File:** `Source/CoMCore/World/CoMWorldGenerator.cpp` — `GenerateUnderdark()`

### What was done
- **Pass 1**: Biome noise + DataAsset `UnderdarkWeights` drive base terrain (60% DataAsset, 40% static for variety; rare feature pockets at 100% DataAsset)
- **Pass 2**: Signature feature-zone clusters stamped per plane (6 zone types × 2–6 clusters each):
  - Aurelith: CavernFloor, FungalGrove, CrystalCavern_Arc, UndergroundLake, DwarvenRuins, DeepChasm
  - Noctharion: ShadowCavern, ObsidianLabyrinth, WebbedTunnels, EchoChamber, BoneOssuary, CorruptedCrystal
  - Verdantis: RootTunnel, MyceliumNet_Verd, InsectHive, UndergroundGarden, AmberPool, PetrifiedForest
  - Infernyx: MagmaChamber, LavaRiver_Inf, ForgeCavern, SulfurPit, ObsidianCavern, CrystalForge
  - Aethermist: PhaseCavern, CrystalResonance, DreamGrotto, GravityWell, MirrorCavern, StarlightPool
- **Pass 3**: Underdark entrances using `CoM::UNDERDARK_ENTRANCES_PER_PLANE_MIN/MAX` constants

### Tests (4):
- `CoM.WorldGen.S3T3.UnderdarkHasEnoughTerrainTypes` — ≥ 5 distinct types per plane
- `CoM.WorldGen.S3T3.UnderdarkTerrainMatchesPlane` — Aurelith UD has no Infernyx/Noctharion/Verdantis/Aethermist-exclusive types; Infernyx has no Aurelith-exclusive types
- `CoM.WorldGen.S3T3.UnderdarkZoneBounds` — all tile positions in [0, MAP_WIDTH) × [0, MAP_HEIGHT)
- `CoM.WorldGen.S3T3.UnderdarkZoneCount` — ≥ 8 terrain zones (types with ≥10 tiles) per plane

---

## S3-T4: Underwater Zone Generation

**File:** `Source/CoMCore/World/CoMWorldGenerator.cpp` — `GenerateUnderwater()`

### What was done
- **Fixed**: Old code used wrong terrain types (Shore, BioluminescentDeep, CrystalLake, Volcano)
- **New Pass 1**: Depth-driven classification using bathymetry + volcanic noise:
  - Near shore → CoralReef (warm planes) / KelpForest (Noctharion, Aethermist)
  - Shallow open → CoralReef / KelpForest mix
  - Mid-depth → OceanFloor (with plane-specific pockets: CrystalGrotto_UW for Aethermist)
  - Deep → DeepTrench / AbyssalPlain
  - Volcanic hot-spots (noise >0.82) → UnderwaterVolcano / ThermalVent
  - Noctharion: SunkenRuins pockets in mid-deep zones
- **New Pass 2**: Guarantees all 5 canonical types present on any ocean-bearing plane

### 5 Canonical Types Guaranteed
`OceanFloor` · `CoralReef` · `KelpForest` · `DeepTrench` · `UnderwaterVolcano`

### Tests (4):
- `CoM.WorldGen.S3T4.AllFiveTypesPresent` — all 5 types exist on every ocean-bearing plane
- `CoM.WorldGen.S3T4.UnderwaterBoundaryValid` — non-impassable tiles only under Ocean surface
- `CoM.WorldGen.S3T4.DepthProgression` — DeepTrench tiles average farther from shore than CoralReef
- `CoM.WorldGen.S3T4.TilePositionBounds` — all UW tile positions within map bounds

---

## Blocker: Build validation

**BLK-S3-001** — Pre-existing UHT errors prevent full compilation:
- `CoMLeyPortalSubsystem.h:45,48,51,54,57,60` — `const FStruct*` returns in UFUNCTION declarations
- `CoMTerritorySubsystem.h:94` — C++ default param uses `CoM::MAX_INFLUENCE_RADIUS` namespace constant

These are NOT caused by S3-T2/T3/T4 changes. They exist in Lead Engineer files awaiting S3-T1.
**Lead Engineer: Please fix these so QA can run the 13 new tests.**

---

## Files Modified
- `Source/CoMCore/World/CoMWorldGenerator.h` — UPrimaryDataAsset upgrade, helpers, static factory
- `Source/CoMCore/World/CoMWorldGenerator.cpp` — DataAsset impl, enhanced Underdark + Underwater
- `Source/CoMCore/Tests/CoMWorldGeneratorTests.cpp` — 13 new tests (tests 12–24)

## Test Count
- Pre-existing: 11 tests
- New (S3-T2): 5 tests  
- New (S3-T3): 4 tests
- New (S3-T4): 4 tests
- **Total: 24 tests**

## Next Steps
1. Lead Engineer: fix BLK-S3-001 UHT errors (S3-T1)
2. QA Lead: run full `CoM.WorldGen.*` test suite once build is green
3. If Lead Engineer S3-T1 changes `CoMWorldGenerator.h`, coordinate to avoid conflicts
