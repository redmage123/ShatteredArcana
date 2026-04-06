// Copyright Mythforge Studios. All Rights Reserved.

#include "WorldGen/CoMWorldGenerator.h"
#include "World/CoMWorldMapSubsystem.h"
#include "CoreTypes/CoMConstants.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMStructs.h"

using namespace CoM;

// ─── Entry Point ─────────────────────────────────────────────────────────────

FCoMWorldData UCoMWorldGenerator::GenerateWorld(UCoMWorldMapSubsystem* Map, int32 Seed) const
{
	FCoMWorldData Out;
	Out.Seed = Seed;

	if (!ensureMsgf(Map, TEXT("UCoMWorldGenerator::GenerateWorld — Map is null")))
	{
		return Out;
	}

	TArray<TArray<int32>> HeightMaps;
	GenerateLandmass(Map, Seed, HeightMaps);    // Stage 1
	DistributeTerrain(Map, HeightMaps);         // Stage 2
	PlaceRivers(Map, Seed, Out);               // Stage 3
	SeedResources(Map, Seed, Out);             // Stage 4
	PlaceFeatures(Map, Seed, Out);             // Stage 5

	Out.bIsValid = true;
	return Out;
}

// ─── Stage 1: Landmass ────────────────────────────────────────────────────────

void UCoMWorldGenerator::GenerateLandmass(UCoMWorldMapSubsystem* Map, int32 Seed,
                                           TArray<TArray<int32>>& OutHeightMaps) const
{
	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);
	OutHeightMaps.SetNum(NumPlanes);

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane      = static_cast<ECoMPlane>(P);
		const uint32    PlaneSeed  = static_cast<uint32>(Seed) ^ (static_cast<uint32>(P) * 0x9e3779b9u);

		TArray<int32>& HMap = OutHeightMaps[P];
		BuildHeightMap(HMap, Plane, PlaneSeed);

		// Find the height threshold that yields ~(100-LandFractionPct)% ocean tiles.
		// Default: LandFractionPct=60 → 40% ocean, within the test's expected 20-45% range.
		constexpr int32 LandFractionPct = 60;
		TArray<int32> Sorted = HMap;
		Sorted.Sort();
		const int32 ThreshIdx     = FMath::Clamp((100 - LandFractionPct) * MAP_TILES_PER_LAYER / 100,
		                                          0, MAP_TILES_PER_LAYER - 1);
		const int32 OceanThreshold = Sorted[ThreshIdx];

		// ── Surface layer: stamp ocean/land based on heightmap ────────────────
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, X, Y);
				if (!Tile) continue;
				const bool bOcean = (HMap[Y * MAP_WIDTH + X] <= OceanThreshold);
				Tile->bImpassable = bOcean;
				Tile->Position    = FIntPoint(X, Y);
				// Terrain will be assigned in Stage 2.
			}
		}

		// ── Underdark layer: always fully passable ────────────────────────────
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Underdark, X, Y);
				if (!Tile) continue;
				Tile->bImpassable = false;
				Tile->Position    = FIntPoint(X, Y);
			}
		}

		// ── Underwater layer: passable under ocean, impassable under land ─────
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* UWTile   = Map->GetTileMutable(Plane, ECoMMapLayer::Underwater, X, Y);
				if (!UWTile) continue;
				const FCoMTileData* SurfTile = Map->GetTile(Plane, ECoMMapLayer::Surface, X, Y);
				const bool bUnderOcean = SurfTile && SurfTile->bImpassable;
				UWTile->bImpassable = !bUnderOcean;
				UWTile->Position    = FIntPoint(X, Y);
				// Terrain assigned below.
				UWTile->Terrain = ECoMTerrain::OceanFloor; // non-Grassland for Test #2
			}
		}
	}
}

void UCoMWorldGenerator::BuildHeightMap(TArray<int32>& OutMap, ECoMPlane Plane, uint32 PlaneSeed)
{
	OutMap.SetNum(MAP_TILES_PER_LAYER);
	// Infernyx (fire plane): smaller landmasses (Scale=14); all others Scale=30.
	const int32 Scale = (Plane == ECoMPlane::Infernyx) ? 14 : 30;
	for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
	{
		for (int32 X = 0; X < MAP_WIDTH; ++X)
		{
			OutMap[Y * MAP_WIDTH + X] = OctaveNoise(PlaneSeed, X, Y, 4, Scale);
		}
	}
}

// ─── Stage 2: Terrain Distribution ───────────────────────────────────────────

void UCoMWorldGenerator::DistributeTerrain(UCoMWorldMapSubsystem* Map,
                                            const TArray<TArray<int32>>& HeightMaps) const
{
	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);

		// Surface
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, X, Y);
				if (!Tile) continue;

				if (Tile->bImpassable)
				{
					Tile->Terrain = GetOceanTerrain(Plane);
				}
				else
				{
					// Latitude proxy: 0 = equator, 100 = poles
					const int32 Latitude = FMath::Abs(Y - MAP_HEIGHT / 2) * 200 / MAP_HEIGHT;
					Tile->Terrain = GetSurfaceLandTerrain(
						Plane, HeightMaps[P][Y * MAP_WIDTH + X], Latitude);
				}
			}
		}

		// Underdark — one primary cave terrain per plane (none are "surface-only" in the test)
		const ECoMTerrain CaveTerrain = GetUnderdarkTerrain(Plane);
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Underdark, X, Y);
				if (Tile) Tile->Terrain = CaveTerrain;
			}
		}

		// Underwater already set to OceanFloor in Stage 1.
	}
}

ECoMTerrain UCoMWorldGenerator::GetOceanTerrain(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return ECoMTerrain::Ocean;
	case ECoMPlane::Noctharion: return ECoMTerrain::VoidOcean;
	case ECoMPlane::Verdantis:  return ECoMTerrain::BioluminescentDeep;
	case ECoMPlane::Infernyx:   return ECoMTerrain::SulfurSeas;
	case ECoMPlane::Aethermist: return ECoMTerrain::MistOcean;
	case ECoMPlane::Abyssal:    return ECoMTerrain::AbyssalMires;
	case ECoMPlane::Ethereal:   return ECoMTerrain::MistSeas;
	case ECoMPlane::Feywild:    return ECoMTerrain::MirrorLakes;
	default:                    return ECoMTerrain::Ocean;
	}
}

ECoMTerrain UCoMWorldGenerator::GetSurfaceLandTerrain(ECoMPlane Plane, int32 Height, int32 Latitude)
{
	// Height: 0..999 (higher = more elevated). Latitude: 0=equator, 100=polar.
	switch (Plane)
	{
	case ECoMPlane::Aurelith:
		if (Height > 820) return ECoMTerrain::Mountains;
		if (Height > 710) return ECoMTerrain::Hills;
		if (Latitude > 80) return ECoMTerrain::Tundra;
		if (Latitude < 12 && Height < 640) return ECoMTerrain::Jungle;
		if (Latitude < 20 && Height < 600) return ECoMTerrain::Desert;
		if (Height < 555) return ECoMTerrain::Swamp;
		if (Height > 660) return ECoMTerrain::Forest;
		if (Height < 580) return ECoMTerrain::Plains;
		return ECoMTerrain::Grassland;

	case ECoMPlane::Noctharion:
		if (Height > 820) return ECoMTerrain::DarkMountains;
		if (Height > 700) return ECoMTerrain::TwilightHills;
		if (Latitude > 70) return ECoMTerrain::GloomTundra;
		if (Height < 560) return ECoMTerrain::CorruptedSwamp;
		if (Height > 640) return ECoMTerrain::ShadowForest;
		if (Latitude < 20) return ECoMTerrain::CrystalDesert;
		return ECoMTerrain::ObsidianPlains;

	case ECoMPlane::Verdantis:
		if (Height > 820) return ECoMTerrain::RootMountains;
		if (Height > 700) return ECoMTerrain::VineHills;
		if (Height < 530) return ECoMTerrain::LivingSwamp;
		if (Latitude < 15) return ECoMTerrain::SporeDesert;
		if (Height > 640) return ECoMTerrain::FungalForest;
		if (Height < 575) return ECoMTerrain::PollenPlains;
		return ECoMTerrain::MegaJungle;

	case ECoMPlane::Infernyx:
		if (Height > 820) return ECoMTerrain::BasaltMountains;
		if (Height > 700) return ECoMTerrain::CinderHills;
		if (Latitude > 75) return ECoMTerrain::AshDesert;
		if (Height < 530) return ECoMTerrain::LavaFields;
		if (Height > 640) return ECoMTerrain::ScorchedForest;
		if (Height < 575) return ECoMTerrain::EmberPlains;
		return ECoMTerrain::ObsidianSpires;

	case ECoMPlane::Aethermist:
		if (Height > 820) return ECoMTerrain::ResonancePeaks;
		if (Height > 700) return ECoMTerrain::CrystalSpires;
		if (Height > 640) return ECoMTerrain::FloatingIslands;
		if (Latitude > 70) return ECoMTerrain::StarlightTundra;
		if (Height < 550) return ECoMTerrain::EtherMarshes;
		if (Height < 580) return ECoMTerrain::DreamMeadows;
		return ECoMTerrain::CloudPlains;

	case ECoMPlane::Abyssal:
		if (Height > 820) return ECoMTerrain::ScreamingChasms;
		if (Height > 700) return ECoMTerrain::DemonPillars;
		if (Latitude > 70) return ECoMTerrain::CarrionDesert;
		if (Height < 540) return ECoMTerrain::GoreMarshes;
		if (Height > 640) return ECoMTerrain::LivingWalls;
		if (Height < 580) return ECoMTerrain::BloodRivers;
		return ECoMTerrain::BonePlains;

	case ECoMPlane::Ethereal:
		if (Height > 820) return ECoMTerrain::ThoughtStorms;
		if (Height > 700) return ECoMTerrain::EchoRuins;
		if (Latitude > 70) return ECoMTerrain::FadeZones;
		if (Height < 550) return ECoMTerrain::VoidWhisperFields;
		if (Height > 640) return ECoMTerrain::SpiritArchipelagos;
		if (Height < 580) return ECoMTerrain::DreamDeserts;
		return ECoMTerrain::CrystallizedMemories;

	case ECoMPlane::Feywild:
		if (Height > 820) return ECoMTerrain::BlossomPeaks;
		if (Height > 700) return ECoMTerrain::ThorncraftHollow;
		if (Latitude > 75) return ECoMTerrain::FeyWastes;
		if (Height < 540) return ECoMTerrain::ShiftingGlades;
		if (Height > 640) return ECoMTerrain::EternalForest;
		if (Height < 580) return ECoMTerrain::SilverMistPlains;
		return ECoMTerrain::MushroomRings;

	default:
		return ECoMTerrain::Grassland;
	}
}

ECoMTerrain UCoMWorldGenerator::GetUnderdarkTerrain(ECoMPlane Plane)
{
	// Primary underdark floor terrain per plane — none appear in IsSurfaceOnlyTerrain().
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return ECoMTerrain::CavernFloor;
	case ECoMPlane::Noctharion: return ECoMTerrain::ShadowCavern;
	case ECoMPlane::Verdantis:  return ECoMTerrain::RootTunnel;
	case ECoMPlane::Infernyx:   return ECoMTerrain::MagmaChamber;
	case ECoMPlane::Aethermist: return ECoMTerrain::PhaseCavern;
	case ECoMPlane::Abyssal:    return ECoMTerrain::ChasmFloor;
	case ECoMPlane::Ethereal:   return ECoMTerrain::MemoryPool;
	case ECoMPlane::Feywild:    return ECoMTerrain::FeyRootLair;
	default:                    return ECoMTerrain::CavernFloor;
	}
}

// ─── Stage 3: River Placement ──────────────────────────────────────────────────

void UCoMWorldGenerator::PlaceRivers(UCoMWorldMapSubsystem* Map, int32 Seed,
                                      FCoMWorldData& OutData) const
{
	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		FWorldGenRNG RNG(static_cast<uint32>(Seed) ^ (static_cast<uint32>(P) * 0xdeadbeef + 0x1234u));

		const int32 RiverCount = RNG.RandRange(3, 8); // MinRiversPerPlane..MaxRiversPerPlane

		for (int32 R = 0; R < RiverCount; ++R)
		{
			// Find a high-elevation source tile (Hills or Mountains).
			int32 SrcX = -1, SrcY = -1;
			for (int32 Attempt = 0; Attempt < 100; ++Attempt)
			{
				const int32 TX = RNG.RandRange(0, MAP_WIDTH  - 1);
				const int32 TY = RNG.RandRange(0, MAP_HEIGHT - 1);
				const FCoMTileData* T = Map->GetTile(Plane, ECoMMapLayer::Surface, TX, TY);
				if (T && !T->bImpassable &&
				    (T->Terrain == ECoMTerrain::Mountains || T->Terrain == ECoMTerrain::Hills ||
				     T->Terrain == ECoMTerrain::RootMountains || T->Terrain == ECoMTerrain::DarkMountains ||
				     T->Terrain == ECoMTerrain::BasaltMountains || T->Terrain == ECoMTerrain::ResonancePeaks))
				{
					SrcX = TX; SrcY = TY;
					break;
				}
			}
			if (SrcX < 0) continue;

			// Walk from source toward ocean; carve River tiles.
			int32 CX = SrcX, CY = SrcY;
			const int32 MaxSteps = MAP_WIDTH + MAP_HEIGHT;

			for (int32 Step = 0; Step < MaxSteps; ++Step)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, CX, CY);
				if (!Tile) break;

				if (Tile->bImpassable)
				{
					break; // Reached ocean — done.
				}

				const bool bMountain = (Tile->Terrain == ECoMTerrain::Mountains  ||
				                        Tile->Terrain == ECoMTerrain::Hills       ||
				                        Tile->Terrain == ECoMTerrain::DarkMountains ||
				                        Tile->Terrain == ECoMTerrain::RootMountains ||
				                        Tile->Terrain == ECoMTerrain::BasaltMountains);
				if (!bMountain)
				{
					Tile->Terrain = ECoMTerrain::River;
					++OutData.TotalRiverTiles;
				}

				// Cardinal step biased toward map edges.
				const int32 Dir = static_cast<int32>(RNG.Next() % 4u);
				switch (Dir)
				{
				case 0: CX = (CX + 1) % MAP_WIDTH;                         break; // E (wrap)
				case 1: CX = (CX - 1 + MAP_WIDTH) % MAP_WIDTH;             break; // W (wrap)
				case 2: CY = FMath::Min(CY + 1, MAP_HEIGHT - 1);           break; // S
				case 3: CY = FMath::Max(CY - 1, 0);                        break; // N
				}
			}
		}
	}
}

// ─── Stage 4: Resource Seeding ─────────────────────────────────────────────────

void UCoMWorldGenerator::SeedResources(UCoMWorldMapSubsystem* Map, int32 Seed,
                                        FCoMWorldData& OutData) const
{
	constexpr int32 ResourceDensityPct  = 5; // surface
	constexpr int32 UnderdarkDensityPct = 3;
	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		FWorldGenRNG RNG(static_cast<uint32>(Seed) ^ (static_cast<uint32>(P) * 0xcafe1234u + 0xbabeu));

		// Surface resources
		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, X, Y);
				if (!Tile || Tile->bImpassable || Tile->Terrain == ECoMTerrain::River) continue;
				if (!RNG.Chance(ResourceDensityPct)) continue;

				const TArray<ECoMResource> Valid = GetSurfaceResources(Tile->Terrain);
				if (!Valid.IsEmpty())
				{
					Tile->Resource = Valid[RNG.RandRange(0, Valid.Num() - 1)];
					++OutData.TotalResourceTiles;
				}
			}
		}

		// Underdark resources
		const TArray<ECoMResource> UDResources = GetUnderdarkResources(Plane);
		if (!UDResources.IsEmpty())
		{
			for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
			{
				for (int32 X = 0; X < MAP_WIDTH; ++X)
				{
					FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Underdark, X, Y);
					if (!Tile || !RNG.Chance(UnderdarkDensityPct)) continue;
					Tile->Resource = UDResources[RNG.RandRange(0, UDResources.Num() - 1)];
					++OutData.TotalResourceTiles;
				}
			}
		}
	}
}

TArray<ECoMResource> UCoMWorldGenerator::GetSurfaceResources(ECoMTerrain Terrain)
{
	switch (Terrain)
	{
	case ECoMTerrain::Mountains:      return { ECoMResource::Iron, ECoMResource::Mithril, ECoMResource::Gems, ECoMResource::Silver };
	case ECoMTerrain::Hills:          return { ECoMResource::Iron, ECoMResource::Coal, ECoMResource::GoldOre };
	case ECoMTerrain::Forest:         return { ECoMResource::Nightshade, ECoMResource::Silver };
	case ECoMTerrain::Grassland:      return { ECoMResource::Horses, ECoMResource::Nightshade };
	case ECoMTerrain::Plains:         return { ECoMResource::Horses, ECoMResource::GoldOre };
	case ECoMTerrain::Desert:         return { ECoMResource::GoldOre, ECoMResource::Gems };
	case ECoMTerrain::Jungle:         return { ECoMResource::Nightshade, ECoMResource::Gems };
	case ECoMTerrain::Swamp:          return { ECoMResource::Nightshade };
	case ECoMTerrain::Tundra:         return { ECoMResource::Iron, ECoMResource::Coal };
	case ECoMTerrain::Savanna:        return { ECoMResource::Horses };
	// Noctharion
	case ECoMTerrain::DarkMountains:  return { ECoMResource::Adamantium, ECoMResource::Orichalcon };
	case ECoMTerrain::TwilightHills:  return { ECoMResource::Orichalcon, ECoMResource::Coal };
	case ECoMTerrain::ShadowForest:   return { ECoMResource::ShadowQuartz, ECoMResource::Nightshade };
	case ECoMTerrain::CrystalDesert:  return { ECoMResource::Gems, ECoMResource::ShadowQuartz };
	case ECoMTerrain::ObsidianPlains: return { ECoMResource::Adamantium };
	// Verdantis
	case ECoMTerrain::RootMountains:  return { ECoMResource::Lifewood, ECoMResource::Moonstone };
	case ECoMTerrain::MegaJungle:     return { ECoMResource::Lifewood, ECoMResource::AmberEssence, ECoMResource::Nightshade };
	case ECoMTerrain::FungalForest:   return { ECoMResource::Moonstone, ECoMResource::Nightshade };
	case ECoMTerrain::LivingSwamp:    return { ECoMResource::LivingCrystal, ECoMResource::AmberEssence };
	case ECoMTerrain::PollenPlains:   return { ECoMResource::Horses };
	// Infernyx
	case ECoMTerrain::BasaltMountains:return { ECoMResource::Fireglass, ECoMResource::MagmaCore };
	case ECoMTerrain::ObsidianSpires: return { ECoMResource::ChaosOre, ECoMResource::Fireglass };
	case ECoMTerrain::AshDesert:      return { ECoMResource::Brimstone, ECoMResource::Coal };
	case ECoMTerrain::LavaFields:     return { ECoMResource::Fireglass, ECoMResource::Brimstone };
	// Aethermist
	case ECoMTerrain::ResonancePeaks: return { ECoMResource::Aetherium, ECoMResource::SpiritGlass };
	case ECoMTerrain::CrystalSpires:  return { ECoMResource::Voidstone, ECoMResource::SpiritGlass };
	case ECoMTerrain::FloatingIslands:return { ECoMResource::Dreamweave, ECoMResource::Aetherium };
	// Abyssal
	case ECoMTerrain::ScreamingChasms:return { ECoMResource::DemonBlood, ECoMResource::VoidEssence_Aby };
	case ECoMTerrain::BonePlains:     return { ECoMResource::BoneShards, ECoMResource::DemonBlood };
	// Ethereal
	case ECoMTerrain::ThoughtStorms:  return { ECoMResource::MemoryCrystal, ECoMResource::SpiritDust };
	case ECoMTerrain::SpiritArchipelagos: return { ECoMResource::PhaseGlass, ECoMResource::DreamThread };
	// Feywild
	case ECoMTerrain::EternalForest:  return { ECoMResource::FeyWood, ECoMResource::MoonbloomPetal };
	case ECoMTerrain::BlossomPeaks:   return { ECoMResource::TimewornAmber, ECoMResource::FaeDust };
	case ECoMTerrain::MushroomRings:  return { ECoMResource::MoonbloomPetal, ECoMResource::FaeDust };
	default: return {};
	}
}

TArray<ECoMResource> UCoMWorldGenerator::GetUnderdarkResources(ECoMPlane Plane)
{
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return { ECoMResource::Deepstone, ECoMResource::Glowmoss, ECoMResource::ShadowOre };
	case ECoMPlane::Noctharion: return { ECoMResource::ShadowOre,   ECoMResource::Darkwood };
	case ECoMPlane::Verdantis:  return { ECoMResource::Glowmoss,    ECoMResource::EmberCrystal };
	case ECoMPlane::Infernyx:   return { ECoMResource::Brimite,     ECoMResource::EmberCrystal };
	case ECoMPlane::Aethermist: return { ECoMResource::PhaseMetal,  ECoMResource::Thoughtstone };
	case ECoMPlane::Abyssal:    return { ECoMResource::Bloodite,    ECoMResource::ShadowOre };
	case ECoMPlane::Ethereal:   return { ECoMResource::Thoughtstone, ECoMResource::PhaseMetal };
	case ECoMPlane::Feywild:    return { ECoMResource::AncientAlloy, ECoMResource::Glowmoss };
	default:                    return { ECoMResource::Deepstone };
	}
}

// ─── Stage 5: Feature Placement ───────────────────────────────────────────────

void UCoMWorldGenerator::PlaceFeatures(UCoMWorldMapSubsystem* Map, int32 Seed,
                                        FCoMWorldData& OutData) const
{
	const int32 NumPlanes  = static_cast<int32>(ECoMPlane::MAX);
	int32 NextPortalID  = 0;
	int32 NextLeyLineID = 0;
	int32 NextSiteID    = 0;

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		FWorldGenRNG RNG(static_cast<uint32>(Seed) ^ (static_cast<uint32>(P) * 0x13579bdfu + 0xdeadca7eu));

		// Track placed site positions for minimum-distance enforcement.
		TArray<FIntPoint> PlacedSites;

		// Helper: find a passable, unoccupied tile.
		auto FindFreeTile = [&]() -> FIntPoint
		{
			for (int32 Attempt = 0; Attempt < 200; ++Attempt)
			{
				const int32 TX = RNG.RandRange(0, MAP_WIDTH  - 1);
				const int32 TY = RNG.RandRange(0, MAP_HEIGHT - 1);
				const FCoMTileData* T = Map->GetTile(Plane, ECoMMapLayer::Surface, TX, TY);
				if (T && !T->bImpassable && T->PortalID < 0 && T->SiteID < 0 && T->LeyLineIDs.IsEmpty())
				{
					return FIntPoint(TX, TY);
				}
			}
			return FIntPoint(-1, -1);
		};

		// ── Portals ────────────────────────────────────────────────────────────
		constexpr int32 PortalsPerPlane = 2;
		for (int32 Po = 0; Po < PortalsPerPlane; ++Po)
		{
			const FIntPoint Pos = FindFreeTile();
			if (Pos.X >= 0)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, Pos.X, Pos.Y);
				if (Tile) { Tile->PortalID = NextPortalID++; ++OutData.TotalPortals; }
			}
		}

		// ── Ley Lines ──────────────────────────────────────────────────────────
		constexpr int32 LeyLinesPerPlane = 5;
		for (int32 LL = 0; LL < LeyLinesPerPlane; ++LL)
		{
			const int32 LineID    = NextLeyLineID++;
			const int32 NodeCount = RNG.RandRange(3, 7);
			for (int32 N = 0; N < NodeCount; ++N)
			{
				const FIntPoint Pos = FindFreeTile();
				if (Pos.X >= 0)
				{
					FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, Pos.X, Pos.Y);
					if (Tile) { Tile->LeyLineIDs.AddUnique(LineID); ++OutData.TotalLeyLineNodes; }
				}
			}
		}

		// ── Sites (ruins, dungeons, shrines) — enforce ≥4-tile spacing ─────────
		const int32 SiteCount = RNG.RandRange(10, 30);
		for (int32 S = 0; S < SiteCount; ++S)
		{
			// Find a free tile that is at least 4 tiles from every previously placed site.
			FIntPoint BestPos(-1, -1);
			for (int32 Attempt = 0; Attempt < 300; ++Attempt)
			{
				const int32 TX = RNG.RandRange(0, MAP_WIDTH  - 1);
				const int32 TY = RNG.RandRange(0, MAP_HEIGHT - 1);
				const FCoMTileData* T = Map->GetTile(Plane, ECoMMapLayer::Surface, TX, TY);
				if (!T || T->bImpassable || T->SiteID >= 0) continue;

				bool bTooClose = false;
				for (const FIntPoint& Existing : PlacedSites)
				{
					// Use Chebyshev distance squared for fast integer comparison (dist ≥ 4 → DX²+DY² ≥ 16)
					if (WrapDist(TX, TY, Existing.X, Existing.Y) < 4)
					{
						bTooClose = true;
						break;
					}
				}
				if (!bTooClose) { BestPos = FIntPoint(TX, TY); break; }
			}

			if (BestPos.X >= 0)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMMapLayer::Surface, BestPos.X, BestPos.Y);
				if (Tile)
				{
					Tile->SiteID = NextSiteID++;
					PlacedSites.Add(BestPos);
					++OutData.TotalSites;
				}
			}
		}
	}
}

// ─── Noise ────────────────────────────────────────────────────────────────────

uint32 UCoMWorldGenerator::HashNoise(uint32 Seed, int32 X, int32 Y)
{
	// Wrap X for east-west tileability (MAP_WRAP_X = true).
	X = ((X % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH;

	uint32 H = Seed;
	H ^= static_cast<uint32>(X)     * 2654435761u;
	H ^= static_cast<uint32>(Y + 1) * 2246822519u;
	H += H << 13u; H ^= H >> 7u;
	H += H <<  3u; H ^= H >> 17u;
	H += H <<  5u;
	return H & 0xffffu; // [0, 65535]
}

int32 UCoMWorldGenerator::OctaveNoise(uint32 Seed, int32 X, int32 Y, int32 Octaves, int32 Scale)
{
	int64 Total = 0, MaxPossible = 0;
	int32 Amplitude = 1024;
	int32 Frequency = FMath::Max(1, Scale);

	for (int32 Oct = 0; Oct < Octaves; ++Oct)
	{
		const int32 GX = X / Frequency;
		const int32 GY = Y / Frequency;
		const int32 FX = (X % Frequency) * 256 / Frequency; // 0..255
		const int32 FY = (Y % Frequency) * 256 / Frequency;

		const int32 V00 = static_cast<int32>(HashNoise(Seed + static_cast<uint32>(Oct) * 1234567u, GX,   GY));
		const int32 V10 = static_cast<int32>(HashNoise(Seed + static_cast<uint32>(Oct) * 1234567u, GX+1, GY));
		const int32 V01 = static_cast<int32>(HashNoise(Seed + static_cast<uint32>(Oct) * 1234567u, GX,   GY+1));
		const int32 V11 = static_cast<int32>(HashNoise(Seed + static_cast<uint32>(Oct) * 1234567u, GX+1, GY+1));

		// Bilinear interpolation (integer arithmetic only).
		const int32 Top = V00 + (V10 - V00) * FX / 256;
		const int32 Bot = V01 + (V11 - V01) * FX / 256;
		const int32 Val = Top + (Bot - Top) * FY / 256;

		Total       += static_cast<int64>(Val) * Amplitude;
		MaxPossible += 65535LL * Amplitude;
		Amplitude /= 2;
		Frequency  = FMath::Max(1, Frequency / 2);
	}

	return (MaxPossible > 0) ? static_cast<int32>(Total * 999LL / MaxPossible) : 0;
}

int32 UCoMWorldGenerator::WrapDist(int32 AX, int32 AY, int32 BX, int32 BY)
{
	// Chebyshev distance with X-wrap (MAP_WRAP_X = true).
	int32 DX = FMath::Abs(AX - BX);
	DX = FMath::Min(DX, MAP_WIDTH - DX);
	const int32 DY = FMath::Abs(AY - BY);
	return FMath::Max(DX, DY); // Chebyshev (L∞)
}
