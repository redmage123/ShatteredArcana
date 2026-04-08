// OLD Phase 3 code — disabled
#if 0
// Copyright Mythforge Studios. All Rights Reserved.
// COM-S3-T2: Per-Plane Terrain Distribution DataAsset implementation.

#include "WorldGen/TerrainDistribution/CoMTerrainDistributionDataAsset.h"

// ─── Helper macro ─────────────────────────────────────────────────────────────

// Adds one FCoMTerrainWeightEntry to a TArray<FCoMTerrainWeightEntry>&.
// Args: array, terrain enum, weight, minLat, maxLat, minAlt, maxAlt
#define COM_ADD_WEIGHT(Arr, TerrainVal, W, MinLat, MaxLat, MinAlt, MaxAlt) \
	{ FCoMTerrainWeightEntry _E; _E.Terrain = ECoMTerrain::TerrainVal; _E.Weight = (W); \
	  _E.MinLatitude = (MinLat); _E.MaxLatitude = (MaxLat); \
	  _E.MinAltitude = (MinAlt); _E.MaxAltitude = (MaxAlt); (Arr).Add(_E); }

// Wide-coverage entry (all lat/alt): simplest common case.
#define COM_ADD_W(Arr, TerrainVal, W) \
	COM_ADD_WEIGHT(Arr, TerrainVal, W, 0.0f, 1.0f, 0.0f, 1.0f)

// ─── PrimaryAssetId ───────────────────────────────────────────────────────────

FPrimaryAssetId UCoMTerrainDistributionDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMTerrainDistribution"), GetFName());
}

// ─── Query ────────────────────────────────────────────────────────────────────

UCoMTerrainWeightDataAsset* UCoMTerrainDistributionDataAsset::GetWeightsForPlane(ECoMPlane Plane) const
{
	for (UCoMTerrainWeightDataAsset* Asset : PlaneWeights)
	{
		if (Asset && Asset->Plane == Plane)
		{
			return Asset;
		}
	}
	return nullptr;
}

TArray<UCoMTerrainWeightDataAsset*> UCoMTerrainDistributionDataAsset::BuildPlaneWeightsArray() const
{
	TArray<UCoMTerrainWeightDataAsset*> Out;
	Out.Reserve(PlaneWeights.Num());
	for (UCoMTerrainWeightDataAsset* Asset : PlaneWeights)
	{
		if (Asset)
		{
			Out.Add(Asset);
		}
	}
	return Out;
}

// ─── Validation ───────────────────────────────────────────────────────────────

float UCoMTerrainDistributionDataAsset::ComputeWeightSum(const TArray<FCoMTerrainWeightEntry>& Entries)
{
	float Sum = 0.0f;
	for (const FCoMTerrainWeightEntry& E : Entries)
	{
		Sum += E.Weight;
	}
	return Sum;
}

bool UCoMTerrainDistributionDataAsset::IsDataValid(TArray<FString>& OutErrors) const
{
	bool bValid = true;
	constexpr float WeightTolerance = 0.001f;

	if (PlaneWeights.IsEmpty())
	{
		OutErrors.Add(TEXT("PlaneWeights is empty — no terrain distribution configured."));
		return false;
	}

	for (int32 I = 0; I < PlaneWeights.Num(); ++I)
	{
		const UCoMTerrainWeightDataAsset* Asset = PlaneWeights[I];
		if (!Asset)
		{
			OutErrors.Add(FString::Printf(TEXT("PlaneWeights[%d] is null."), I));
			bValid = false;
			continue;
		}

		// Check for negative weights in any layer.
		auto CheckNoNegative = [&](const TArray<FCoMTerrainWeightEntry>& Entries, const TCHAR* LayerName)
		{
			for (const FCoMTerrainWeightEntry& E : Entries)
			{
				if (E.Weight < 0.0f)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Plane %d (%s) has negative weight (%.4f) for terrain %d."),
						static_cast<int32>(Asset->Plane), LayerName,
						E.Weight, static_cast<int32>(E.Terrain)));
					bValid = false;
				}
			}
		};
		CheckNoNegative(Asset->SurfaceWeights,    TEXT("Surface"));
		CheckNoNegative(Asset->UnderdarkWeights,  TEXT("Underdark"));
		CheckNoNegative(Asset->UnderwaterWeights, TEXT("Underwater"));

		// Surface must be non-empty and sum to ≈ 1.0.
		if (Asset->SurfaceWeights.IsEmpty())
		{
			OutErrors.Add(FString::Printf(
				TEXT("Plane %d SurfaceWeights is empty."), static_cast<int32>(Asset->Plane)));
			bValid = false;
		}
		else
		{
			const float Sum = ComputeWeightSum(Asset->SurfaceWeights);
			if (FMath::Abs(Sum - 1.0f) > WeightTolerance)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Plane %d SurfaceWeights sum = %.5f (expected 1.0 ± %.3f)."),
					static_cast<int32>(Asset->Plane), Sum, WeightTolerance));
				bValid = false;
			}
		}

		// Underdark: if non-empty, must also sum to ≈ 1.0.
		if (!Asset->UnderdarkWeights.IsEmpty())
		{
			const float Sum = ComputeWeightSum(Asset->UnderdarkWeights);
			if (FMath::Abs(Sum - 1.0f) > WeightTolerance)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Plane %d UnderdarkWeights sum = %.5f (expected 1.0 ± %.3f)."),
					static_cast<int32>(Asset->Plane), Sum, WeightTolerance));
				bValid = false;
			}
		}

		// Underwater: if non-empty, must also sum to ≈ 1.0.
		if (!Asset->UnderwaterWeights.IsEmpty())
		{
			const float Sum = ComputeWeightSum(Asset->UnderwaterWeights);
			if (FMath::Abs(Sum - 1.0f) > WeightTolerance)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Plane %d UnderwaterWeights sum = %.5f (expected 1.0 ± %.3f)."),
					static_cast<int32>(Asset->Plane), Sum, WeightTolerance));
				bValid = false;
			}
		}
	}

	return bValid;
}

// ─── Default factory ──────────────────────────────────────────────────────────

UCoMTerrainDistributionDataAsset* UCoMTerrainDistributionDataAsset::CreateDefaults(UObject* Outer)
{
	UCoMTerrainDistributionDataAsset* Dist =
		NewObject<UCoMTerrainDistributionDataAsset>(Outer ? Outer : GetTransientPackage());

	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);
	Dist->PlaneWeights.Reserve(NumPlanes);

	for (int32 P = 0; P < NumPlanes; ++P)
	{
		Dist->PlaneWeights.Add(MakePlaneAsset(static_cast<ECoMPlane>(P), Dist));
	}

	return Dist;
}

// ---------------------------------------------------------------------------
// MakePlaneAsset — per-plane weight tables
//
// Design rules applied here (matching GetSurfaceLandTerrain / GDD):
//   • Surface weights sum to exactly 1.0 (validated by IsDataValid).
//   • Underdark weights sum to exactly 1.0.
//   • Underwater weights sum to exactly 1.0.
//   • Terrain types are plane-appropriate (matched to ECoMTerrain plane sections).
//   • Latitude/altitude ranges are used to enforce biome bands where meaningful.
// ---------------------------------------------------------------------------

UCoMTerrainWeightDataAsset* UCoMTerrainDistributionDataAsset::MakePlaneAsset(
	ECoMPlane Plane, UObject* Outer)
{
	UCoMTerrainWeightDataAsset* A = NewObject<UCoMTerrainWeightDataAsset>(Outer);
	A->Plane = Plane;

	TArray<FCoMTerrainWeightEntry>& S  = A->SurfaceWeights;
	TArray<FCoMTerrainWeightEntry>& UD = A->UnderdarkWeights;
	TArray<FCoMTerrainWeightEntry>& UW = A->UnderwaterWeights;

	switch (Plane)
	{
	// ──────────────────────────────────────────────────────────────────────────
	// Aurelith — Golden high-fantasy realm: rich diverse temperate terrain
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Aurelith:
		// Surface (sum = 1.00) — biome bands by altitude and latitude
		COM_ADD_WEIGHT(S, Mountains,  0.10f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, Hills,      0.15f, 0.0f, 1.0f, 0.71f, 0.82f)
		COM_ADD_WEIGHT(S, Tundra,     0.05f, 0.8f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, Jungle,     0.03f, 0.0f, 0.2f, 0.0f,  0.65f)
		COM_ADD_WEIGHT(S, Desert,     0.05f, 0.0f, 0.2f, 0.0f,  0.60f)
		COM_ADD_WEIGHT(S, Swamp,      0.07f, 0.0f, 1.0f, 0.0f,  0.56f)
		COM_ADD_WEIGHT(S, Forest,     0.20f, 0.0f, 0.8f, 0.66f, 1.0f)
		COM_ADD_WEIGHT(S, Plains,     0.15f, 0.0f, 1.0f, 0.0f,  0.59f)
		COM_ADD_WEIGHT(S, Grassland,  0.20f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.10+0.15+0.05+0.03+0.05+0.07+0.20+0.15+0.20 = 1.00

		// Underdark: The Deep Halls (sum = 1.00)
		COM_ADD_W(UD, CavernFloor,      0.30f)
		COM_ADD_W(UD, FungalGrove,      0.20f)
		COM_ADD_W(UD, CrystalCavern_Arc,0.15f)
		COM_ADD_W(UD, UndergroundLake,  0.15f)
		COM_ADD_W(UD, DwarvenRuins,     0.10f)
		COM_ADD_W(UD, CollapsedTunnel,  0.05f)
		COM_ADD_W(UD, DeepChasm,        0.05f)
		// sum = 1.00

		// Underwater (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,       0.35f)
		COM_ADD_W(UW, CoralReef,        0.25f)
		COM_ADD_W(UW, KelpForest,       0.20f)
		COM_ADD_W(UW, SunkenRuins,      0.10f)
		COM_ADD_W(UW, CrystalGrotto_UW, 0.05f)
		COM_ADD_W(UW, Seamount,         0.05f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Noctharion — Dark arcane shadow realm: obsidian, void, twilight terrain
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Noctharion:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, DarkMountains,   0.07f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, TwilightHills,   0.15f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, GloomTundra,     0.13f, 0.7f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, CorruptedSwamp,  0.10f, 0.0f, 1.0f, 0.0f,  0.56f)
		COM_ADD_WEIGHT(S, ShadowForest,    0.18f, 0.0f, 0.7f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, CrystalDesert,   0.07f, 0.0f, 0.2f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, AshWastes,       0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, ObsidianPlains,  0.25f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.07+0.15+0.13+0.10+0.18+0.07+0.05+0.25 = 1.00

		// Underdark: The Shadow Deep (sum = 1.00)
		COM_ADD_W(UD, ShadowCavern,       0.30f)
		COM_ADD_W(UD, ObsidianLabyrinth,  0.20f)
		COM_ADD_W(UD, VoidPocket,         0.15f)
		COM_ADD_W(UD, BoneOssuary,        0.12f)
		COM_ADD_W(UD, WebbedTunnels,      0.10f)
		COM_ADD_W(UD, CorruptedCrystal,   0.08f)
		COM_ADD_W(UD, EchoChamber,        0.05f)
		// sum = 1.00

		// Underwater (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,         0.40f)
		COM_ADD_W(UW, AbyssalPlain,       0.30f)
		COM_ADD_W(UW, DeepTrench,         0.20f)
		COM_ADD_W(UW, ThermalVent,        0.10f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Verdantis — Primal nature realm: mega-jungle, fungi, living terrain
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Verdantis:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, RootMountains,  0.05f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, VineHills,      0.10f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, LivingSwamp,    0.15f, 0.0f, 1.0f, 0.0f,  0.53f)
		COM_ADD_WEIGHT(S, SporeDesert,    0.05f, 0.0f, 0.2f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, FungalForest,   0.20f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, PollenPlains,   0.15f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, AmberCoast,     0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, MegaJungle,     0.25f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.05+0.10+0.15+0.05+0.20+0.15+0.05+0.25 = 1.00

		// Underdark: The Root World (sum = 1.00)
		COM_ADD_W(UD, RootTunnel,         0.30f)
		COM_ADD_W(UD, MyceliumNet_Verd,   0.20f)
		COM_ADD_W(UD, UndergroundGarden,  0.15f)
		COM_ADD_W(UD, AmberPool,          0.15f)
		COM_ADD_W(UD, InsectHive,         0.10f)
		COM_ADD_W(UD, MudCavern,          0.05f)
		COM_ADD_W(UD, SeedVault,          0.05f)
		// sum = 1.00

		// Underwater (bioluminescent) (sum = 1.00)
		COM_ADD_W(UW, KelpForest,         0.35f)
		COM_ADD_W(UW, CoralReef,          0.30f)
		COM_ADD_W(UW, OceanFloor,         0.20f)
		COM_ADD_W(UW, CrystalGrotto_UW,  0.15f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Infernyx — Fire/iron dual realm: volcanic + devil iron-city terrain
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Infernyx:
		// Surface (sum = 1.00) — volcanic majority, iron-city fraction
		COM_ADD_WEIGHT(S, BasaltMountains, 0.05f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, CinderHills,     0.10f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, AshDesert,       0.13f, 0.75f,1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, LavaFields,      0.25f, 0.0f, 1.0f, 0.0f,  0.53f)
		COM_ADD_WEIGHT(S, ScorchedForest,  0.08f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, EmberPlains,     0.15f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, ObsidianSpires,  0.12f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, IronFortresses,  0.07f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, SoulForges,      0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.05+0.10+0.13+0.25+0.08+0.15+0.12+0.07+0.05 = 1.00

		// Underdark: The Molten Depths (sum = 1.00)
		COM_ADD_W(UD, MagmaChamber,       0.30f)
		COM_ADD_W(UD, ObsidianCavern,     0.20f)
		COM_ADD_W(UD, SulfurPit,          0.15f)
		COM_ADD_W(UD, ForgeCavern,        0.15f)
		COM_ADD_W(UD, LavaRiver_Inf,      0.10f)
		COM_ADD_W(UD, CinderTunnel,       0.05f)
		COM_ADD_W(UD, EmberShelf,         0.05f)
		// sum = 1.00

		// Underwater: superheated sulfur seas (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,         0.30f)
		COM_ADD_W(UW, ThermalVent,        0.25f)
		COM_ADD_W(UW, UnderwaterVolcano,  0.25f)
		COM_ADD_W(UW, AbyssalPlain,       0.20f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Aethermist — Celestial spirit realm: floating islands, crystal spires, mist
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Aethermist:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, ResonancePeaks,    0.10f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, CrystalSpires,     0.15f, 0.0f, 1.0f, 0.70f, 1.0f)
		COM_ADD_WEIGHT(S, FloatingIslands,   0.15f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, StarlightTundra,   0.08f, 0.7f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, EtherMarshes,      0.10f, 0.0f, 1.0f, 0.0f,  0.55f)
		COM_ADD_WEIGHT(S, DreamMeadows,      0.12f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, AuroraField,       0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, CloudPlains,       0.25f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.10+0.15+0.15+0.08+0.10+0.12+0.05+0.25 = 1.00

		// Underdark: The Inverse (sum = 1.00)
		COM_ADD_W(UD, PhaseCavern,          0.30f)
		COM_ADD_W(UD, CrystalResonance,     0.20f)
		COM_ADD_W(UD, GravityWell,          0.15f)
		COM_ADD_W(UD, DreamGrotto,          0.15f)
		COM_ADD_W(UD, VoidBubble,           0.10f)
		COM_ADD_W(UD, MirrorCavern,         0.05f)
		COM_ADD_W(UD, StarlightPool,        0.05f)
		// sum = 1.00

		// Underwater: crystal mist-seas (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,           0.35f)
		COM_ADD_W(UW, CrystalGrotto_UW,    0.30f)
		COM_ADD_W(UW, CoralReef,            0.20f)
		COM_ADD_W(UW, Seamount,             0.15f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Abyssal — Demon realm: chaos, bone wastes, screaming chasms
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Abyssal:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, ScreamingChasms,  0.10f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, DemonPillars,     0.10f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, CarrionDesert,    0.10f, 0.7f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, GoreMarshes,      0.12f, 0.0f, 1.0f, 0.0f,  0.54f)
		COM_ADD_WEIGHT(S, LivingWalls,      0.08f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, BloodRivers,      0.10f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, FleshCitadels,    0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, VoidFissures,     0.10f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, BonePlains,       0.25f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.10+0.10+0.10+0.12+0.08+0.10+0.05+0.10+0.25 = 1.00

		// Underdark: The Endless Chasms (sum = 1.00)
		COM_ADD_W(UD, ChasmFloor,          0.30f)
		COM_ADD_W(UD, BoneLabyrinth,       0.20f)
		COM_ADD_W(UD, DemonNest,           0.20f)
		COM_ADD_W(UD, FleshCorridor,       0.15f)
		COM_ADD_W(UD, BloodPool_Aby,       0.10f)
		COM_ADD_W(UD, VoidPocket_Aby,      0.05f)
		// sum = 1.00

		// Underwater: blood seas (sum = 1.00)
		COM_ADD_W(UW, AbyssalPlain,        0.40f)
		COM_ADD_W(UW, DeepTrench,          0.30f)
		COM_ADD_W(UW, OceanFloor,          0.20f)
		COM_ADD_W(UW, ThermalVent,         0.10f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Ethereal — Spirit/memory realm: alien, phase-shifting, unknowable
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Ethereal:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, ThoughtStorms,         0.07f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, EchoRuins,             0.10f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, FadeZones,             0.08f, 0.7f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, VoidWhisperFields,     0.12f, 0.0f, 1.0f, 0.0f,  0.55f)
		COM_ADD_WEIGHT(S, SpiritArchipelagos,    0.10f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, DreamDeserts,          0.08f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, ReflectionMirrors,     0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, MemoryLandscape,       0.15f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, CrystallizedMemories,  0.25f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.07+0.10+0.08+0.12+0.10+0.08+0.05+0.15+0.25 = 1.00

		// Underdark: The Memory Deep (sum = 1.00)
		COM_ADD_W(UD, MemoryPool,          0.30f)
		COM_ADD_W(UD, EchoTunnel,          0.20f)
		COM_ADD_W(UD, PhaseLabyrinth,      0.20f)
		COM_ADD_W(UD, MistCavern,          0.15f)
		COM_ADD_W(UD, DreamChamber,        0.10f)
		COM_ADD_W(UD, SpiritWell,          0.05f)
		// sum = 1.00

		// Underwater: spirit seas (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,          0.35f)
		COM_ADD_W(UW, AbyssalPlain,        0.30f)
		COM_ADD_W(UW, CrystalGrotto_UW,   0.20f)
		COM_ADD_W(UW, Seamount,            0.15f)
		// sum = 1.00
		break;

	// ──────────────────────────────────────────────────────────────────────────
	// Feywild — Ever-shifting fey realm: glamour, eternal forests, mushroom rings
	// ──────────────────────────────────────────────────────────────────────────
	case ECoMPlane::Feywild:
		// Surface (sum = 1.00)
		COM_ADD_WEIGHT(S, BlossomPeaks,         0.10f, 0.0f, 1.0f, 0.82f, 1.0f)
		COM_ADD_WEIGHT(S, ThorncraftHollow,     0.10f, 0.0f, 1.0f, 0.70f, 0.82f)
		COM_ADD_WEIGHT(S, FeyWastes,            0.05f, 0.75f,1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, ShiftingGlades,       0.12f, 0.0f, 1.0f, 0.0f,  0.54f)
		COM_ADD_WEIGHT(S, EternalForest,        0.20f, 0.0f, 1.0f, 0.64f, 1.0f)
		COM_ADD_WEIGHT(S, SilverMistPlains,     0.12f, 0.0f, 1.0f, 0.0f,  0.58f)
		COM_ADD_WEIGHT(S, TwilightMeadows,      0.13f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, WildHuntGrounds,      0.05f, 0.0f, 1.0f, 0.0f,  1.0f)
		COM_ADD_WEIGHT(S, MushroomRings,        0.13f, 0.0f, 1.0f, 0.0f,  1.0f)
		// sum = 0.10+0.10+0.05+0.12+0.20+0.12+0.13+0.05+0.13 = 1.00

		// Underdark: The Roots of Wonder (sum = 1.00)
		COM_ADD_W(UD, FeyRootLair,         0.30f)
		COM_ADD_W(UD, MushroomCave_Fey,    0.20f)
		COM_ADD_W(UD, FeyUndermarket,      0.15f)
		COM_ADD_W(UD, CrystalSpring_Fey,  0.15f)
		COM_ADD_W(UD, AncientBurrow,       0.10f)
		COM_ADD_W(UD, GlowwormDen,         0.05f)
		COM_ADD_W(UD, CobwebHalls,         0.05f)
		// sum = 1.00

		// Underwater: mirror-lake beds (sum = 1.00)
		COM_ADD_W(UW, OceanFloor,          0.30f)
		COM_ADD_W(UW, CoralReef,           0.30f)
		COM_ADD_W(UW, KelpForest,          0.25f)
		COM_ADD_W(UW, CrystalGrotto_UW,   0.15f)
		// sum = 1.00
		break;

	default:
		// Should never occur; return an asset with a single wide-coverage Grassland entry.
		COM_ADD_W(S,  Grassland,  1.0f)
		COM_ADD_W(UD, CavernFloor, 1.0f)
		COM_ADD_W(UW, OceanFloor,  1.0f)
		break;
	}

	return A;
}

#undef COM_ADD_WEIGHT
#undef COM_ADD_W


#endif
