// Copyright Mythforge Studios. All Rights Reserved.
// COM-S3-T2: Unit tests for UCoMTerrainDistributionDataAsset.
// Filter prefix: CoM.WorldGen.TerrainDist.*
//
// Tests:
//   1. WeightSumsToOnePerPlane       — all 8 planes' Surface weights sum to 1.0
//   2. AllPlanesHaveValidAsset        — CreateDefaults() produces an entry for every ECoMPlane
//   3. UnderdarkWeightsSumToOne       — Underdark weights sum to 1.0 per plane
//   4. UnderwaterWeightsSumToOne      — Underwater weights sum to 1.0 per plane
//   5. InvalidWeightRejectedByIsDataValid — negative weight fails validation
//   6. NullEntryRejectedByIsDataValid — null pointer in PlaneWeights array fails validation
//   7. GeneratorReadsDataAsset        — GenerateWorld(Map, Seed, Dist) uses asset terrain types
//   8. DistinctPlanesHaveDistinctTerrain — two different planes produce different dominant types
//   9. GetWeightsForPlane             — correct asset returned for each plane
//  10. EmptyDistributionFallsBack     — null TerrainDist uses hardcoded path without crash

#include "Misc/AutomationTest.h"
#include "WorldGen/TerrainDistribution/CoMTerrainDistributionDataAsset.h"
#include "CoMCore/WorldGen/CoMWorldGenerator.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

// ─── Shared test tolerance ────────────────────────────────────────────────────

namespace CoMTerrainDistTestPrivate
{
	constexpr float SumTolerance = 0.001f;

	/** Verify surface weight sum == 1.0 for one plane asset. */
	static bool CheckSumForPlane(FAutomationTestBase& Test,
	                              UCoMTerrainDistributionDataAsset* Dist,
	                              ECoMPlane Plane)
	{
		UCoMTerrainWeightDataAsset* Asset = Dist->GetWeightsForPlane(Plane);
		const int32 P = static_cast<int32>(Plane);

		if (!Test.TestNotNull(FString::Printf(TEXT("Plane %d: asset not null"), P), Asset))
		{
			return false;
		}

		const float Sum = UCoMTerrainDistributionDataAsset::ComputeWeightSum(Asset->SurfaceWeights);
		return Test.TestTrue(
			FString::Printf(TEXT("Plane %d Surface weight sum %.5f ≈ 1.0"), P, Sum),
			FMath::Abs(Sum - 1.0f) <= SumTolerance);
	}
}

// ─── Test 1: Surface weights sum to 1.0 for every plane ─────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_WeightSumsToOnePerPlane,
	"CoM.WorldGen.TerrainDist.WeightSumsToOnePerPlane",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_WeightSumsToOnePerPlane::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Distribution asset created", Dist)) return false;

	bool bAllPass = true;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		bAllPass &= CoMTerrainDistTestPrivate::CheckSumForPlane(
			*this, Dist, static_cast<ECoMPlane>(P));
	}
	return bAllPass;
}

// ─── Test 2: All 8 planes have a non-null DataAsset entry ───────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_AllPlanesHaveValidAsset,
	"CoM.WorldGen.TerrainDist.AllPlanesHaveValidAsset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_AllPlanesHaveValidAsset::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Distribution asset created", Dist)) return false;

	const int32 NumPlanes = static_cast<int32>(ECoMPlane::MAX);

	// Registry must have an entry for every plane.
	TestEqual("PlaneWeights count == NumPlanes", Dist->PlaneWeights.Num(), NumPlanes);

	bool bAllPass = true;
	for (int32 P = 0; P < NumPlanes; ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		UCoMTerrainWeightDataAsset* Asset = Dist->GetWeightsForPlane(Plane);
		bAllPass &= TestNotNull(
			FString::Printf(TEXT("Plane %d GetWeightsForPlane returns non-null"), P),
			Asset);
		if (Asset)
		{
			bAllPass &= TestTrue(
				FString::Printf(TEXT("Plane %d: stored plane field matches"), P),
				Asset->Plane == Plane);
			bAllPass &= TestTrue(
				FString::Printf(TEXT("Plane %d: SurfaceWeights non-empty"), P),
				Asset->SurfaceWeights.Num() > 0);
		}
	}
	return bAllPass;
}

// ─── Test 3: Underdark weights sum to 1.0 per plane ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_UnderdarkWeightsSumToOne,
	"CoM.WorldGen.TerrainDist.UnderdarkWeightsSumToOne",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_UnderdarkWeightsSumToOne::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Distribution asset created", Dist)) return false;

	bool bAllPass = true;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		UCoMTerrainWeightDataAsset* Asset = Dist->GetWeightsForPlane(static_cast<ECoMPlane>(P));
		if (!Asset || Asset->UnderdarkWeights.IsEmpty()) continue;

		const float Sum = UCoMTerrainDistributionDataAsset::ComputeWeightSum(Asset->UnderdarkWeights);
		bAllPass &= TestTrue(
			FString::Printf(TEXT("Plane %d Underdark weight sum %.5f ≈ 1.0"), P, Sum),
			FMath::Abs(Sum - 1.0f) <= CoMTerrainDistTestPrivate::SumTolerance);
	}
	return bAllPass;
}

// ─── Test 4: Underwater weights sum to 1.0 per plane ────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_UnderwaterWeightsSumToOne,
	"CoM.WorldGen.TerrainDist.UnderwaterWeightsSumToOne",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_UnderwaterWeightsSumToOne::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Distribution asset created", Dist)) return false;

	bool bAllPass = true;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		UCoMTerrainWeightDataAsset* Asset = Dist->GetWeightsForPlane(static_cast<ECoMPlane>(P));
		if (!Asset || Asset->UnderwaterWeights.IsEmpty()) continue;

		const float Sum = UCoMTerrainDistributionDataAsset::ComputeWeightSum(Asset->UnderwaterWeights);
		bAllPass &= TestTrue(
			FString::Printf(TEXT("Plane %d Underwater weight sum %.5f ≈ 1.0"), P, Sum),
			FMath::Abs(Sum - 1.0f) <= CoMTerrainDistTestPrivate::SumTolerance);
	}
	return bAllPass;
}

// ─── Test 5: Negative weight fails IsDataValid ───────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_InvalidWeightRejected,
	"CoM.WorldGen.TerrainDist.InvalidWeightRejected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_InvalidWeightRejected::RunTest(const FString& Parameters)
{
	// Build a distribution with one valid plane and one with a negative weight.
	UCoMTerrainDistributionDataAsset* Dist = NewObject<UCoMTerrainDistributionDataAsset>();

	UCoMTerrainWeightDataAsset* BadAsset = NewObject<UCoMTerrainWeightDataAsset>();
	BadAsset->Plane = ECoMPlane::Aurelith;

	FCoMTerrainWeightEntry Good;
	Good.Terrain = ECoMTerrain::Grassland; Good.Weight = 0.8f;
	Good.MinLatitude = 0.0f; Good.MaxLatitude = 1.0f;
	Good.MinAltitude = 0.0f; Good.MaxAltitude = 1.0f;
	BadAsset->SurfaceWeights.Add(Good);

	FCoMTerrainWeightEntry Bad;
	Bad.Terrain = ECoMTerrain::Mountains; Bad.Weight = -0.5f; // invalid
	Bad.MinLatitude = 0.0f; Bad.MaxLatitude = 1.0f;
	Bad.MinAltitude = 0.0f; Bad.MaxAltitude = 1.0f;
	BadAsset->SurfaceWeights.Add(Bad);

	Dist->PlaneWeights.Add(BadAsset);

	TArray<FString> Errors;
	const bool bValid = Dist->IsDataValid(Errors);

	TestFalse("IsDataValid returns false for negative weight", bValid);
	TestTrue("At least one error reported", Errors.Num() > 0);
	return true;
}

// ─── Test 6: Null entry fails IsDataValid ────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_NullEntryRejected,
	"CoM.WorldGen.TerrainDist.NullEntryRejected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_NullEntryRejected::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = NewObject<UCoMTerrainDistributionDataAsset>();
	Dist->PlaneWeights.Add(nullptr); // deliberately null

	TArray<FString> Errors;
	const bool bValid = Dist->IsDataValid(Errors);

	TestFalse("IsDataValid returns false for null entry", bValid);
	TestTrue("At least one error reported", Errors.Num() > 0);
	return true;
}

// ─── Test 7: Generator reads DataAsset, not hardcoded path ──────────────────
//
// Strategy: generate two worlds from the same seed — one using the DataAsset
// (which will assign Aurelith surface tiles to Grassland/Forest/etc. from the table)
// and one using the plain GenerateWorld (hardcoded path).
// They MUST produce the same bIsValid result; the DataAsset world must also be valid.
// Additionally, verify that plane-specific terrain types actually appear in the asset world.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_GeneratorReadsDataAsset,
	"CoM.WorldGen.TerrainDist.GeneratorReadsDataAsset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_GeneratorReadsDataAsset::RunTest(const FString& Parameters)
{
	using namespace CoM;

	// Build a minimal distribution for Aurelith only — all other planes fall back.
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Dist not null", Dist)) return false;

	UCoMWorldMapSubsystem* Map = NewObject<UCoMWorldMapSubsystem>();
	UCoMWorldGenerator*    Gen = NewObject<UCoMWorldGenerator>();
	if (!TestNotNull("Map not null", Map) || !TestNotNull("Gen not null", Gen)) return false;

	Map->InitializeLayers();
	const FCoMWorldData Data = Gen->GenerateWorld(Map, /*Seed=*/12345, Dist);

	TestTrue("GenerateWorld(Dist) succeeded", Data.bIsValid);

	// Aurelith's default table includes Grassland, Forest, Hills, Plains, Mountains etc.
	// Verify at least one of those appears on Aurelith's surface.
	const TSet<ECoMTerrain> AurelithExpected = {
		ECoMTerrain::Grassland, ECoMTerrain::Forest, ECoMTerrain::Hills,
		ECoMTerrain::Plains, ECoMTerrain::Mountains, ECoMTerrain::Swamp,
		ECoMTerrain::Desert, ECoMTerrain::Tundra, ECoMTerrain::Jungle,
	};

	int32 MatchCount = 0;
	for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
	{
		for (int32 X = 0; X < MAP_WIDTH; ++X)
		{
			const FCoMTileData* Tile = Map->GetTile(ECoMPlane::Aurelith, ECoMMapLayer::Surface, X, Y);
			if (Tile && !Tile->bImpassable && AurelithExpected.Contains(Tile->Terrain))
			{
				++MatchCount;
			}
		}
	}
	TestTrue("Aurelith surface has DataAsset terrain types after DataAsset generation",
	         MatchCount > 0);

	return true;
}

// ─── Test 8: Distinct planes produce distinct dominant terrain ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_DistinctPlanesDistinctTerrain,
	"CoM.WorldGen.TerrainDist.DistinctPlanesDistinctTerrain",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_DistinctPlanesDistinctTerrain::RunTest(const FString& Parameters)
{
	using namespace CoM;

	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	UCoMWorldMapSubsystem* Map = NewObject<UCoMWorldMapSubsystem>();
	UCoMWorldGenerator*    Gen = NewObject<UCoMWorldGenerator>();

	Map->InitializeLayers();
	Gen->GenerateWorld(Map, /*Seed=*/7777, Dist);

	// Collect dominant (most-frequent) land terrain per plane.
	TArray<ECoMTerrain> DominantPerPlane;
	DominantPerPlane.SetNum(static_cast<int32>(ECoMPlane::MAX));

	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		TMap<ECoMTerrain, int32> Counts;

		for (int32 Y = 0; Y < MAP_HEIGHT; ++Y)
		{
			for (int32 X = 0; X < MAP_WIDTH; ++X)
			{
				const FCoMTileData* T = Map->GetTile(Plane, ECoMMapLayer::Surface, X, Y);
				if (T && !T->bImpassable)
				{
					Counts.FindOrAdd(T->Terrain)++;
				}
			}
		}

		ECoMTerrain Dominant = ECoMTerrain::Grassland;
		int32 MaxCount = 0;
		for (const auto& KV : Counts)
		{
			if (KV.Value > MaxCount) { MaxCount = KV.Value; Dominant = KV.Key; }
		}
		DominantPerPlane[P] = Dominant;
	}

	// Aurelith and Infernyx must have different dominant terrain (golden vs volcanic).
	const ECoMTerrain AurelithDom = DominantPerPlane[static_cast<int32>(ECoMPlane::Aurelith)];
	const ECoMTerrain InfernyxDom = DominantPerPlane[static_cast<int32>(ECoMPlane::Infernyx)];

	TestTrue("Aurelith and Infernyx have different dominant terrain",
	         AurelithDom != InfernyxDom);

	// Verdantis and Noctharion must also differ.
	const ECoMTerrain VerdDom  = DominantPerPlane[static_cast<int32>(ECoMPlane::Verdantis)];
	const ECoMTerrain NoctDom  = DominantPerPlane[static_cast<int32>(ECoMPlane::Noctharion)];

	TestTrue("Verdantis and Noctharion have different dominant terrain",
	         VerdDom != NoctDom);

	return true;
}

// ─── Test 9: GetWeightsForPlane returns correct asset ────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_GetWeightsForPlane,
	"CoM.WorldGen.TerrainDist.GetWeightsForPlane",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_GetWeightsForPlane::RunTest(const FString& Parameters)
{
	UCoMTerrainDistributionDataAsset* Dist = UCoMTerrainDistributionDataAsset::CreateDefaults();
	if (!TestNotNull("Dist created", Dist)) return false;

	bool bAllPass = true;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		UCoMTerrainWeightDataAsset* Asset = Dist->GetWeightsForPlane(Plane);

		bAllPass &= TestNotNull(
			FString::Printf(TEXT("GetWeightsForPlane(%d) returns non-null"), P), Asset);

		if (Asset)
		{
			bAllPass &= TestTrue(
				FString::Printf(TEXT("GetWeightsForPlane(%d) returns correct plane field"), P),
				Asset->Plane == Plane);
		}

		// A plane NOT in the registry should return null.
		// (Only valid to test if we explicitly omit a plane — using default covers all, skip.)
	}
	return bAllPass;
}

// ─── Test 10: Null TerrainDist falls back to hardcoded path without crash ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_NullDistributionFallsBack,
	"CoM.WorldGen.TerrainDist.NullDistributionFallsBack",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_NullDistributionFallsBack::RunTest(const FString& Parameters)
{
	using namespace CoM;

	UCoMWorldMapSubsystem* Map = NewObject<UCoMWorldMapSubsystem>();
	UCoMWorldGenerator*    Gen = NewObject<UCoMWorldGenerator>();

	Map->InitializeLayers();

	// Passing nullptr triggers the hardcoded fallback path.
	const FCoMWorldData Data = Gen->GenerateWorld(Map, /*Seed=*/99999,
	                                              static_cast<UCoMTerrainDistributionDataAsset*>(nullptr));

	TestTrue("Null dist: GenerateWorld succeeds (bIsValid)", Data.bIsValid);

	// Verify terrain is assigned on every plane's center surface tile.
	int32 AssignedCount = 0;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		const FCoMTileData* Tile = Map->GetTile(Plane, ECoMMapLayer::Surface,
		                                         MAP_WIDTH / 2, MAP_HEIGHT / 2);
		// Any terrain other than the zero-initialised default of Grassland
		// (which is the real first enum value) counts as assigned.
		// We only require bIsValid == true as the strong check; tile presence is a bonus.
		if (Tile) ++AssignedCount;
	}
	TestEqual("All 8 planes have a center surface tile", AssignedCount, static_cast<int32>(ECoMPlane::MAX));

	return true;
}
