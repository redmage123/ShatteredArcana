// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMWorldGenerator.cpp — Procedural world generation with biome-coherent terrain.
//
// Terrain assignment uses a Whittaker-style biome classification:
//   Elevation (Perlin) × Moisture (Perlin) × Temperature (latitude + noise)
//   → biome zone → plane-specific terrain type
//
// This guarantees spatial coherence: jungles form contiguous regions,
// deserts cluster together, and transitions are gradual
// (jungle → forest → grassland → savanna → desert).
//
// Rain Shadow Effect (Pangaea model):
//   Each plane has a prevailing wind direction. After elevation is computed,
//   a moisture shadow pass reduces moisture on the leeward side of mountain
//   ranges. This creates realistic geography: wet windward forests and dry
//   interior deserts, just like Pangaea's interior was arid because
//   peripheral mountains blocked oceanic moisture.

#include "CoMWorldGenerator.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"

// ════════════════════════════════════════════════════════════════════════════
// UCoMTerrainWeightTable — PrimaryDataAsset implementation (S3-T2)
// ════════════════════════════════════════════════════════════════════════════

FPrimaryAssetId UCoMTerrainWeightTable::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("TerrainWeightTable"), GetFName());
}

float UCoMTerrainWeightTable::GetSurfaceWeightSum() const
{
	float Total = 0.0f;
	for (const FCoMTerrainWeight& W : SurfaceWeights) Total += W.Weight;
	return Total;
}

float UCoMTerrainWeightTable::GetUnderdarkWeightSum() const
{
	float Total = 0.0f;
	for (const FCoMTerrainWeight& W : UnderdarkWeights) Total += W.Weight;
	return Total;
}

// ---------------------------------------------------------------------------
// Default per-plane weight table factory (used by tests and blueprint setup).
// Weights in each array sum to 1.0 — verified by S3-T2 unit tests.
// ---------------------------------------------------------------------------
UCoMTerrainWeightTable* UCoMTerrainWeightTable::BuildDefaultWeightTable(ECoMPlane Plane)
{
	UCoMTerrainWeightTable* Table = NewObject<UCoMTerrainWeightTable>();
	Table->Plane = Plane;

	auto AddW = [](TArray<FCoMTerrainWeight>& Arr, ECoMTerrain T, float W)
	{
		FCoMTerrainWeight E; E.TerrainType = T; E.Weight = W;
		Arr.Add(E);
	};

	switch (Plane)
	{
	case ECoMPlane::Aurelith:
		// Surface: golden high-fantasy — meadows, forests, mild mountains (sum=1.0)
		AddW(Table->SurfaceWeights, ECoMTerrain::Grassland,       0.25f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Forest,          0.20f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Hills,           0.15f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Mountains,       0.10f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Savanna,         0.08f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Plains,          0.07f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Desert,          0.05f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Swamp,           0.04f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Jungle,          0.03f);
		AddW(Table->SurfaceWeights, ECoMTerrain::EnchantedForest, 0.02f);
		AddW(Table->SurfaceWeights, ECoMTerrain::Tundra,          0.01f);
		// Underdark: The Deep Halls (sum=1.0)
		AddW(Table->UnderdarkWeights, ECoMTerrain::CavernFloor,       0.30f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::FungalGrove,       0.20f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::UndergroundLake,   0.15f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CrystalCavern_Arc, 0.12f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::DwarvenRuins,      0.08f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CollapsedTunnel,   0.07f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::LavaTube_Arc,      0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::DeepChasm,         0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::MineralVein,       0.01f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::UndergroundRiver,  0.01f);
		break;

	case ECoMPlane::Noctharion:
		// Surface: dark arcane shadow realm (sum=1.0)
		AddW(Table->SurfaceWeights, ECoMTerrain::ObsidianPlains,  0.22f);
		AddW(Table->SurfaceWeights, ECoMTerrain::ShadowForest,    0.18f);
		AddW(Table->SurfaceWeights, ECoMTerrain::CrystalDesert,   0.15f);
		AddW(Table->SurfaceWeights, ECoMTerrain::DarkMountains,   0.12f);
		AddW(Table->SurfaceWeights, ECoMTerrain::AshWastes,       0.10f);
		AddW(Table->SurfaceWeights, ECoMTerrain::CorruptedSwamp,  0.10f);
		AddW(Table->SurfaceWeights, ECoMTerrain::TwilightHills,   0.08f);
		AddW(Table->SurfaceWeights, ECoMTerrain::GloomTundra,     0.04f);
		AddW(Table->SurfaceWeights, ECoMTerrain::ShadowRift,      0.01f);
		// Underdark: The Shadow Deep (sum=1.0)
		AddW(Table->UnderdarkWeights, ECoMTerrain::ShadowCavern,      0.30f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::ObsidianLabyrinth, 0.18f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::WebbedTunnels,     0.16f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::EchoChamber,       0.13f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::BoneOssuary,       0.10f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CorruptedCrystal,  0.07f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::DarkRiver,         0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::VoidPocket,        0.02f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::SoulWell,          0.01f);
		break;

	case ECoMPlane::Verdantis:
		// Surface: primal nature realm — jungle-dominant (sum=1.0)
		AddW(Table->SurfaceWeights, ECoMTerrain::MegaJungle,      0.25f);
		AddW(Table->SurfaceWeights, ECoMTerrain::FungalForest,    0.20f);
		AddW(Table->SurfaceWeights, ECoMTerrain::LivingSwamp,     0.15f);
		AddW(Table->SurfaceWeights, ECoMTerrain::PollenPlains,    0.12f);
		AddW(Table->SurfaceWeights, ECoMTerrain::RootMountains,   0.08f);
		AddW(Table->SurfaceWeights, ECoMTerrain::VineHills,       0.07f);
		AddW(Table->SurfaceWeights, ECoMTerrain::SporeDesert,     0.05f);
		AddW(Table->SurfaceWeights, ECoMTerrain::AmberCoast,      0.04f);
		AddW(Table->SurfaceWeights, ECoMTerrain::MyceliumNetwork, 0.02f);
		AddW(Table->SurfaceWeights, ECoMTerrain::WorldTree,       0.02f);
		// Underdark: The Root World (sum=1.0)
		AddW(Table->UnderdarkWeights, ECoMTerrain::RootTunnel,        0.25f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::MyceliumNet_Verd,  0.20f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::InsectHive,        0.15f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::UndergroundGarden, 0.12f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::MudCavern,         0.10f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::AmberPool,         0.07f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::PetrifiedForest,   0.05f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CocoonChamber,     0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::SapRiver,          0.02f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::SeedVault,         0.01f);
		break;

	case ECoMPlane::Infernyx:
		// Surface: fire/iron dual realm — volcanic + iron cities (sum=1.0)
		AddW(Table->SurfaceWeights, ECoMTerrain::LavaFields,      0.20f);
		AddW(Table->SurfaceWeights, ECoMTerrain::AshDesert,       0.18f);
		AddW(Table->SurfaceWeights, ECoMTerrain::EmberPlains,     0.14f);
		AddW(Table->SurfaceWeights, ECoMTerrain::CinderHills,     0.12f);
		AddW(Table->SurfaceWeights, ECoMTerrain::BasaltMountains, 0.10f);
		AddW(Table->SurfaceWeights, ECoMTerrain::ObsidianSpires,  0.08f);
		AddW(Table->SurfaceWeights, ECoMTerrain::ScorchedForest,  0.06f);
		AddW(Table->SurfaceWeights, ECoMTerrain::MagmaRivers,     0.04f);
		AddW(Table->SurfaceWeights, ECoMTerrain::IronFortresses,  0.04f);
		AddW(Table->SurfaceWeights, ECoMTerrain::FireGeyser,      0.02f);
		AddW(Table->SurfaceWeights, ECoMTerrain::VolcanicChain,   0.02f);
		// Underdark: The Molten Depths (sum=1.0)
		AddW(Table->UnderdarkWeights, ECoMTerrain::MagmaChamber,  0.25f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::ObsidianCavern,0.20f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::SulfurPit,     0.15f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::ForgeCavern,   0.12f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::LavaRiver_Inf, 0.10f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CinderTunnel,  0.07f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::EmberShelf,    0.05f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::DemonPit,      0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CrystalForge,  0.02f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::SmokeVent,     0.01f);
		break;

	case ECoMPlane::Aethermist:
		// Surface: celestial spirit realm — floating, crystalline (sum=1.0)
		AddW(Table->SurfaceWeights, ECoMTerrain::CloudPlains,      0.20f);
		AddW(Table->SurfaceWeights, ECoMTerrain::DreamMeadows,     0.18f);
		AddW(Table->SurfaceWeights, ECoMTerrain::CrystalSpires,    0.15f);
		AddW(Table->SurfaceWeights, ECoMTerrain::FloatingIslands,  0.12f);
		AddW(Table->SurfaceWeights, ECoMTerrain::ResonancePeaks,   0.10f);
		AddW(Table->SurfaceWeights, ECoMTerrain::StarlightTundra,  0.08f);
		AddW(Table->SurfaceWeights, ECoMTerrain::EtherMarshes,     0.07f);
		AddW(Table->SurfaceWeights, ECoMTerrain::VoidRifts,        0.05f);
		AddW(Table->SurfaceWeights, ECoMTerrain::AuroraField,      0.03f);
		AddW(Table->SurfaceWeights, ECoMTerrain::GravityAnomaly,   0.02f);
		// Underdark: The Inverse (sum=1.0)
		AddW(Table->UnderdarkWeights, ECoMTerrain::PhaseCavern,      0.25f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CrystalResonance, 0.20f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::DreamGrotto,      0.15f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::GravityWell,      0.12f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::MirrorCavern,     0.10f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::StarlightPool,    0.08f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::TemporalEddy,     0.05f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::VoidBubble,       0.03f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::ThoughtCorridor,  0.02f);
		break;

	default:
		AddW(Table->SurfaceWeights,   ECoMTerrain::Grassland,   1.0f);
		AddW(Table->UnderdarkWeights, ECoMTerrain::CavernFloor, 1.0f);
		break;
	}

	return Table;
}


// ════════════════════════════════════════════════════════════════════════════
// Noise helpers — simple Perlin-like noise via FMath
// ════════════════════════════════════════════════════════════════════════════

// Multi-octave 2D noise returning [0, 1]
static float OctaveNoise2D(float X, float Y, int32 Octaves, float Persistence, float Lacunarity, int32 SeedOffset)
{
	float Total = 0.0f;
	float MaxValue = 0.0f;
	float Amplitude = 1.0f;
	float Frequency = 1.0f;

	for (int32 i = 0; i < Octaves; ++i)
	{
		// FMath::PerlinNoise2D returns [-1, 1]
		float NoiseVal = FMath::PerlinNoise2D(FVector2D(
			(X + SeedOffset * 1000.0f) * Frequency,
			(Y + SeedOffset * 1000.0f) * Frequency
		));
		Total += NoiseVal * Amplitude;
		MaxValue += Amplitude;
		Amplitude *= Persistence;
		Frequency *= Lacunarity;
	}

	// Normalize to [0, 1]
	return (Total / MaxValue + 1.0f) * 0.5f;
}

// ════════════════════════════════════════════════════════════════════════════
// Biome classification — Whittaker model adapted for fantasy
// ════════════════════════════════════════════════════════════════════════════

// Returns a generic biome code based on elevation, moisture, temperature
// 0=Ocean, 1=Desert, 2=Savanna, 3=Grassland, 4=Forest, 5=Jungle,
// 6=Swamp, 7=Tundra, 8=Mountains, 9=Hills, 10=Shore, 11=Volcano
static int32 ClassifyBiome(float Elevation, float Moisture, float Temperature)
{
	if (Elevation < 0.30f) return 0;  // Ocean
	if (Elevation < 0.33f) return 10; // Shore/Coast
	if (Elevation > 0.85f) return 8;  // Mountains
	if (Elevation > 0.92f) return 11; // Volcano (rare peaks)
	if (Elevation > 0.72f) return 9;  // Hills

	// Land biomes: moisture × temperature → biome
	if (Temperature < 0.25f)
	{
		// Cold
		if (Moisture > 0.5f) return 7;  // Tundra (cold + wet)
		return 1; // Cold desert
	}
	if (Temperature < 0.5f)
	{
		// Temperate
		if (Moisture > 0.7f) return 4;  // Forest (temperate + wet)
		if (Moisture > 0.4f) return 3;  // Grassland (temperate + moderate)
		return 2; // Savanna (temperate + dry)
	}
	// Hot
	if (Moisture > 0.7f) return 5;  // Jungle (hot + wet)
	if (Moisture > 0.5f) return 6;  // Swamp (hot + moderate-wet)
	if (Moisture > 0.3f) return 2;  // Savanna (hot + moderate)
	return 1; // Desert (hot + dry)
}

// Map generic biome code to plane-specific ECoMTerrain
static ECoMTerrain BiomeToTerrain(int32 Biome, ECoMPlane Plane)
{
	// Default (Aurelith) terrain mapping
	switch (Biome)
	{
	case 0:  return ECoMTerrain::Ocean;
	case 1:  // Desert
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Desert;
		case ECoMPlane::Noctharion: return ECoMTerrain::CrystalDesert;
		case ECoMPlane::Verdantis:  return ECoMTerrain::SporeDesert;
		case ECoMPlane::Infernyx:   return ECoMTerrain::AshDesert;
		case ECoMPlane::Aethermist: return ECoMTerrain::StarlightTundra;
		case ECoMPlane::Abyssal:    return ECoMTerrain::CarrionDesert;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Desert; // TODO: Ethereal desert
		case ECoMPlane::Feywild:    return ECoMTerrain::DreamMeadows;
		default: return ECoMTerrain::Desert;
		}
	case 2:  // Savanna
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Savanna;
		case ECoMPlane::Noctharion: return ECoMTerrain::AshWastes;
		case ECoMPlane::Verdantis:  return ECoMTerrain::PollenPlains;
		case ECoMPlane::Infernyx:   return ECoMTerrain::EmberPlains;
		case ECoMPlane::Aethermist: return ECoMTerrain::CloudPlains;
		case ECoMPlane::Abyssal:    return ECoMTerrain::BonePlains;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Plains;
		case ECoMPlane::Feywild:    return ECoMTerrain::DreamMeadows;
		default: return ECoMTerrain::Savanna;
		}
	case 3:  // Grassland
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Grassland;
		case ECoMPlane::Noctharion: return ECoMTerrain::ObsidianPlains;
		case ECoMPlane::Verdantis:  return ECoMTerrain::PollenPlains;
		case ECoMPlane::Infernyx:   return ECoMTerrain::CinderHills;
		case ECoMPlane::Aethermist: return ECoMTerrain::DreamMeadows;
		case ECoMPlane::Abyssal:    return ECoMTerrain::BonePlains;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Grassland;
		case ECoMPlane::Feywild:    return ECoMTerrain::DreamMeadows;
		default: return ECoMTerrain::Grassland;
		}
	case 4:  // Forest
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Forest;
		case ECoMPlane::Noctharion: return ECoMTerrain::ShadowForest;
		case ECoMPlane::Verdantis:  return ECoMTerrain::FungalForest;
		case ECoMPlane::Infernyx:   return ECoMTerrain::ScorchedForest;
		case ECoMPlane::Aethermist: return ECoMTerrain::CrystalSpires;
		case ECoMPlane::Abyssal:    return ECoMTerrain::LivingWalls;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Forest;
		case ECoMPlane::Feywild:    return ECoMTerrain::EnchantedForest;
		default: return ECoMTerrain::Forest;
		}
	case 5:  // Jungle
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Jungle;
		case ECoMPlane::Noctharion: return ECoMTerrain::CorruptedSwamp;
		case ECoMPlane::Verdantis:  return ECoMTerrain::MegaJungle;
		case ECoMPlane::Infernyx:   return ECoMTerrain::LavaFields;
		case ECoMPlane::Aethermist: return ECoMTerrain::EtherMarshes;
		case ECoMPlane::Abyssal:    return ECoMTerrain::FleshCitadels;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Jungle;
		case ECoMPlane::Feywild:    return ECoMTerrain::MegaJungle;
		default: return ECoMTerrain::Jungle;
		}
	case 6:  // Swamp
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Swamp;
		case ECoMPlane::Noctharion: return ECoMTerrain::CorruptedSwamp;
		case ECoMPlane::Verdantis:  return ECoMTerrain::LivingSwamp;
		case ECoMPlane::Infernyx:   return ECoMTerrain::SulfurSeas;
		case ECoMPlane::Aethermist: return ECoMTerrain::EtherMarshes;
		case ECoMPlane::Abyssal:    return ECoMTerrain::AbyssalMires;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Swamp;
		case ECoMPlane::Feywild:    return ECoMTerrain::LivingSwamp;
		default: return ECoMTerrain::Swamp;
		}
	case 7:  // Tundra
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Tundra;
		case ECoMPlane::Noctharion: return ECoMTerrain::GloomTundra;
		case ECoMPlane::Verdantis:  return ECoMTerrain::SporeDesert;
		case ECoMPlane::Infernyx:   return ECoMTerrain::AshDesert;
		case ECoMPlane::Aethermist: return ECoMTerrain::StarlightTundra;
		case ECoMPlane::Abyssal:    return ECoMTerrain::CarrionDesert;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Tundra;
		case ECoMPlane::Feywild:    return ECoMTerrain::StarlightTundra;
		default: return ECoMTerrain::Tundra;
		}
	case 8:  // Mountains
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Mountains;
		case ECoMPlane::Noctharion: return ECoMTerrain::DarkMountains;
		case ECoMPlane::Verdantis:  return ECoMTerrain::RootMountains;
		case ECoMPlane::Infernyx:   return ECoMTerrain::BasaltMountains;
		case ECoMPlane::Aethermist: return ECoMTerrain::ResonancePeaks;
		case ECoMPlane::Abyssal:    return ECoMTerrain::DemonPillars;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Mountains;
		case ECoMPlane::Feywild:    return ECoMTerrain::ResonancePeaks;
		default: return ECoMTerrain::Mountains;
		}
	case 9:  // Hills
		switch (Plane)
		{
		case ECoMPlane::Aurelith:   return ECoMTerrain::Hills;
		case ECoMPlane::Noctharion: return ECoMTerrain::TwilightHills;
		case ECoMPlane::Verdantis:  return ECoMTerrain::VineHills;
		case ECoMPlane::Infernyx:   return ECoMTerrain::CinderHills;
		case ECoMPlane::Aethermist: return ECoMTerrain::FloatingIslands;
		case ECoMPlane::Abyssal:    return ECoMTerrain::ScreamingChasms;
		case ECoMPlane::Ethereal:   return ECoMTerrain::Hills;
		case ECoMPlane::Feywild:    return ECoMTerrain::FloatingIslands;
		default: return ECoMTerrain::Hills;
		}
	case 10: // Shore
		switch (Plane)
		{
		case ECoMPlane::Verdantis:  return ECoMTerrain::AmberCoast;
		case ECoMPlane::Abyssal:    return ECoMTerrain::ChasmEdge;
		default: return ECoMTerrain::Shore;
		}
	case 11: // Volcano
		switch (Plane)
		{
		case ECoMPlane::Infernyx:   return ECoMTerrain::VolcanicChain;
		default: return ECoMTerrain::Mountains;
		}
	default: return ECoMTerrain::Grassland;
	}
}

// Map generic biome code to plane-specific Underdark terrain
static ECoMTerrain BiomeToUnderdarkTerrain(int32 SurfaceBiome, ECoMPlane Plane)
{
	// Surface mountains → deep caverns, surface water → underground lakes, etc.
	bool bDeep = (SurfaceBiome == 8 || SurfaceBiome == 11);  // Mountains/Volcano above
	bool bWet  = (SurfaceBiome == 0 || SurfaceBiome == 5 || SurfaceBiome == 6 || SurfaceBiome == 10);

	switch (Plane)
	{
	case ECoMPlane::Aurelith:
		if (bDeep) return ECoMTerrain::DeepChasm;
		if (bWet) return ECoMTerrain::UndergroundLake;
		return ECoMTerrain::CavernFloor;
	case ECoMPlane::Noctharion:
		if (bDeep) return ECoMTerrain::ObsidianLabyrinth;
		if (bWet) return ECoMTerrain::DarkRiver;
		return ECoMTerrain::ShadowCavern;
	case ECoMPlane::Verdantis:
		if (bDeep) return ECoMTerrain::PetrifiedForest;
		if (bWet) return ECoMTerrain::SapRiver;
		return ECoMTerrain::RootTunnel;
	case ECoMPlane::Infernyx:
		if (bDeep) return ECoMTerrain::MagmaChamber;
		if (bWet) return ECoMTerrain::LavaRiver_Inf;
		return ECoMTerrain::ObsidianCavern;
	case ECoMPlane::Aethermist:
		if (bDeep) return ECoMTerrain::ParadoxChamber;
		if (bWet) return ECoMTerrain::StarlightPool;
		return ECoMTerrain::PhaseCavern;
	case ECoMPlane::Abyssal:
		if (bDeep) return ECoMTerrain::ChaosCore;
		if (bWet) return ECoMTerrain::BloodPool_Aby;
		return ECoMTerrain::ChasmFloor;
	case ECoMPlane::Ethereal:
		if (bDeep) return ECoMTerrain::DeepChasm;
		if (bWet) return ECoMTerrain::UndergroundLake;
		return ECoMTerrain::CavernFloor;
	case ECoMPlane::Feywild:
		if (bDeep) return ECoMTerrain::CrystalCavern_Arc;
		if (bWet) return ECoMTerrain::UndergroundLake;
		return ECoMTerrain::FungalGrove;
	default:
		return ECoMTerrain::CavernFloor;
	}
}

// ════════════════════════════════════════════════════════════════════════════
// Per-plane noise modifiers — shift moisture/temperature for plane flavor
// ════════════════════════════════════════════════════════════════════════════

struct FPlaneClimateModifier
{
	float MoistureBias;     // Added to moisture noise (-1 to 1)
	float TemperatureBias;  // Added to temperature noise (-1 to 1)
	float OceanThreshold;   // Override for ocean ratio (0 = use default)
	int32 WindDirX;         // Prevailing wind direction X component (-1, 0, 1)
	int32 WindDirY;         // Prevailing wind direction Y component (-1, 0, 1)
	float RainShadowStrength; // How much mountains block moisture (0-1)
};

static FPlaneClimateModifier GetPlaneModifier(ECoMPlane Plane)
{
	//                          MoistBias  TempBias  OceanThr  WindX  WindY  RainShadow
	switch (Plane)
	{
	case ECoMPlane::Aurelith:   return { 0.0f,  0.0f,  0.0f,   1, 0, 0.6f };  // Wind W→E, strong rain shadow
	case ECoMPlane::Noctharion: return {-0.1f, -0.15f, 0.0f,  -1, 0, 0.5f };  // Wind E→W
	case ECoMPlane::Verdantis:  return { 0.25f, 0.1f, -0.05f,  0, 1, 0.3f };  // Wind S→N, weak shadow (so wet everywhere)
	case ECoMPlane::Infernyx:   return {-0.3f,  0.3f, -0.1f,   1, 0, 0.8f };  // Strong shadow → extreme interior desert
	case ECoMPlane::Aethermist: return { 0.0f, -0.2f,  0.1f,   0,-1, 0.4f };  // Wind N→S
	case ECoMPlane::Abyssal:    return {-0.2f,  0.1f, -0.15f,  1, 1, 0.7f };  // Diagonal wind, strong shadow
	case ECoMPlane::Ethereal:   return { 0.05f,-0.1f,  0.0f,  -1, 0, 0.3f };  // Weak shadow — mists penetrate everywhere
	case ECoMPlane::Feywild:    return { 0.15f, 0.05f, 0.0f,   0, 1, 0.2f };  // Very weak shadow — Feywild ignores physics
	default:                    return { 0.0f,  0.0f,  0.0f,   1, 0, 0.5f };
	}
}

// ════════════════════════════════════════════════════════════════════════════
// Rain Shadow Pass — mountains block moisture from prevailing wind direction
// ════════════════════════════════════════════════════════════════════════════

// Computes a rain shadow map: for each tile, how much moisture is blocked
// by upwind mountains. Returns a 2D array [MapWidth * MapHeight] of
// moisture reduction factors (0.0 = no shadow, 1.0 = fully blocked).
static TArray<float> ComputeRainShadow(
	const TArray<float>& ElevationMap,
	int32 Width, int32 Height,
	int32 WindDirX, int32 WindDirY,
	float Strength,
	float MountainThreshold)
{
	TArray<float> Shadow;
	Shadow.SetNumZeroed(Width * Height);

	if (Strength <= 0.0f) return Shadow;
	if (WindDirX == 0 && WindDirY == 0) return Shadow;

	// For each tile, trace UPWIND and accumulate mountain blockage.
	// The further behind a mountain range you are, the drier it gets.
	// Shadow decays over distance (moisture gradually recovers).

	const int32 TraceLength = 40; // How far upwind to check
	const float DecayPerTile = 0.03f; // Shadow diminishes by this much per tile

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			float Blockage = 0.0f;

			// Trace upwind (opposite of wind direction)
			for (int32 Step = 1; Step <= TraceLength; ++Step)
			{
				int32 SrcX = (X - WindDirX * Step + Width) % Width; // WrapX
				int32 SrcY = FMath::Clamp(Y - WindDirY * Step, 0, Height - 1);

				float SrcElev = ElevationMap[SrcY * Width + SrcX];

				if (SrcElev > MountainThreshold)
				{
					// Mountain found upwind — blocks moisture
					float MountainHeight = (SrcElev - MountainThreshold) / (1.0f - MountainThreshold);
					float ContributedBlock = MountainHeight * Strength;

					// Closer mountains block more
					float DistanceFactor = 1.0f - (float(Step) / TraceLength);
					Blockage = FMath::Max(Blockage, ContributedBlock * DistanceFactor);
				}

				// If we already found strong blockage, no need to trace further
				if (Blockage > 0.8f) break;
			}

			// Apply decay — even in a rain shadow, some moisture gets through
			Shadow[Y * Width + X] = FMath::Clamp(Blockage, 0.0f, 0.85f);
		}
	}

	return Shadow;
}

// ════════════════════════════════════════════════════════════════════════════
// UCoMWorldGenerator implementation
// ════════════════════════════════════════════════════════════════════════════

void UCoMWorldGenerator::GenerateWorld(UCoMWorldMapSubsystem* MapSubsystem, int32 Seed)
{
	if (!MapSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[WorldGen] MapSubsystem is null!"));
		return;
	}

	CurrentSeed = Seed;
	RandomStream.Initialize(Seed);
	CachedMapSubsystem = MapSubsystem;

	// Ensure layers are allocated
	if (!MapSubsystem->IsInitialized())
	{
		MapSubsystem->InitializeLayers();
	}

	UE_LOG(LogTemp, Log, TEXT("[WorldGen] Starting world generation with seed %d"), Seed);

	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);
	for (int32 PlaneIdx = 0; PlaneIdx < NumPlanes; ++PlaneIdx)
	{
		ECoMPlane Plane = static_cast<ECoMPlane>(PlaneIdx);
		UE_LOG(LogTemp, Log, TEXT("[WorldGen] Generating plane %d/%d"), PlaneIdx + 1, NumPlanes);

		// Surface pipeline: 5 stages
		GenerateLandmass(MapSubsystem, Plane);
		AssignTerrain(MapSubsystem, Plane);
		GenerateRivers(MapSubsystem, Plane);
		PlaceResources(MapSubsystem, Plane);
		PlaceFeatures(MapSubsystem, Plane);

		// Secondary layers
		GenerateUnderdark(MapSubsystem, Plane);
		GenerateUnderwater(MapSubsystem, Plane);
	}

	CachedMapSubsystem = nullptr;
	UE_LOG(LogTemp, Log, TEXT("[WorldGen] World generation complete. Seed=%d"), Seed);
}

// ────────────────────────────────────────────────────────────────────────────
// Stage 1: Landmass — elevation noise → ocean/land/mountain classification
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::GenerateLandmass(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	const int32 PlaneOffset = static_cast<int32>(Plane) * 137; // Unique noise seed per plane
	const FPlaneClimateModifier Mod = GetPlaneModifier(Plane);

	FCoMMapLayerData& Surface = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Surface);

	// ── Pass 1: Compute elevation map ──────────────────────────────────
	TArray<float> ElevationMap;
	ElevationMap.SetNum(MapWidth * MapHeight);

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			ElevationMap[Y * MapWidth + X] = OctaveNoise2D(
				X * 0.02f, Y * 0.02f,
				6, 0.5f, 2.0f,
				CurrentSeed + PlaneOffset
			);
		}
	}

	// ── Pass 2: Compute rain shadow from mountains ─────────────────────
	// Mountains block moisture carried by prevailing winds, creating
	// dry leeward interiors (like Pangaea's central desert).
	const float MountainThreshold = 0.72f; // Same as biome classification
	TArray<float> RainShadow = ComputeRainShadow(
		ElevationMap, MapWidth, MapHeight,
		Mod.WindDirX, Mod.WindDirY,
		Mod.RainShadowStrength,
		MountainThreshold
	);

	// ── Pass 3: Classify biomes using elevation + moisture + temperature ─
	// Moisture is computed from noise MINUS the rain shadow reduction.
	// This means tiles downwind of mountain ranges are drier, producing
	// deserts and savanna where forests would otherwise grow.

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			FCoMTileData* Tile = Surface.GetTile(X, Y);
			if (!Tile) continue;

			Tile->Position = FIntPoint(X, Y);

			float Elev = ElevationMap[Y * MapWidth + X];

			// Base moisture from noise
			float Moist = OctaveNoise2D(
				X * 0.03f, Y * 0.03f,
				4, 0.5f, 2.0f,
				CurrentSeed + PlaneOffset + 500
			);

			// Apply rain shadow: reduce moisture on leeward side of mountains
			float Shadow = RainShadow[Y * MapWidth + X];
			Moist = Moist * (1.0f - Shadow);

			// Coastal moisture bonus: tiles near ocean get extra moisture
			// (ocean evaporation feeds coastal precipitation)
			// Check within 5-tile radius for ocean
			float CoastalBonus = 0.0f;
			for (int32 R = 1; R <= 5; ++R)
			{
				for (int32 DY = -1; DY <= 1; ++DY)
				{
					for (int32 DX = -1; DX <= 1; ++DX)
					{
						if (DX == 0 && DY == 0) continue;
						int32 NX = (X + DX * R + MapWidth) % MapWidth;
						int32 NY = FMath::Clamp(Y + DY * R, 0, MapHeight - 1);
						float NElev = ElevationMap[NY * MapWidth + NX];
						if (NElev < 0.30f) // Ocean
						{
							CoastalBonus = FMath::Max(CoastalBonus, 0.15f * (1.0f - float(R) / 6.0f));
						}
					}
				}
			}
			Moist += CoastalBonus;

			// Apply plane climate bias
			Moist = FMath::Clamp(Moist + Mod.MoistureBias, 0.0f, 1.0f);

			// Temperature: latitude-based (warmer at equator) + noise
			float LatFactor = 1.0f - FMath::Abs((float(Y) / MapHeight) - 0.5f) * 2.0f;
			float TempNoise = OctaveNoise2D(
				X * 0.025f, Y * 0.025f,
				3, 0.5f, 2.0f,
				CurrentSeed + PlaneOffset + 1000
			);
			float Temp = FMath::Clamp(LatFactor * 0.7f + TempNoise * 0.3f + Mod.TemperatureBias, 0.0f, 1.0f);

			// Windward mountain slopes get extra moisture (orographic lift)
			if (Elev > 0.65f && Elev < MountainThreshold)
			{
				// Check if we're on the windward side (upwind neighbor is lower)
				int32 UpwindX = (X - Mod.WindDirX + MapWidth) % MapWidth;
				int32 UpwindY = FMath::Clamp(Y - Mod.WindDirY, 0, MapHeight - 1);
				float UpwindElev = ElevationMap[UpwindY * MapWidth + UpwindX];
				if (UpwindElev < Elev)
				{
					// Windward slope — orographic precipitation
					Moist = FMath::Clamp(Moist + 0.2f, 0.0f, 1.0f);
				}
			}

			// Classify biome
			int32 Biome = ClassifyBiome(Elev, Moist, Temp);

			// Assign terrain
			Tile->Terrain = BiomeToTerrain(Biome, Plane);

			// Mark ocean tiles as impassable to ground units
			if (Biome == 0)
			{
				Tile->bImpassable = true;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[WorldGen] Plane %d: landmass complete (rain shadow applied, wind=%d,%d, strength=%.1f)"),
		static_cast<int32>(Plane), Mod.WindDirX, Mod.WindDirY, Mod.RainShadowStrength);
}

// ────────────────────────────────────────────────────────────────────────────
// Stage 2: AssignTerrain — override with DataAsset weights if configured
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::AssignTerrain(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	// If a DataAsset weight table is configured for this plane, use it
	// to refine non-ocean surface tiles. The biome classification from
	// Stage 1 provides the base; the weight table adds variety within
	// each biome zone.
	UCoMTerrainWeightTable* const* TablePtr = TerrainWeights.Find(Plane);
	if (!TablePtr || !(*TablePtr) || (*TablePtr)->SurfaceWeights.Num() == 0)
	{
		// No override — Stage 1 biome classification stands
		return;
	}

	// With a weight table, replace ~20% of non-ocean tiles with weighted picks
	// to add variety while preserving biome coherence
	FCoMMapLayerData& Surface = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Surface);
	const float VarietyChance = 0.20f;

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			FCoMTileData* Tile = Surface.GetTile(X, Y);
			if (!Tile || Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::Shore)
				continue;

			if (RandomStream.FRand() < VarietyChance)
			{
				ECoMTerrain Picked = PickWeightedTerrain((*TablePtr)->SurfaceWeights);
				// Only accept the pick if it's not ocean (prevent DataAsset misconfiguration)
				if (Picked != ECoMTerrain::Ocean)
				{
					Tile->Terrain = Picked;
				}
			}
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Stage 3: Rivers — trace from mountains downhill to ocean
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::GenerateRivers(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	FCoMMapLayerData& Surface = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Surface);
	const int32 PlaneOffset = static_cast<int32>(Plane) * 137;

	// Collect mountain tile positions as river sources
	TArray<FIntPoint> MountainTiles;
	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			const FCoMTileData* Tile = Surface.GetTile(X, Y);
			if (Tile && (Tile->Terrain == ECoMTerrain::Mountains ||
			             Tile->Terrain == ECoMTerrain::DarkMountains ||
			             Tile->Terrain == ECoMTerrain::RootMountains ||
			             Tile->Terrain == ECoMTerrain::BasaltMountains ||
			             Tile->Terrain == ECoMTerrain::ResonancePeaks ||
			             Tile->Terrain == ECoMTerrain::DemonPillars))
			{
				MountainTiles.Add(FIntPoint(X, Y));
			}
		}
	}

	if (MountainTiles.Num() == 0) return;

	// Pick 3-5 random mountain tiles as river sources
	const int32 NumRivers = FMath::Clamp(3 + RandomStream.RandRange(0, 2), 3, FMath::Min(5, MountainTiles.Num()));

	for (int32 RiverIdx = 0; RiverIdx < NumRivers; ++RiverIdx)
	{
		int32 SourceIdx = RandomStream.RandRange(0, MountainTiles.Num() - 1);
		FIntPoint Current = MountainTiles[SourceIdx];

		// Trace downhill using noise-based elevation
		TSet<int32> Visited;
		for (int32 Step = 0; Step < 80; ++Step)
		{
			int32 FlatIdx = Current.Y * MapWidth + ((Current.X % MapWidth + MapWidth) % MapWidth);
			if (Visited.Contains(FlatIdx)) break;
			Visited.Add(FlatIdx);

			FCoMTileData* Tile = Surface.GetTile(Current.X, Current.Y);
			if (!Tile) break;

			// Stop if we hit ocean
			if (Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::Shore)
				break;

			// Don't overwrite mountains — start the river one step away
			if (Step > 0)
			{
				Tile->Terrain = ECoMTerrain::River;
				Tile->bImpassable = false;
			}

			// Find lowest neighbor (steepest descent in noise)
			float BestElev = 999.0f;
			FIntPoint BestNeighbor = Current;

			for (int32 DY = -1; DY <= 1; ++DY)
			{
				for (int32 DX = -1; DX <= 1; ++DX)
				{
					if (DX == 0 && DY == 0) continue;
					int32 NX = (Current.X + DX + MapWidth) % MapWidth; // WrapX
					int32 NY = FMath::Clamp(Current.Y + DY, 0, MapHeight - 1);

					float NElev = OctaveNoise2D(
						NX * 0.02f, NY * 0.02f,
						6, 0.5f, 2.0f,
						CurrentSeed + PlaneOffset
					);

					// Add slight random jitter to prevent perfectly straight rivers
					NElev += RandomStream.FRand() * 0.02f;

					if (NElev < BestElev)
					{
						BestElev = NElev;
						BestNeighbor = FIntPoint(NX, NY);
					}
				}
			}

			if (BestNeighbor == Current) break; // No downhill — stuck
			Current = BestNeighbor;
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Stage 4: Resources — scatter based on terrain
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::PlaceResources(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	FCoMMapLayerData& Surface = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Surface);

	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			FCoMTileData* Tile = Surface.GetTile(X, Y);
			if (!Tile || Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::River)
				continue;

			if (RandomStream.FRand() > ResourceDensity * 0.25f) // ~5% of tiles
				continue;

			// Terrain-appropriate resources
			ECoMTerrain T = Tile->Terrain;

			// Mountains → Iron/Mithril
			if (T == ECoMTerrain::Mountains || T == ECoMTerrain::DarkMountains ||
			    T == ECoMTerrain::RootMountains || T == ECoMTerrain::BasaltMountains ||
			    T == ECoMTerrain::ResonancePeaks || T == ECoMTerrain::DemonPillars)
			{
				Tile->Resource = (RandomStream.FRand() > 0.7f) ? ECoMResource::Mithril : ECoMResource::Iron;
			}
			// Forest → Wood
			else if (T == ECoMTerrain::Forest || T == ECoMTerrain::ShadowForest ||
			         T == ECoMTerrain::FungalForest || T == ECoMTerrain::MegaJungle ||
			         T == ECoMTerrain::EnchantedForest || T == ECoMTerrain::Jungle ||
			         T == ECoMTerrain::ScorchedForest)
			{
				Tile->Resource = ECoMResource::Iron;
			}
			// Hills → Gold/Gems
			else if (T == ECoMTerrain::Hills || T == ECoMTerrain::TwilightHills ||
			         T == ECoMTerrain::VineHills || T == ECoMTerrain::CinderHills)
			{
				Tile->Resource = (RandomStream.FRand() > 0.6f) ? ECoMResource::Gems : ECoMResource::GoldOre;
			}
			// Grassland/Plains → Food (no resource node, implicit via farming)
			// Desert/Tundra → rare resources
			else if (T == ECoMTerrain::Desert || T == ECoMTerrain::CrystalDesert ||
			         T == ECoMTerrain::AshDesert || T == ECoMTerrain::SporeDesert)
			{
				if (RandomStream.FRand() > 0.8f)
				{
					Tile->Resource = ECoMResource::Gems;
				}
			}
			// Volcano → special
			else if (T == ECoMTerrain::Mountains || T == ECoMTerrain::VolcanicChain)
			{
				Tile->Resource = ECoMResource::Adamantium;
			}
		}
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Stage 5: Features — ruins, dungeons, shrines
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::PlaceFeatures(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	FCoMMapLayerData& Surface = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Surface);

	const int32 TargetFeatures = 15 + RandomStream.RandRange(0, 10); // 15-25
	int32 Placed = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = TargetFeatures * 20;
	int32 NextSiteID = static_cast<int32>(Plane) * 100; // Unique IDs per plane

	while (Placed < TargetFeatures && Attempts < MaxAttempts)
	{
		++Attempts;

		int32 X = RandomStream.RandRange(0, MapWidth - 1);
		int32 Y = RandomStream.RandRange(0, MapHeight - 1);

		FCoMTileData* Tile = Surface.GetTile(X, Y);
		if (!Tile) continue;

		// No features on ocean, river, or existing feature tiles
		if (Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::River)
			continue;
		if (Tile->SiteID >= 0) continue;

		// Minimum distance check
		if (!CanPlaceFeature(MapSubsystem, Plane, FIntPoint(X, Y), 5))
			continue;

		Tile->SiteID = NextSiteID++;
		++Placed;
	}
}
// ────────────────────────────────────────────────────────────────────────────
// S3-T3: Underdark generation — plane-unique terrain
// Cave floors, fungal forests, lava fields, crystal caverns per each of 5 planes.
// Layers 8-12 in world-layer numbering (Underdark for each of the 5 original planes).
// DataAsset UnderdarkWeights drive distribution; static mapping is the fallback.
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::GenerateUnderdark(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	FCoMMapLayerData& Underdark = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Underdark);
	const FCoMMapLayerData& Surface = MapSubsystem->GetLayer(Plane, ECoMMapLayer::Surface);
	const int32 PlaneOffset = static_cast<int32>(Plane) * 137;

	// Resolve DataAsset weight table for this plane (may be null)
	const TArray<FCoMTerrainWeight>* UDWeights = nullptr;
	if (UCoMTerrainWeightTable* const* TablePtr = TerrainWeights.Find(Plane))
	{
		if (*TablePtr && (*TablePtr)->UnderdarkWeights.Num() > 0)
			UDWeights = &(*TablePtr)->UnderdarkWeights;
	}

	// ── Pass 1: Base terrain via biome noise + DataAsset weights ─────────────
	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			FCoMTileData* UDTile = Underdark.GetTile(X, Y);
			if (!UDTile) continue;

			UDTile->Position = FIntPoint(X, Y);

			float ZoneNoise = OctaveNoise2D(X * 0.04f, Y * 0.04f, 4, 0.5f, 2.0f,
			                               CurrentSeed + PlaneOffset);
			float FeatNoise = OctaveNoise2D(X * 0.08f, Y * 0.08f, 3, 0.5f, 2.0f,
			                               CurrentSeed + PlaneOffset + 700);
			float Moist = OctaveNoise2D(X * 0.03f, Y * 0.03f, 3, 0.5f, 2.0f,
			                            CurrentSeed + PlaneOffset + 500);
			int32 SurfBiome = ClassifyBiome(ZoneNoise, Moist, 0.5f);

			if (UDWeights)
			{
				// Rare pockets (FeatNoise > 0.85) always use weight table for variety
				float UseChance = (FeatNoise > 0.85f) ? 1.0f : 0.60f;
				if (RandomStream.FRand() < UseChance)
					UDTile->Terrain = PickWeightedTerrain(*UDWeights);
				else
					UDTile->Terrain = BiomeToUnderdarkTerrain(SurfBiome, Plane);
			}
			else
			{
				UDTile->Terrain = BiomeToUnderdarkTerrain(SurfBiome, Plane);
			}
		}
	}

	// ── Pass 2: Signature feature-zone clusters ───────────────────────────────
	//  Guarantee recognisable zones: cave floors, fungal forests, lava fields,
	//  crystal caverns — plane-unique.

	struct FUDZone { ECoMTerrain Terrain; int32 Count; int32 Radius; };
	TArray<FUDZone> FeatureZones;

	switch (Plane)
	{
	case ECoMPlane::Aurelith:    // The Deep Halls
		FeatureZones.Add({ECoMTerrain::CavernFloor,       6, 4});
		FeatureZones.Add({ECoMTerrain::FungalGrove,       4, 3});
		FeatureZones.Add({ECoMTerrain::CrystalCavern_Arc, 3, 2});
		FeatureZones.Add({ECoMTerrain::UndergroundLake,   3, 3});
		FeatureZones.Add({ECoMTerrain::DwarvenRuins,      2, 2});
		FeatureZones.Add({ECoMTerrain::DeepChasm,         2, 2});
		break;
	case ECoMPlane::Noctharion:  // The Shadow Deep
		FeatureZones.Add({ECoMTerrain::ShadowCavern,      6, 4});
		FeatureZones.Add({ECoMTerrain::ObsidianLabyrinth, 4, 3});
		FeatureZones.Add({ECoMTerrain::WebbedTunnels,     3, 3});
		FeatureZones.Add({ECoMTerrain::EchoChamber,       3, 2});
		FeatureZones.Add({ECoMTerrain::BoneOssuary,       2, 2});
		FeatureZones.Add({ECoMTerrain::CorruptedCrystal,  2, 2});
		break;
	case ECoMPlane::Verdantis:   // The Root World
		FeatureZones.Add({ECoMTerrain::RootTunnel,        6, 4});
		FeatureZones.Add({ECoMTerrain::MyceliumNet_Verd,  4, 3});
		FeatureZones.Add({ECoMTerrain::InsectHive,        3, 3});
		FeatureZones.Add({ECoMTerrain::UndergroundGarden, 3, 3});
		FeatureZones.Add({ECoMTerrain::AmberPool,         2, 2});
		FeatureZones.Add({ECoMTerrain::PetrifiedForest,   2, 2});
		break;
	case ECoMPlane::Infernyx:    // The Molten Depths
		FeatureZones.Add({ECoMTerrain::MagmaChamber,  6, 4});
		FeatureZones.Add({ECoMTerrain::LavaRiver_Inf, 4, 3});
		FeatureZones.Add({ECoMTerrain::ForgeCavern,   3, 3});
		FeatureZones.Add({ECoMTerrain::SulfurPit,     3, 3});
		FeatureZones.Add({ECoMTerrain::ObsidianCavern,2, 2});
		FeatureZones.Add({ECoMTerrain::CrystalForge,  2, 2});
		break;
	case ECoMPlane::Aethermist:  // The Inverse
		FeatureZones.Add({ECoMTerrain::PhaseCavern,      6, 4});
		FeatureZones.Add({ECoMTerrain::CrystalResonance, 4, 3});
		FeatureZones.Add({ECoMTerrain::DreamGrotto,      3, 3});
		FeatureZones.Add({ECoMTerrain::GravityWell,      3, 3});
		FeatureZones.Add({ECoMTerrain::MirrorCavern,     2, 2});
		FeatureZones.Add({ECoMTerrain::StarlightPool,    2, 2});
		break;
	default:
		FeatureZones.Add({ECoMTerrain::CavernFloor, 4, 3});
		FeatureZones.Add({ECoMTerrain::FungalGrove,  2, 2});
		break;
	}

	const int32 MaxStampAttempts = 200;
	for (const FUDZone& Zone : FeatureZones)
	{
		int32 Placed = 0, Attempts = 0;
		while (Placed < Zone.Count && Attempts < MaxStampAttempts)
		{
			++Attempts;
			int32 CX = RandomStream.RandRange(Zone.Radius, MapWidth  - Zone.Radius - 1);
			int32 CY = RandomStream.RandRange(Zone.Radius, MapHeight - Zone.Radius - 1);
			int32 Stamped = 0;
			for (int32 DY = -Zone.Radius; DY <= Zone.Radius; ++DY)
			{
				for (int32 DX = -Zone.Radius; DX <= Zone.Radius; ++DX)
				{
					if (DX*DX + DY*DY > Zone.Radius*Zone.Radius) continue;
					int32 TX = (CX+DX+MapWidth) % MapWidth;
					int32 TY = FMath::Clamp(CY+DY, 0, MapHeight-1);
					FCoMTileData* T = Underdark.GetTile(TX, TY);
					if (T) { T->Terrain = Zone.Terrain; ++Stamped; }
				}
			}
			if (Stamped > 0) ++Placed;
		}
	}

	// ── Pass 3: Underdark entrances near surface mountains ────────────────────
	TArray<FIntPoint> MountainTiles;
	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			const FCoMTileData* ST = Surface.GetTile(X, Y);
			if (ST && (ST->Terrain == ECoMTerrain::Mountains      ||
			           ST->Terrain == ECoMTerrain::DarkMountains   ||
			           ST->Terrain == ECoMTerrain::RootMountains   ||
			           ST->Terrain == ECoMTerrain::BasaltMountains ||
			           ST->Terrain == ECoMTerrain::ResonancePeaks  ||
			           ST->Terrain == ECoMTerrain::DemonPillars))
			{
				MountainTiles.Add(FIntPoint(X, Y));
			}
		}
	}

	const int32 NumEntrances = FMath::Clamp(
		CoM::UNDERDARK_ENTRANCES_PER_PLANE_MIN + RandomStream.RandRange(0, 7),
		CoM::UNDERDARK_ENTRANCES_PER_PLANE_MIN,
		FMath::Min(CoM::UNDERDARK_ENTRANCES_PER_PLANE_MAX, MountainTiles.Num())
	);
	for (int32 i = 0; i < NumEntrances && MountainTiles.Num() > 0; ++i)
	{
		int32 Idx = RandomStream.RandRange(0, MountainTiles.Num() - 1);
		FIntPoint Pos = MountainTiles[Idx];
		MountainTiles.RemoveAtSwap(Idx);
		int32 PortalID = static_cast<int32>(Plane) * 1000 + 100 + i;
		FCoMTileData* SurfMut = MapSubsystem->GetTileMutable(Plane, ECoMMapLayer::Surface, Pos.X, Pos.Y);
		FCoMTileData* UDMut   = Underdark.GetTile(Pos.X, Pos.Y);
		if (SurfMut) SurfMut->PortalID = PortalID;
		if (UDMut)   UDMut->PortalID   = PortalID;
	}

	UE_LOG(LogTemp, Log, TEXT("[WorldGen] Plane %d Underdark complete (%d zone types, %d entrances)"),
	       static_cast<int32>(Plane), FeatureZones.Num(), NumEntrances);
}

// ────────────────────────────────────────────────────────────────────────────
// S3-T4: Underwater zone generation
// Five canonical types: OceanFloor, CoralReef, KelpForest, DeepTrench,
// UnderwaterVolcano.  Depth progression: shallow edge → CoralReef/Kelp,
// mid → OceanFloor, deep → DeepTrench, volcanic hot-spots → UnderwaterVolcano.
// All five types guaranteed to appear on any plane that has ocean tiles.
// ────────────────────────────────────────────────────────────────────────────

void UCoMWorldGenerator::GenerateUnderwater(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane)
{
	FCoMMapLayerData& Underwater = MapSubsystem->GetLayerMutable(Plane, ECoMMapLayer::Underwater);
	const FCoMMapLayerData& Surface = MapSubsystem->GetLayer(Plane, ECoMMapLayer::Surface);
	const int32 PlaneOffset = static_cast<int32>(Plane) * 137;

	// ── Pass 1: Depth-driven terrain assignment ───────────────────────────────
	for (int32 Y = 0; Y < MapHeight; ++Y)
	{
		for (int32 X = 0; X < MapWidth; ++X)
		{
			FCoMTileData* UWTile = Underwater.GetTile(X, Y);
			const FCoMTileData* SurfTile = Surface.GetTile(X, Y);
			if (!UWTile || !SurfTile) continue;

			UWTile->Position = FIntPoint(X, Y);

			if (SurfTile->Terrain != ECoMTerrain::Ocean)
			{
				UWTile->bImpassable = true;
				continue;
			}
			UWTile->bImpassable = false;

			// Bathymetry: [0=shallow, 1=abyss]
			float Depth = OctaveNoise2D(X * 0.025f, Y * 0.025f, 4, 0.5f, 2.0f,
			                            CurrentSeed + PlaneOffset + 3000);
			// Volcanic activity for hydrothermal hot-spots
			float Volcanic = OctaveNoise2D(X * 0.06f, Y * 0.06f, 3, 0.5f, 2.0f,
			                               CurrentSeed + PlaneOffset + 4000);

			// Shore proximity: count non-ocean neighbors within 3 tiles
			int32 LandNeighbors = 0;
			for (int32 DY = -3; DY <= 3; ++DY)
			{
				for (int32 DX = -3; DX <= 3; ++DX)
				{
					if (DX == 0 && DY == 0) continue;
					int32 NX = (X+DX+MapWidth) % MapWidth;
					int32 NY = FMath::Clamp(Y+DY, 0, MapHeight-1);
					const FCoMTileData* N = Surface.GetTile(NX, NY);
					if (N && N->Terrain != ECoMTerrain::Ocean) ++LandNeighbors;
				}
			}
			const bool bNearShore = (LandNeighbors >= 3);

			// Classify zone
			if (Volcanic > 0.82f)
			{
				// Hydrothermal hot-spot
				UWTile->Terrain = (Volcanic > 0.91f)
					? ECoMTerrain::ThermalVent
					: ECoMTerrain::UnderwaterVolcano;
			}
			else if (bNearShore)
			{
				// Shallow coastal: reef (warm planes) or kelp (cold/arcane planes)
				bool bPreferKelp = (Plane == ECoMPlane::Noctharion ||
				                    Plane == ECoMPlane::Aethermist);
				UWTile->Terrain = bPreferKelp
					? ECoMTerrain::KelpForest
					: ECoMTerrain::CoralReef;
			}
			else if (Depth < 0.35f)
			{
				// Shallow open water
				UWTile->Terrain = (RandomStream.FRand() < 0.6f)
					? ECoMTerrain::CoralReef
					: ECoMTerrain::KelpForest;
			}
			else if (Depth < 0.65f)
			{
				// Mid-depth: OceanFloor (with plane-specific pockets)
				if (Plane == ECoMPlane::Aethermist && Depth < 0.50f && RandomStream.FRand() < 0.15f)
					UWTile->Terrain = ECoMTerrain::CrystalGrotto_UW;
				else if (Plane == ECoMPlane::Verdantis && RandomStream.FRand() < 0.12f)
					UWTile->Terrain = ECoMTerrain::KelpForest;
				else
					UWTile->Terrain = ECoMTerrain::OceanFloor;
			}
			else
			{
				// Deep to abyssal
				UWTile->Terrain = (Depth < 0.85f && RandomStream.FRand() < 0.45f)
					? ECoMTerrain::AbyssalPlain
					: ECoMTerrain::DeepTrench;
			}

			// Noctharion: sunken ruins pockets in mid-deep zones
			if (Plane == ECoMPlane::Noctharion && Depth > 0.50f && Depth < 0.80f &&
			    RandomStream.FRand() < 0.08f)
			{
				UWTile->Terrain = ECoMTerrain::SunkenRuins;
			}
		}
	}

	// ── Pass 2: Guarantee all 5 canonical types present ──────────────────────
	struct FUWZone { ECoMTerrain Terrain; int32 Radius; };
	const TArray<FUWZone> Required = {
		{ECoMTerrain::OceanFloor,        3},
		{ECoMTerrain::CoralReef,         2},
		{ECoMTerrain::KelpForest,        2},
		{ECoMTerrain::DeepTrench,        2},
		{ECoMTerrain::UnderwaterVolcano, 1},
	};

	TArray<FIntPoint> OceanTiles;
	for (int32 Y = 0; Y < MapHeight; ++Y)
		for (int32 X = 0; X < MapWidth; ++X)
		{
			const FCoMTileData* ST = Surface.GetTile(X, Y);
			if (ST && ST->Terrain == ECoMTerrain::Ocean)
				OceanTiles.Add(FIntPoint(X, Y));
		}

	for (const FUWZone& Zone : Required)
	{
		bool bFound = false;
		for (const FIntPoint& P : OceanTiles)
		{
			const FCoMTileData* T = Underwater.GetTile(P.X, P.Y);
			if (T && T->Terrain == Zone.Terrain) { bFound = true; break; }
		}
		if (bFound || OceanTiles.Num() == 0) continue;

		// Stamp at a random ocean tile
		FIntPoint Center = OceanTiles[RandomStream.RandRange(0, OceanTiles.Num()-1)];
		for (int32 DY = -Zone.Radius; DY <= Zone.Radius; ++DY)
		{
			for (int32 DX = -Zone.Radius; DX <= Zone.Radius; ++DX)
			{
				if (DX*DX + DY*DY > Zone.Radius*Zone.Radius) continue;
				int32 TX = (Center.X+DX+MapWidth) % MapWidth;
				int32 TY = FMath::Clamp(Center.Y+DY, 0, MapHeight-1);
				FCoMTileData* T = Underwater.GetTile(TX, TY);
				const FCoMTileData* S = Surface.GetTile(TX, TY);
				if (T && S && S->Terrain == ECoMTerrain::Ocean)
					T->Terrain = Zone.Terrain;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[WorldGen] Plane %d Underwater: %d ocean tiles, 5 zone types guaranteed"),
	       static_cast<int32>(Plane), OceanTiles.Num());
}

// Helper implementations
// ────────────────────────────────────────────────────────────────────────────

FFixed64 UCoMWorldGenerator::SampleNoise(FIntPoint Position, FFixed64 Frequency, FFixed64 Amplitude) const
{
	float Val = FMath::PerlinNoise2D(FVector2D(
		Position.X * static_cast<float>(Frequency.IsZero() ? 1.0 : Frequency.IsPositive() ? 0.01 : 0.01),
		Position.Y * 0.01f
	));
	return FFixed64((Val + 1.0f) * 0.5f * static_cast<float>(Amplitude.IsZero() ? 1.0 : 1.0));
}

ECoMTerrain UCoMWorldGenerator::PickWeightedTerrain(const TArray<FCoMTerrainWeight>& Weights)
{
	if (Weights.Num() == 0) return ECoMTerrain::Grassland;

	float TotalWeight = 0.0f;
	for (const FCoMTerrainWeight& W : Weights)
	{
		TotalWeight += W.Weight;
	}

	if (TotalWeight <= 0.0f) return Weights[0].TerrainType;

	float Roll = RandomStream.FRand() * TotalWeight;
	float Accumulated = 0.0f;
	for (const FCoMTerrainWeight& W : Weights)
	{
		Accumulated += W.Weight;
		if (Roll <= Accumulated)
		{
			return W.TerrainType;
		}
	}

	return Weights.Last().TerrainType;
}

bool UCoMWorldGenerator::CanPlaceFeature(UCoMWorldMapSubsystem* MapSubsystem, ECoMPlane Plane, FIntPoint Position, int32 Radius) const
{
	const FCoMMapLayerData& Surface = MapSubsystem->GetLayer(Plane, ECoMMapLayer::Surface);

	for (int32 DY = -Radius; DY <= Radius; ++DY)
	{
		for (int32 DX = -Radius; DX <= Radius; ++DX)
		{
			if (DX == 0 && DY == 0) continue;

			int32 NX = (Position.X + DX + MapWidth) % MapWidth;
			int32 NY = FMath::Clamp(Position.Y + DY, 0, MapHeight - 1);

			const FCoMTileData* Neighbor = Surface.GetTile(NX, NY);
			if (Neighbor && Neighbor->SiteID >= 0)
			{
				return false; // Too close to existing feature
			}
		}
	}

	return true;
}
