#if WITH_AUTOMATION_TESTS
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "CoMCore/World/CoMLeyPortalSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMWorldGenerator.h"

// ---------------------------------------------------------------------------
// Shared helper: stand up a fully-generated world + ley/portal layer
// ---------------------------------------------------------------------------

namespace CoMLeyPortalTestHelpers
{
	struct FTestWorld
	{
		UCoMWorldMapSubsystem* Map   = nullptr;
		UCoMWorldGenerator*    Gen   = nullptr;
		UCoMLeyPortalSubsystem* LPS  = nullptr;

		FTestWorld()
		{
			Map = NewObject<UCoMWorldMapSubsystem>();
			Gen = NewObject<UCoMWorldGenerator>();
			LPS = NewObject<UCoMLeyPortalSubsystem>();

			Map->InitializeLayers();
			Gen->GenerateWorld(Map, 42);

			FRandomStream Rng(42);
			LPS->GenerateLeyLines(Map, Rng);
			LPS->GeneratePortals(Map, Rng);
		}
	};
}

// ---------------------------------------------------------------------------
// 1. Each plane has 15-25 ley lines
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMLeyLineCountTest,
	"CoM.LeyPortal.LeyLineCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMLeyLineCountTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMLeyLine*> Lines = World.LPS->GetLeyLinesOnPlane(static_cast<ECoMPlane>(Plane));
		const int32 Count = Lines.Num();

		TestTrue(
			FString::Printf(TEXT("Plane %d has >= 15 ley lines (got %d)"), Plane, Count),
			Count >= 15);
		TestTrue(
			FString::Printf(TEXT("Plane %d has <= 25 ley lines (got %d)"), Plane, Count),
			Count <= 25);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 2. No ley line path tile should be an Ocean tile
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMLeyLinesAvoidOceanTest,
	"CoM.LeyPortal.LeyLinesAvoidOcean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMLeyLinesAvoidOceanTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMLeyLine*> Lines = World.LPS->GetLeyLinesOnPlane(static_cast<ECoMPlane>(Plane));

		for (const FCoMLeyLine* Line : Lines)
		{
			for (const FIntPoint& Tile : Line->Path)
			{
				const FCoMTileData* TD = World.Map->GetTile(static_cast<ECoMPlane>(Plane), ECoMMapLayer::Surface, Tile.X, Tile.Y);
				if (!TD) continue;

				TestFalse(
					FString::Printf(
						TEXT("LeyLine %d on plane %d: tile (%d,%d) should not be Ocean"),
						Line->LeyLineID, Plane, Tile.X, Tile.Y),
					TD->Terrain == ECoMTerrain::Ocean);
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 3. Each plane has 3-15 intersections
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMIntersectionCountTest,
	"CoM.LeyPortal.IntersectionCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMIntersectionCountTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMLeyIntersection*> Intersections =
			World.LPS->GetIntersectionsOnPlane(static_cast<ECoMPlane>(Plane));
		const int32 Count = Intersections.Num();

		TestTrue(
			FString::Printf(TEXT("Plane %d has >= 3 intersections (got %d)"), Plane, Count),
			Count >= 3);
		TestTrue(
			FString::Printf(TEXT("Plane %d has <= 15 intersections (got %d)"), Plane, Count),
			Count <= 15);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 4. Intersections with 3+ converging lines have bHasNexus=true
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMIntersectionsAreNexusTest,
	"CoM.LeyPortal.IntersectionsAreNexus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMIntersectionsAreNexusTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMLeyIntersection*> Intersections =
			World.LPS->GetIntersectionsOnPlane(static_cast<ECoMPlane>(Plane));

		for (const FCoMLeyIntersection* IX : Intersections)
		{
			if (IX->ConvergingLeyLineIDs.Num() >= 3)
			{
				TestTrue(
					FString::Printf(
						TEXT("Intersection %d on plane %d with %d converging lines should be a nexus"),
						IX->IntersectionID, Plane, IX->ConvergingLeyLineIDs.Num()),
					IX->bHasNexus);
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 5. Tiles in ley line paths have LeyLineIDs populated
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMTilesMarkedWithLeyLineIDsTest,
	"CoM.LeyPortal.TilesMarkedWithLeyLineIDs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMTilesMarkedWithLeyLineIDsTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMLeyLine*> Lines = World.LPS->GetLeyLinesOnPlane(static_cast<ECoMPlane>(Plane));

		for (const FCoMLeyLine* Line : Lines)
		{
			for (const FIntPoint& Tile : Line->Path)
			{
				const FCoMTileData* TD = World.Map->GetTile(static_cast<ECoMPlane>(Plane), ECoMMapLayer::Surface, Tile.X, Tile.Y);
				if (!TD) continue;

				TestTrue(
					FString::Printf(
						TEXT("Tile (%d,%d) on plane %d should list LeyLineID %d"),
						Tile.X, Tile.Y, Plane, Line->LeyLineID),
					TD->LeyLineIDs.Contains(Line->LeyLineID));
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 6. Each plane has at least 2 inter-plane portals (NaturalGateway or PlanarRift)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMInterPlanePortalsTest,
	"CoM.LeyPortal.InterPlanePortals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMInterPlanePortalsTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMPortal*> Portals = World.LPS->GetPortalsOnPlane(static_cast<ECoMPlane>(Plane));

		int32 InterPlaneCount = 0;
		for (const FCoMPortal* Portal : Portals)
		{
			const bool bIsInterPlane =
				Portal->SourcePlane != Portal->DestPlane &&
				(Portal->Type == ECoMPortalType::NaturalGateway ||
				 Portal->Type == ECoMPortalType::PlanarRift);

			if (bIsInterPlane)
			{
				++InterPlaneCount;
			}
		}

		TestTrue(
			FString::Printf(
				TEXT("Plane %d should have >= 2 inter-plane portals (got %d)"),
				Plane, InterPlaneCount),
			InterPlaneCount >= 2);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 7. All natural portals have bBidirectional=true
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMPortalsBidirectionalTest,
	"CoM.LeyPortal.PortalsBidirectional",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMPortalsBidirectionalTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMPortal*> Portals = World.LPS->GetPortalsOnPlane(static_cast<ECoMPlane>(Plane));

		for (const FCoMPortal* Portal : Portals)
		{
			if (Portal->Type == ECoMPortalType::NaturalGateway)
			{
				TestTrue(
					FString::Printf(
						TEXT("NaturalGateway portal %d on plane %d should be bidirectional"),
						Portal->PortalID, Plane),
					Portal->bBidirectional);
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 8. Portal source/dest tiles have PortalID set on the tile data
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoMPortalTilesMarkedTest,
	"CoM.LeyPortal.PortalTilesMarked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoMPortalTilesMarkedTest::RunTest(const FString& Parameters)
{
	CoMLeyPortalTestHelpers::FTestWorld World;

	for (int32 Plane = 0; Plane < static_cast<int32>(ECoMPlane::MAX); ++Plane)
	{
		const TArray<const FCoMPortal*> Portals = World.LPS->GetPortalsOnPlane(static_cast<ECoMPlane>(Plane));

		for (const FCoMPortal* Portal : Portals)
		{
			// Check source tile
			{
				const FCoMTileData* SrcTile = World.Map->GetTile(
					static_cast<ECoMPlane>(Portal->SourcePlane), ECoMMapLayer::Surface,
					Portal->SourcePosition.X,
					Portal->SourcePosition.Y);

				if (SrcTile)
				{
					TestEqual(
						FString::Printf(
							TEXT("Source tile (%d,%d) on plane %d should have PortalID %d"),
							Portal->SourcePosition.X, Portal->SourcePosition.Y,
							Portal->SourcePlane, Portal->PortalID),
						SrcTile->PortalID,
						Portal->PortalID);
				}
			}

			// Check destination tile
			{
				const FCoMTileData* DstTile = World.Map->GetTile(
					static_cast<ECoMPlane>(Portal->DestPlane), ECoMMapLayer::Surface,
					Portal->DestPosition.X,
					Portal->DestPosition.Y);

				if (DstTile)
				{
					TestEqual(
						FString::Printf(
							TEXT("Dest tile (%d,%d) on plane %d should have PortalID %d"),
							Portal->DestPosition.X, Portal->DestPosition.Y,
							Portal->DestPlane, Portal->PortalID),
						DstTile->PortalID,
						Portal->PortalID);
				}
			}
		}
	}

	return true;
}

#endif
