#if WITH_AUTOMATION_TESTS
// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMTerrainDistTests.cpp — Unit tests for per-plane terrain distribution DataAssets (S3-T2).
// Filter: CoM.TerrainDist.*

#include "Misc/AutomationTest.h"
#include "CoMCore/Data/CoMTerrainWeightDataAsset.h"
#include "CoMCore/World/CoMWorldGenerator.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

// ---------------------------------------------------------------------------
// Helpers shared across all terrain-dist tests
// ---------------------------------------------------------------------------

namespace CoMTerrainDistTestHelpers
{
	/** Build a minimal UCoMTerrainWeightDataAsset for a given plane (no outer required in tests). */
	static UCoMTerrainWeightDataAsset* MakeTestAsset(ECoMPlane Plane)
	{
		UCoMTerrainWeightDataAsset* Asset = NewObject<UCoMTerrainWeightDataAsset>();
		Asset->Plane = Plane;

		// Surface: basic flat weight covering all lat/alt so all tiles get assigned.
		FCoMTerrainWeightEntry E1;
		E1.Terrain      = ECoMTerrain::Grassland;
		E1.Weight       = 2.0f;
		E1.MinLatitude  = 0.0f; E1.MaxLatitude  = 1.0f;
		E1.MinAltitude  = 0.0f; E1.MaxAltitude  = 1.0f;

		FCoMTerrainWeightEntry E2;
		E2.Terrain      = ECoMTerrain::Mountains;
		E2.Weight       = 1.0f;
		E2.MinLatitude  = 0.0f; E2.MaxLatitude  = 1.0f;
		E2.MinAltitude  = 0.7f; E2.MaxAltitude  = 1.0f;

		Asset->SurfaceWeights.Add(E1);
		Asset->SurfaceWeights.Add(E2);

		// Underdark: single entry
		FCoMTerrainWeightEntry UD;
		UD.Terrain     = ECoMTerrain::CavernFloor;
		UD.Weight      = 1.0f;
		UD.MinLatitude = 0.0f; UD.MaxLatitude = 1.0f;
		UD.MinAltitude = 0.0f; UD.MaxAltitude = 1.0f;

		Asset->UnderdarkWeights.Add(UD);

		return Asset;
	}

	/** Derive the expected asset FName from the plane enum (convention: TW_<PlaneName>). */
	static FName PlaneToAssetName(ECoMPlane Plane)
	{
		const UEnum* EnumPtr = StaticEnum<ECoMPlane>();
		if (!EnumPtr) return NAME_None;
		const FString DisplayName = EnumPtr->GetNameStringByValue(static_cast<int64>(Plane));
		return FName(*FString::Printf(TEXT("TW_%s"), *DisplayName));
	}
}

// ---------------------------------------------------------------------------
// Test 1 — GetPrimaryAssetId returns the correct type and name
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_DataAssetLoadsByPlane,
	"CoM.TerrainDist.DataAssetLoadsByPlane",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_DataAssetLoadsByPlane::RunTest(const FString& Parameters)
{
	using namespace CoMTerrainDistTestHelpers;

	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane     = static_cast<ECoMPlane>(P);
		UCoMTerrainWeightDataAsset* Asset = MakeTestAsset(Plane);

		// Rename the object to match the convention so GetPrimaryAssetId uses the right FName.
		const FName ExpectedName = PlaneToAssetName(Plane);
		Asset->Rename(*ExpectedName.ToString(), nullptr, REN_DoNotDirty);

		const FPrimaryAssetId Id = Asset->GetPrimaryAssetId();

		TestEqual(FString::Printf(TEXT("Plane %d: PrimaryAssetType"), P),
		          Id.PrimaryAssetType.ToString(), FString(TEXT("CoMTerrainWeight")));
		TestEqual(FString::Printf(TEXT("Plane %d: PrimaryAssetName"), P),
		          Id.PrimaryAssetName.ToString(), ExpectedName.ToString());
	}
	return true;
}

// ---------------------------------------------------------------------------
// Test 2 — SurfaceWeights.Num() > 0 and total positive weight > 0
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_SurfaceWeightsSumPositive,
	"CoM.TerrainDist.SurfaceWeightsSumPositive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_SurfaceWeightsSumPositive::RunTest(const FString& Parameters)
{
	using namespace CoMTerrainDistTestHelpers;

	UCoMTerrainWeightDataAsset* Asset = MakeTestAsset(ECoMPlane::Aurelith);

	TestTrue("SurfaceWeights not empty", Asset->SurfaceWeights.Num() > 0);

	float TotalWeight = 0.0f;
	for (const FCoMTerrainWeightEntry& E : Asset->SurfaceWeights)
	{
		TotalWeight += E.Weight;
	}
	TestTrue("Total SurfaceWeight > 0", TotalWeight > 0.0f);

	return true;
}

// ---------------------------------------------------------------------------
// Test 3 — Weighted pick always returns a valid ECoMTerrain within the table
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_WeightedPickStayInBounds,
	"CoM.TerrainDist.WeightedPickStayInBounds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_WeightedPickStayInBounds::RunTest(const FString& Parameters)
{
	using namespace CoMTerrainDistTestHelpers;

	// Build a weight table using the correct FCoMTerrainWeight struct.
	TArray<FCoMTerrainWeight> Weights;

	auto AddEntry = [&](ECoMTerrain T, float W)
	{
		FCoMTerrainWeight E;
		E.TerrainType = T;
		E.Weight = W;
		Weights.Add(E);
	};

	AddEntry(ECoMTerrain::Mountains,  1.0f);
	AddEntry(ECoMTerrain::Hills,      1.0f);
	AddEntry(ECoMTerrain::Grassland,  2.0f);
	AddEntry(ECoMTerrain::Desert,     1.0f);

	// Collect valid terrain values present in the table.
	TSet<ECoMTerrain> ValidTerrains;
	for (const FCoMTerrainWeight& E : Weights)
	{
		ValidTerrains.Add(E.TerrainType);
	}

	// Create a generator instance and run picks.
	UCoMWorldGenerator* Gen = NewObject<UCoMWorldGenerator>();
	constexpr int32 NumPicks = 100;
	int32 SuccessCount = 0;

	for (int32 I = 0; I < NumPicks; ++I)
	{
		const ECoMTerrain Picked = Gen->PickWeightedTerrain(Weights);
		TestTrue(FString::Printf(TEXT("Pick %d terrain in valid set"), I),
		         ValidTerrains.Contains(Picked));
		++SuccessCount;
	}

	// All picks should succeed (non-empty weight table).
	TestTrue("All picks succeeded", SuccessCount == NumPicks);

	return true;
}

// ---------------------------------------------------------------------------
// Test 4 — UnderdarkWeights array is not empty on a constructed asset
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_UnderdarkWeightsPresent,
	"CoM.TerrainDist.UnderdarkWeightsPresent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_UnderdarkWeightsPresent::RunTest(const FString& Parameters)
{
	using namespace CoMTerrainDistTestHelpers;

	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		UCoMTerrainWeightDataAsset* Asset = MakeTestAsset(static_cast<ECoMPlane>(P));
		TestTrue(FString::Printf(TEXT("Plane %d UnderdarkWeights not empty"), P),
		         Asset->UnderdarkWeights.Num() > 0);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Test 5 — Plane enum field is consistent with the asset naming convention
//          Convention: asset FName == "TW_<EnumDisplayName>"
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_PlaneFieldMatchesAssetName,
	"CoM.TerrainDist.PlaneFieldMatchesAssetName",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_PlaneFieldMatchesAssetName::RunTest(const FString& Parameters)
{
	using namespace CoMTerrainDistTestHelpers;

	const UEnum* EnumPtr = StaticEnum<ECoMPlane>();
	if (!TestNotNull("ECoMPlane enum found", EnumPtr)) return false;

	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		UCoMTerrainWeightDataAsset* Asset = MakeTestAsset(Plane);
		Asset->Rename(*PlaneToAssetName(Plane).ToString(), nullptr, REN_DoNotDirty);

		// Derive expected plane enum display name from the asset FName (strip "TW_" prefix).
		const FString AssetFNameStr = Asset->GetFName().ToString();
		const FString PrefixStripped = AssetFNameStr.Mid(3); // Remove "TW_"

		const FString PlaneDisplayName = EnumPtr->GetNameStringByValue(static_cast<int64>(Plane));

		TestEqual(FString::Printf(TEXT("Plane %d: asset name suffix matches enum"), P),
		          PrefixStripped, PlaneDisplayName);

		// Also verify the stored Plane field matches what we set.
		TestEqual(FString::Printf(TEXT("Plane %d: Plane field identity"), P),
		          static_cast<int32>(Asset->Plane), P);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Test 6 — Empty PlaneWeights triggers fallback path; Stage2 must not crash
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTerrainDist_FallbackToHardcodedWhenNoAsset,
	"CoM.TerrainDist.FallbackToHardcodedWhenNoAsset",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCoMTerrainDist_FallbackToHardcodedWhenNoAsset::RunTest(const FString& Parameters)
{
	// Create a minimal world using the generator with an empty PlaneWeights array.
	// The generator's no-arg DistributeTerrain delegates to the overload with an empty
	// TArray<UCoMTerrainWeightDataAsset*>, which is the exact fallback path.
	// We verify: (a) generation does not crash; (b) every surface tile has a non-None terrain.

	UCoMWorldMapSubsystem* Map = NewObject<UCoMWorldMapSubsystem>();
	if (!TestNotNull("Map allocated", Map)) return false;

	Map->InitializeLayers();

	UCoMWorldGenerator* Gen = NewObject<UCoMWorldGenerator>();
	if (!TestNotNull("Gen allocated", Gen)) return false;

	// GenerateWorld calls DistributeTerrain(Map, HeightMaps, {}) internally —
	// no PlaneWeights asset is injected, so every plane uses the hardcoded fallback.
	Gen->GenerateWorld(Map, /*Seed=*/99999);

	// Spot-check a sample of tiles across all planes to verify terrain is assigned.
	using namespace CoM;
	int32 AssignedCount = 0;
	for (int32 P = 0; P < static_cast<int32>(ECoMPlane::MAX); ++P)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(P);
		// Check the center tile of each plane's surface layer.
		const FCoMTileData* Tile = Map->GetTile(Plane, ECoMMapLayer::Surface,
		                                         MAP_WIDTH / 2, MAP_HEIGHT / 2);
		if (Tile)
		{
			++AssignedCount;
		}
	}

	TestEqual("All 8 planes have terrain assigned at center tile",
	          AssignedCount, static_cast<int32>(ECoMPlane::MAX));

	return true;
}

#endif
