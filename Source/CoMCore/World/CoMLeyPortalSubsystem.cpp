// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMLeyPortalSubsystem.h"
#include "CoMWorldMapSubsystem.h"

// ============================================================================
// Subsystem lifecycle
// ============================================================================

void UCoMLeyPortalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NextLeyLineID = 1;
	NextIntersectionID = 1;
	NextPortalID = 1;
}

void UCoMLeyPortalSubsystem::Deinitialize()
{
	AllLeyLines.Empty();
	AllIntersections.Empty();
	AllPortals.Empty();
	Super::Deinitialize();
}

// ============================================================================
// Ley line generation
// ============================================================================

void UCoMLeyPortalSubsystem::GenerateLeyLines(UCoMWorldMapSubsystem* Map,
											   FRandomStream& Rng)
{
	if (!Map)
	{
		return;
	}

	for (int32 PlaneIdx = 0; PlaneIdx < CoM::NUM_PLANES; ++PlaneIdx)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(PlaneIdx);

		// 1. Generate 20-30 random node positions on Surface, avoiding ocean.
		const int32 NodeCount = Rng.RandRange(20, 30);
		TArray<FIntPoint> Nodes = GenerateNodes(Map, Plane, NodeCount, Rng);
		if (Nodes.Num() < 4)
		{
			continue; // not enough valid land tiles
		}

		// 2. Lloyd relaxation (3 rounds).
		LloydRelax(Nodes, Map, Plane, 3);

		// 3. Build Delaunay-like edges (each node connects to 2-3 nearest).
		TArray<TPair<int32, int32>> Edges = BuildEdges(Nodes, 3);

		// Clamp to 15-25 ley lines per plane.
		const int32 DesiredMin = 15;
		const int32 DesiredMax = 25;
		if (Edges.Num() > DesiredMax)
		{
			// Shuffle and trim to DesiredMax.
			for (int32 i = Edges.Num() - 1; i > 0; --i)
			{
				const int32 j = Rng.RandRange(0, i);
				Edges.Swap(i, j);
			}
			Edges.SetNum(DesiredMax);
		}

		if (Edges.Num() < DesiredMin)
		{
			// Accept what we have; sparse planes get fewer lines.
		}

		// 4. For each edge, create a ley line.
		for (const TPair<int32, int32>& Edge : Edges)
		{
			const FIntPoint& From = Nodes[Edge.Key];
			const FIntPoint& To = Nodes[Edge.Value];

			TArray<FIntPoint> Path = TracePath(From, To, Map, Plane);
			if (Path.Num() < 2)
			{
				continue;
			}

			FCoMLeyLine Line;
			Line.LeyLineID = NextLeyLineID++;
			Line.Plane = Plane;
			Line.Layer = ECoMLayer::Surface;
			Line.Alignment = RandomAlignment(Rng);
			Line.Path = MoveTemp(Path);
			Line.PowerLevel = FFixed64(Line.Path.Num());
			Line.SourceNodeID = Edge.Key;
			Line.DestNodeID = Edge.Value;
			Line.bSevered = false;
			Line.ControllingWizard = INDEX_NONE;

			// 8. Mark each tile with this ley line ID.
			for (const FIntPoint& Pt : Line.Path)
			{
				FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMLayer::Surface,
														 Pt.X, Pt.Y);
				if (Tile)
				{
					Tile->LeyLineIDs.AddUnique(Line.LeyLineID);
				}
			}

			AllLeyLines.Add(MoveTemp(Line));
		}

		// 5. Detect intersections (tiles shared by 3+ lines).
		DetectIntersections(Plane);
	}
}

TArray<FIntPoint> UCoMLeyPortalSubsystem::GenerateNodes(
	UCoMWorldMapSubsystem* Map, ECoMPlane Plane, int32 Count,
	FRandomStream& Rng) const
{
	TArray<FIntPoint> Nodes;
	Nodes.Reserve(Count);

	const int32 MaxAttempts = Count * 20;
	int32 Attempts = 0;

	while (Nodes.Num() < Count && Attempts < MaxAttempts)
	{
		++Attempts;
		const int32 X = Rng.RandRange(0, CoM::MAP_WIDTH - 1);
		const int32 Y = Rng.RandRange(0, CoM::MAP_HEIGHT - 1);

		if (!IsTilePassable(Map, Plane, X, Y))
		{
			continue;
		}

		// Enforce minimum spacing of 6 tiles between nodes.
		bool bTooClose = false;
		for (const FIntPoint& Existing : Nodes)
		{
			if (WrappedDist(FIntPoint(X, Y), Existing) < 6)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			Nodes.Emplace(X, Y);
		}
	}

	return Nodes;
}

void UCoMLeyPortalSubsystem::LloydRelax(TArray<FIntPoint>& Nodes,
										 UCoMWorldMapSubsystem* Map,
										 ECoMPlane Plane,
										 int32 Iterations) const
{
	for (int32 Iter = 0; Iter < Iterations; ++Iter)
	{
		// Assign every land tile to the nearest node (Voronoi partition).
		// Then compute the centroid of each cell.
		TArray<FVector2D> Centroids;
		TArray<int32> CellCounts;
		Centroids.SetNumZeroed(Nodes.Num());
		CellCounts.SetNumZeroed(Nodes.Num());

		// Sample a grid subset to keep cost manageable.
		const int32 Step = 4;
		for (int32 Y = 0; Y < CoM::MAP_HEIGHT; Y += Step)
		{
			for (int32 X = 0; X < CoM::MAP_WIDTH; X += Step)
			{
				if (!IsTilePassable(Map, Plane, X, Y))
				{
					continue;
				}

				const FIntPoint Pt(X, Y);
				int32 BestIdx = 0;
				int32 BestDist = WrappedDist(Pt, Nodes[0]);

				for (int32 i = 1; i < Nodes.Num(); ++i)
				{
					const int32 D = WrappedDist(Pt, Nodes[i]);
					if (D < BestDist)
					{
						BestDist = D;
						BestIdx = i;
					}
				}

				Centroids[BestIdx].X += static_cast<double>(X);
				Centroids[BestIdx].Y += static_cast<double>(Y);
				CellCounts[BestIdx] += 1;
			}
		}

		// Move nodes toward centroids, snapping to passable tiles.
		for (int32 i = 0; i < Nodes.Num(); ++i)
		{
			if (CellCounts[i] == 0)
			{
				continue;
			}
			int32 NewX = FMath::RoundToInt(Centroids[i].X / CellCounts[i]);
			int32 NewY = FMath::RoundToInt(Centroids[i].Y / CellCounts[i]);
			NewX = WrapX(NewX);
			NewY = FMath::Clamp(NewY, 0, CoM::MAP_HEIGHT - 1);

			if (IsTilePassable(Map, Plane, NewX, NewY))
			{
				Nodes[i] = FIntPoint(NewX, NewY);
			}
		}
	}
}

TArray<TPair<int32, int32>> UCoMLeyPortalSubsystem::BuildEdges(
	const TArray<FIntPoint>& Nodes, int32 MaxNeighbors) const
{
	TSet<uint64> EdgeSet; // canonical (min,max) pair packed into uint64
	TArray<TPair<int32, int32>> Edges;

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		// Collect distances to all other nodes.
		struct FNodeDist
		{
			int32 Index;
			int32 Dist;
		};

		TArray<FNodeDist> Distances;
		Distances.Reserve(Nodes.Num());

		for (int32 j = 0; j < Nodes.Num(); ++j)
		{
			if (i == j)
			{
				continue;
			}
			Distances.Add({j, WrappedDist(Nodes[i], Nodes[j])});
		}

		Distances.Sort([](const FNodeDist& A, const FNodeDist& B)
		{
			return A.Dist < B.Dist;
		});

		// Connect to nearest 2-3 neighbors.
		const int32 ConnectCount = FMath::Min(MaxNeighbors, Distances.Num());
		for (int32 k = 0; k < ConnectCount; ++k)
		{
			const int32 j = Distances[k].Index;
			const int32 Lo = FMath::Min(i, j);
			const int32 Hi = FMath::Max(i, j);
			const uint64 Key = (static_cast<uint64>(Lo) << 32) | static_cast<uint64>(Hi);

			if (!EdgeSet.Contains(Key))
			{
				EdgeSet.Add(Key);
				Edges.Add(TPair<int32, int32>(Lo, Hi));
			}
		}
	}

	return Edges;
}

TArray<FIntPoint> UCoMLeyPortalSubsystem::TracePath(
	FIntPoint From, FIntPoint To, UCoMWorldMapSubsystem* Map,
	ECoMPlane Plane) const
{
	TArray<FIntPoint> Path;

	// Bresenham line interpolation with WrapX awareness.
	// Pick the shorter X direction considering wrap.
	int32 DX = To.X - From.X;
	if (FMath::Abs(DX) > CoM::MAP_WIDTH / 2)
	{
		DX = (DX > 0) ? DX - CoM::MAP_WIDTH : DX + CoM::MAP_WIDTH;
	}
	const int32 DY = To.Y - From.Y;

	const int32 Steps = FMath::Max(FMath::Abs(DX), FMath::Abs(DY));
	if (Steps == 0)
	{
		Path.Add(From);
		return Path;
	}

	Path.Reserve(Steps + 1);

	for (int32 S = 0; S <= Steps; ++S)
	{
		const int32 X = WrapX(From.X + FMath::RoundToInt(static_cast<float>(DX * S) / Steps));
		const int32 Y = FMath::Clamp(
			From.Y + FMath::RoundToInt(static_cast<float>(DY * S) / Steps),
			0, CoM::MAP_HEIGHT - 1);

		// Skip ocean tiles (ley lines avoid ocean).
		if (IsTilePassable(Map, Plane, X, Y))
		{
			if (Path.Num() == 0 || Path.Last() != FIntPoint(X, Y))
			{
				Path.Emplace(X, Y);
			}
		}
	}

	return Path;
}

void UCoMLeyPortalSubsystem::DetectIntersections(ECoMPlane Plane)
{
	// Build a map of tile -> list of ley line IDs on this plane.
	TMap<FIntPoint, TArray<int32>> TileLeyLines;

	for (const FCoMLeyLine& Line : AllLeyLines)
	{
		if (Line.Plane != Plane)
		{
			continue;
		}
		for (const FIntPoint& Pt : Line.Path)
		{
			TileLeyLines.FindOrAdd(Pt).AddUnique(Line.LeyLineID);
		}
	}

	// Tiles with 3+ converging lines become intersections.
	for (const auto& Pair : TileLeyLines)
	{
		if (Pair.Value.Num() >= 3)
		{
			FCoMLeyIntersection Intersection;
			Intersection.IntersectionID = NextIntersectionID++;
			Intersection.Position = Pair.Key;
			Intersection.Plane = Plane;
			Intersection.Layer = ECoMLayer::Surface;
			Intersection.ConvergingLeyLineIDs = Pair.Value;
			Intersection.bHasNexus = true;
			Intersection.ControllingWizard = INDEX_NONE;

			// Combined power = sum of converging line power levels.
			FFixed64 TotalPower(0);
			for (int32 LLID : Pair.Value)
			{
				if (const FCoMLeyLine* LL = GetLeyLine(LLID))
				{
					TotalPower = TotalPower + LL->PowerLevel;
				}
			}
			Intersection.CombinedPower = TotalPower;

			AllIntersections.Add(MoveTemp(Intersection));
		}
	}
}

// ============================================================================
// Portal generation
// ============================================================================

void UCoMLeyPortalSubsystem::GeneratePortals(UCoMWorldMapSubsystem* Map,
											  FRandomStream& Rng)
{
	if (!Map)
	{
		return;
	}

	for (int32 PlaneIdx = 0; PlaneIdx < CoM::NUM_PLANES; ++PlaneIdx)
	{
		const ECoMPlane Plane = static_cast<ECoMPlane>(PlaneIdx);

		// ------------------------------------------------------------------
		// A. Inter-plane natural portals (3-6 per plane, near intersections)
		// ------------------------------------------------------------------
		{
			TArray<const FCoMLeyIntersection*> PlaneIntersections = GetIntersectionsOnPlane(Plane);
			const int32 DesiredCount = Rng.RandRange(3, 6);
			const int32 Count = FMath::Min(DesiredCount, PlaneIntersections.Num());

			// Shuffle intersections so we pick randomly.
			TArray<const FCoMLeyIntersection*> Shuffled = PlaneIntersections;
			for (int32 i = Shuffled.Num() - 1; i > 0; --i)
			{
				const int32 j = Rng.RandRange(0, i);
				Shuffled.Swap(i, j);
			}

			for (int32 i = 0; i < Count; ++i)
			{
				const FCoMLeyIntersection* Src = Shuffled[i];

				// Pick a random destination plane and one of its intersections.
				const ECoMPlane DestPlane = RandomOtherPlane(Plane, Rng);
				TArray<const FCoMLeyIntersection*> DestIntersections = GetIntersectionsOnPlane(DestPlane);
				if (DestIntersections.Num() == 0)
				{
					continue;
				}

				const FCoMLeyIntersection* Dst = DestIntersections[Rng.RandRange(0, DestIntersections.Num() - 1)];

				// Alternate between NaturalGateway and PlanarRift.
				const ECoMPortalType Type = (i % 2 == 0)
					? ECoMPortalType::NaturalGateway
					: ECoMPortalType::PlanarRift;

				CreatePortalPair(Map, Type,
								 Plane, ECoMLayer::Surface, Src->Position,
								 DestPlane, ECoMLayer::Surface, Dst->Position,
								 /*bBidirectional=*/true);
			}
		}

		// ------------------------------------------------------------------
		// B. Underdark entrances (8-15 per plane, near mountains)
		// ------------------------------------------------------------------
		{
			const int32 DesiredCount = Rng.RandRange(8, 15);
			TArray<FIntPoint> MountainTiles;
			MountainTiles.Reserve(256);

			for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
			{
				for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
				{
					if (IsMountainTile(Map, Plane, X, Y))
					{
						MountainTiles.Emplace(X, Y);
					}
				}
			}

			// Shuffle and pick up to DesiredCount.
			for (int32 i = MountainTiles.Num() - 1; i > 0; --i)
			{
				const int32 j = Rng.RandRange(0, i);
				MountainTiles.Swap(i, j);
			}

			const int32 Count = FMath::Min(DesiredCount, MountainTiles.Num());
			for (int32 i = 0; i < Count; ++i)
			{
				const FIntPoint& Pos = MountainTiles[i];

				// Surface -> Underdark at same position on same plane.
				CreatePortalPair(Map, ECoMPortalType::UnderdarkEntrance,
								 Plane, ECoMLayer::Surface, Pos,
								 Plane, ECoMLayer::Underdark, Pos,
								 /*bBidirectional=*/true);
			}
		}

		// ------------------------------------------------------------------
		// C. Underwater access (2-4 per plane, coastal tiles)
		// ------------------------------------------------------------------
		{
			const int32 DesiredCount = Rng.RandRange(2, 4);
			TArray<FIntPoint> CoastalTiles;
			CoastalTiles.Reserve(128);

			for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
			{
				for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
				{
					if (IsCoastalTile(Map, Plane, X, Y))
					{
						CoastalTiles.Emplace(X, Y);
					}
				}
			}

			// Shuffle and pick.
			for (int32 i = CoastalTiles.Num() - 1; i > 0; --i)
			{
				const int32 j = Rng.RandRange(0, i);
				CoastalTiles.Swap(i, j);
			}

			const int32 Count = FMath::Min(DesiredCount, CoastalTiles.Num());
			for (int32 i = 0; i < Count; ++i)
			{
				const FIntPoint& Pos = CoastalTiles[i];

				// Surface -> Underwater at same position on same plane.
				CreatePortalPair(Map, ECoMPortalType::UnderwaterAccess,
								 Plane, ECoMLayer::Surface, Pos,
								 Plane, ECoMLayer::Underwater, Pos,
								 /*bBidirectional=*/true);
			}
		}
	}
}

int32 UCoMLeyPortalSubsystem::CreatePortalPair(
	UCoMWorldMapSubsystem* Map,
	ECoMPortalType Type,
	ECoMPlane SrcPlane, ECoMLayer SrcLayer, FIntPoint SrcPos,
	ECoMPlane DstPlane, ECoMLayer DstLayer, FIntPoint DstPos,
	bool bBidirectional)
{
	// Source portal.
	FCoMPortal SrcPortal;
	SrcPortal.PortalID = NextPortalID++;
	SrcPortal.Type = Type;
	SrcPortal.SourcePlane = SrcPlane;
	SrcPortal.SourceLayer = SrcLayer;
	SrcPortal.SourcePosition = SrcPos;
	SrcPortal.DestPlane = DstPlane;
	SrcPortal.DestLayer = DstLayer;
	SrcPortal.DestPosition = DstPos;
	SrcPortal.bBidirectional = bBidirectional;
	SrcPortal.bAlwaysActive = true;
	SrcPortal.ActivationManaCost = FFixed64(0);
	SrcPortal.bDiscovered = false;
	SrcPortal.bPlayerBuilt = false;
	SrcPortal.OwnerWizardIndex = INDEX_NONE;
	SrcPortal.Capacity = 1;
	SrcPortal.bShifting = false;
	SrcPortal.ShiftIntervalTurns = 0;

	const int32 SrcID = SrcPortal.PortalID;

	// Mark source tile.
	if (FCoMTileData* Tile = Map->GetTileMutable(SrcPlane, SrcLayer, SrcPos.X, SrcPos.Y))
	{
		Tile->PortalID = SrcID;
	}

	AllPortals.Add(MoveTemp(SrcPortal));

	// Destination portal (reverse direction).
	if (bBidirectional)
	{
		FCoMPortal DstPortal;
		DstPortal.PortalID = NextPortalID++;
		DstPortal.Type = Type;
		DstPortal.SourcePlane = DstPlane;
		DstPortal.SourceLayer = DstLayer;
		DstPortal.SourcePosition = DstPos;
		DstPortal.DestPlane = SrcPlane;
		DstPortal.DestLayer = SrcLayer;
		DstPortal.DestPosition = SrcPos;
		DstPortal.bBidirectional = true;
		DstPortal.bAlwaysActive = true;
		DstPortal.ActivationManaCost = FFixed64(0);
		DstPortal.bDiscovered = false;
		DstPortal.bPlayerBuilt = false;
		DstPortal.OwnerWizardIndex = INDEX_NONE;
		DstPortal.Capacity = 1;
		DstPortal.bShifting = false;
		DstPortal.ShiftIntervalTurns = 0;

		if (FCoMTileData* Tile = Map->GetTileMutable(DstPlane, DstLayer, DstPos.X, DstPos.Y))
		{
			Tile->PortalID = DstPortal.PortalID;
		}

		AllPortals.Add(MoveTemp(DstPortal));
	}

	return SrcID;
}

// ============================================================================
// Queries
// ============================================================================

const FCoMLeyLine* UCoMLeyPortalSubsystem::GetLeyLine(int32 ID) const
{
	for (const FCoMLeyLine& Line : AllLeyLines)
	{
		if (Line.LeyLineID == ID)
		{
			return &Line;
		}
	}
	return nullptr;
}

const FCoMPortal* UCoMLeyPortalSubsystem::GetPortal(int32 ID) const
{
	for (const FCoMPortal& Portal : AllPortals)
	{
		if (Portal.PortalID == ID)
		{
			return &Portal;
		}
	}
	return nullptr;
}

TArray<const FCoMLeyLine*> UCoMLeyPortalSubsystem::GetLeyLinesOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMLeyLine*> Result;
	for (const FCoMLeyLine& Line : AllLeyLines)
	{
		if (Line.Plane == Plane)
		{
			Result.Add(&Line);
		}
	}
	return Result;
}

TArray<const FCoMPortal*> UCoMLeyPortalSubsystem::GetPortalsOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMPortal*> Result;
	for (const FCoMPortal& Portal : AllPortals)
	{
		if (Portal.SourcePlane == Plane)
		{
			Result.Add(&Portal);
		}
	}
	return Result;
}

TArray<const FCoMLeyIntersection*> UCoMLeyPortalSubsystem::GetIntersectionsOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMLeyIntersection*> Result;
	for (const FCoMLeyIntersection& Inter : AllIntersections)
	{
		if (Inter.Plane == Plane)
		{
			Result.Add(&Inter);
		}
	}
	return Result;
}

const FCoMPortal* UCoMLeyPortalSubsystem::FindNearestPortal(ECoMPlane Plane,
															 FIntPoint Position) const
{
	const FCoMPortal* Best = nullptr;
	int32 BestDist = TNumericLimits<int32>::Max();

	for (const FCoMPortal& Portal : AllPortals)
	{
		if (Portal.SourcePlane != Plane)
		{
			continue;
		}
		const int32 D = WrappedDist(Position, Portal.SourcePosition);
		if (D < BestDist)
		{
			BestDist = D;
			Best = &Portal;
		}
	}

	return Best;
}

// ============================================================================
// Utility helpers
// ============================================================================

int32 UCoMLeyPortalSubsystem::WrapX(int32 X)
{
	return ((X % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
}

int32 UCoMLeyPortalSubsystem::WrappedDist(FIntPoint A, FIntPoint B)
{
	int32 DX = FMath::Abs(A.X - B.X);
	if (DX > CoM::MAP_WIDTH / 2)
	{
		DX = CoM::MAP_WIDTH - DX;
	}
	const int32 DY = FMath::Abs(A.Y - B.Y);
	return DX + DY;
}

bool UCoMLeyPortalSubsystem::IsTilePassable(UCoMWorldMapSubsystem* Map,
											 ECoMPlane Plane,
											 int32 X, int32 Y) const
{
	const FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMLayer::Surface, X, Y);
	if (!Tile)
	{
		return false;
	}
	// Ocean tiles are impassable for ley lines.
	// Assumes ECoMTerrain::Ocean / DeepOcean exist on the tile.
	return Tile->Terrain != ECoMTerrain::Ocean
		&& Tile->Terrain != ECoMTerrain::DeepOcean;
}

bool UCoMLeyPortalSubsystem::IsMountainTile(UCoMWorldMapSubsystem* Map,
											 ECoMPlane Plane,
											 int32 X, int32 Y) const
{
	const FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMLayer::Surface, X, Y);
	if (!Tile)
	{
		return false;
	}
	return Tile->Terrain == ECoMTerrain::Mountain
		|| Tile->Terrain == ECoMTerrain::VolcanicMountain;
}

bool UCoMLeyPortalSubsystem::IsCoastalTile(UCoMWorldMapSubsystem* Map,
											ECoMPlane Plane,
											int32 X, int32 Y) const
{
	const FCoMTileData* Tile = Map->GetTileMutable(Plane, ECoMLayer::Surface, X, Y);
	if (!Tile)
	{
		return false;
	}
	return Tile->Terrain == ECoMTerrain::Shore
		|| Tile->Terrain == ECoMTerrain::AmberCoast;
}

ECoMSpellRealm UCoMLeyPortalSubsystem::RandomAlignment(FRandomStream& Rng) const
{
	// Assumes ECoMSpellRealm has a known count. Pick uniformly.
	const int32 RealmCount = static_cast<int32>(ECoMSpellRealm::MAX);
	return static_cast<ECoMSpellRealm>(Rng.RandRange(0, RealmCount - 1));
}

ECoMPlane UCoMLeyPortalSubsystem::RandomOtherPlane(ECoMPlane Exclude,
													FRandomStream& Rng) const
{
	ECoMPlane Result;
	do
	{
		Result = static_cast<ECoMPlane>(Rng.RandRange(0, CoM::NUM_PLANES - 1));
	}
	while (Result == Exclude);
	return Result;
}
