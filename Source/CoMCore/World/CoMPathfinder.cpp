#include "CoMPathfinder.h"
#include "CoMWorldMapSubsystem.h"
#include "CoMLeyPortalSubsystem.h"

// --------------------------------------------------------------------------
// Node packing: Plane(4 bits) | Layer(4 bits) | X(24 bits) | Y(24 bits) = 56 bits used
// --------------------------------------------------------------------------
uint64 UCoMPathfinder::PackNode(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y)
{
	const uint64 P = static_cast<uint64>(Plane) & 0xF;
	const uint64 L = static_cast<uint64>(Layer) & 0xF;
	const uint64 PX = static_cast<uint64>(X) & 0xFFFFFF;
	const uint64 PY = static_cast<uint64>(Y) & 0xFFFFFF;
	return (P << 52) | (L << 48) | (PX << 24) | PY;
}

// --------------------------------------------------------------------------
// Terrain base cost lookup
// --------------------------------------------------------------------------
FFixed64 UCoMPathfinder::GetTerrainBaseCost(ECoMTerrain Terrain)
{
	switch (Terrain)
	{
	case ECoMTerrain::Grassland:
	case ECoMTerrain::Plains:
	case ECoMTerrain::Savanna:
	case ECoMTerrain::River:
	case ECoMTerrain::Shore:
		return FFixed64(1.0);

	case ECoMTerrain::Desert:
	case ECoMTerrain::Tundra:
		return FFixed64(1.5);

	case ECoMTerrain::Forest:
	case ECoMTerrain::Jungle:
	case ECoMTerrain::Hills:
		return FFixed64(2.0);

	case ECoMTerrain::Mountains:
		return FFixed64(3.0);

	case ECoMTerrain::Swamp:
	case ECoMTerrain::Marsh:
		return FFixed64(3.0);

	case ECoMTerrain::Ocean:
		// Should not be queried for ground cost; treated as impassable
		return FFixed64(100.0);

	default:
		return FFixed64(1.0);
	}
}

bool UCoMPathfinder::IsTerrainImpassable(ECoMTerrain Terrain)
{
	return Terrain == ECoMTerrain::Ocean;
}

// --------------------------------------------------------------------------
// Heuristic: Manhattan with WrapX. Returns 0 for cross-plane queries.
// --------------------------------------------------------------------------
FFixed64 UCoMPathfinder::ComputeHeuristic(
	ECoMPlane FromPlane, ECoMMapLayer FromLayer, int32 FromX, int32 FromY,
	ECoMPlane GoalPlane, ECoMMapLayer GoalLayer, int32 GoalX, int32 GoalY)
{
	if (FromPlane != GoalPlane || FromLayer != GoalLayer)
	{
		return FFixed64(0.0);
	}

	// WrapX Manhattan distance
	int32 DX = FMath::Abs(GoalX - FromX);
	if (DX > CoM::MAP_WIDTH / 2)
	{
		DX = CoM::MAP_WIDTH - DX;
	}
	const int32 DY = FMath::Abs(GoalY - FromY);

	return FFixed64(static_cast<double>(DX + DY));
}

// --------------------------------------------------------------------------
// Binary min-heap operations (min by FCost)
// --------------------------------------------------------------------------
void UCoMPathfinder::HeapPush(TArray<FPathNode>& Heap, FPathNode Node)
{
	int32 Index = Heap.Add(MoveTemp(Node));

	// Sift up
	while (Index > 0)
	{
		const int32 Parent = (Index - 1) / 2;
		if (Heap[Index].FCost < Heap[Parent].FCost)
		{
			Heap.Swap(Index, Parent);
			Index = Parent;
		}
		else
		{
			break;
		}
	}
}

UCoMPathfinder::FPathNode UCoMPathfinder::HeapPop(TArray<FPathNode>& Heap)
{
	check(Heap.Num() > 0);

	FPathNode Top = MoveTemp(Heap[0]);

	const int32 Last = Heap.Num() - 1;
	if (Last > 0)
	{
		Heap[0] = MoveTemp(Heap[Last]);
	}
	Heap.RemoveAt(Last, EAllowShrinking::No);

	// Sift down
	const int32 Count = Heap.Num();
	int32 Index = 0;
	while (true)
	{
		const int32 Left = 2 * Index + 1;
		const int32 Right = 2 * Index + 2;
		int32 Smallest = Index;

		if (Left < Count && Heap[Left].FCost < Heap[Smallest].FCost)
		{
			Smallest = Left;
		}
		if (Right < Count && Heap[Right].FCost < Heap[Smallest].FCost)
		{
			Smallest = Right;
		}
		if (Smallest != Index)
		{
			Heap.Swap(Index, Smallest);
			Index = Smallest;
		}
		else
		{
			break;
		}
	}

	return Top;
}

// --------------------------------------------------------------------------
// Path reconstruction: backtrack from goal, split at portal transitions
// --------------------------------------------------------------------------
FCoMPathResult UCoMPathfinder::ReconstructPath(
	const TMap<uint64, FPathNode>& AllNodes,
	uint64 GoalKey,
	uint64 StartKey)
{
	FCoMPathResult Result;
	Result.bFound = true;

	// Collect nodes in reverse order (goal to start)
	TArray<const FPathNode*> NodeChain;
	uint64 CurrentKey = GoalKey;

	while (true)
	{
		const FPathNode* Node = AllNodes.Find(CurrentKey);
		if (!Node)
		{
			break;
		}
		NodeChain.Add(Node);
		if (CurrentKey == StartKey)
		{
			break;
		}
		CurrentKey = Node->ParentKey;
	}

	// Reverse so we go start -> goal
	Algo::Reverse(NodeChain);

	if (NodeChain.Num() == 0)
	{
		Result.bFound = false;
		return Result;
	}

	// Build segments, splitting at portal transitions
	FCoMPathSegment CurrentSegment;
	CurrentSegment.Plane = NodeChain[0]->Plane;
	CurrentSegment.Layer = NodeChain[0]->Layer;
	CurrentSegment.SegmentCost = FFixed64(0.0);
	CurrentSegment.PortalUsed = -1;

	for (int32 i = 0; i < NodeChain.Num(); ++i)
	{
		const FPathNode* Node = NodeChain[i];

		// If this node arrived via a portal, close the previous segment and start a new one
		if (Node->PortalUsedToReach >= 0 && i > 0)
		{
			// Finalize the previous segment
			Result.Segments.Add(MoveTemp(CurrentSegment));

			// Start a new segment on the destination side of the portal
			CurrentSegment = FCoMPathSegment();
			CurrentSegment.Plane = Node->Plane;
			CurrentSegment.Layer = Node->Layer;
			CurrentSegment.SegmentCost = FFixed64(0.0);
			CurrentSegment.PortalUsed = Node->PortalUsedToReach;
		}

		CurrentSegment.Tiles.Add(FIntPoint(Node->X, Node->Y));

		// Accumulate cost delta within this segment
		if (i > 0)
		{
			const FPathNode* Prev = NodeChain[i - 1];
			const FFixed64 Delta = Node->GCost - Prev->GCost;
			CurrentSegment.SegmentCost = CurrentSegment.SegmentCost + Delta;
		}
	}

	// Add the final segment
	Result.Segments.Add(MoveTemp(CurrentSegment));

	// Total cost from goal node
	Result.TotalCost = NodeChain.Last()->GCost;

	return Result;
}

// --------------------------------------------------------------------------
// A* pathfinding with portal edges and ley line bonuses
// --------------------------------------------------------------------------
FCoMPathResult UCoMPathfinder::FindPath(
	UCoMWorldMapSubsystem* Map,
	UCoMLeyPortalSubsystem* LeyPortals,
	const FCoMPathRequest& Request)
{
	FCoMPathResult FailResult;
	FailResult.bFound = false;

	if (!Map || !LeyPortals)
	{
		return FailResult;
	}

	// Normalize start and goal positions
	FIntPoint StartNorm = Map->NormalizePosition(Request.StartPos.X, Request.StartPos.Y);
	FIntPoint GoalNorm = Map->NormalizePosition(Request.GoalPos.X, Request.GoalPos.Y);

	const uint64 StartKey = PackNode(Request.StartPlane, Request.StartLayer, StartNorm.X, StartNorm.Y);
	const uint64 GoalKey = PackNode(Request.GoalPlane, Request.GoalLayer, GoalNorm.X, GoalNorm.Y);

	// Trivial case: already at goal
	if (StartKey == GoalKey)
	{
		FCoMPathResult Result;
		Result.bFound = true;
		Result.TotalCost = FFixed64(0.0);

		FCoMPathSegment Seg;
		Seg.Plane = Request.StartPlane;
		Seg.Layer = Request.StartLayer;
		Seg.Tiles.Add(StartNorm);
		Seg.SegmentCost = FFixed64(0.0);
		Seg.PortalUsed = -1;
		Result.Segments.Add(MoveTemp(Seg));
		return Result;
	}

	// Open set (binary min-heap) and node storage
	TArray<FPathNode> OpenHeap;
	OpenHeap.Reserve(1024);

	TMap<uint64, FPathNode> AllNodes;
	AllNodes.Reserve(4096);

	TSet<uint64> ClosedSet;
	ClosedSet.Reserve(4096);

	// Seed the start node
	FPathNode StartNode;
	StartNode.Plane = Request.StartPlane;
	StartNode.Layer = Request.StartLayer;
	StartNode.X = StartNorm.X;
	StartNode.Y = StartNorm.Y;
	StartNode.GCost = FFixed64(0.0);
	StartNode.FCost = ComputeHeuristic(
		Request.StartPlane, Request.StartLayer, StartNorm.X, StartNorm.Y,
		Request.GoalPlane, Request.GoalLayer, GoalNorm.X, GoalNorm.Y
	);
	StartNode.ParentKey = StartKey; // Points to itself
	StartNode.PortalUsedToReach = -1;

	AllNodes.Add(StartKey, StartNode);
	HeapPush(OpenHeap, StartNode);

	int32 NodesExpanded = 0;

	// 4-directional neighbor offsets
	static constexpr int32 DX[] = { 1, -1, 0, 0 };
	static constexpr int32 DY[] = { 0, 0, 1, -1 };

	while (OpenHeap.Num() > 0)
	{
		FPathNode Current = HeapPop(OpenHeap);
		const uint64 CurrentKey = PackNode(Current.Plane, Current.Layer, Current.X, Current.Y);

		// Goal check
		if (CurrentKey == GoalKey)
		{
			return ReconstructPath(AllNodes, GoalKey, StartKey);
		}

		// Skip if already expanded
		if (ClosedSet.Contains(CurrentKey))
		{
			continue;
		}
		ClosedSet.Add(CurrentKey);

		++NodesExpanded;
		if (NodesExpanded >= Request.MaxSearchNodes)
		{
			// Search exhausted without finding path
			return FailResult;
		}

		// ----- Expand cardinal neighbors -----
		for (int32 Dir = 0; Dir < 4; ++Dir)
		{
			int32 NX = Current.X + DX[Dir];
			int32 NY = Current.Y + DY[Dir];

			// WrapX normalization
			if (NX < 0)
			{
				NX += CoM::MAP_WIDTH;
			}
			else if (NX >= CoM::MAP_WIDTH)
			{
				NX -= CoM::MAP_WIDTH;
			}

			// Y bounds (no wrap)
			if (NY < 0 || NY >= CoM::MAP_HEIGHT)
			{
				continue;
			}

			const uint64 NeighborKey = PackNode(Current.Plane, Current.Layer, NX, NY);
			if (ClosedSet.Contains(NeighborKey))
			{
				continue;
			}

			const FCoMTileData& TileData = Map->GetTile(Current.Plane, Current.Layer, NX, NY);

			// Check passability
			if (TileData.bImpassable || IsTerrainImpassable(TileData.Terrain))
			{
				continue;
			}

			// Compute movement cost
			FFixed64 MoveCost = GetTerrainBaseCost(TileData.Terrain) * TileData.MoveCostModifier;

			// Ley line bonus: 0.5x multiplier
			if (TileData.LeyLineIDs.Num() > 0)
			{
				MoveCost = MoveCost * FFixed64(0.5);
			}

			const FFixed64 TentativeG = Current.GCost + MoveCost;

			// Check if we already have a better path to this neighbor
			const FPathNode* ExistingNode = AllNodes.Find(NeighborKey);
			if (ExistingNode && ExistingNode->GCost <= TentativeG)
			{
				continue;
			}

			FPathNode Neighbor;
			Neighbor.Plane = Current.Plane;
			Neighbor.Layer = Current.Layer;
			Neighbor.X = NX;
			Neighbor.Y = NY;
			Neighbor.GCost = TentativeG;
			Neighbor.FCost = TentativeG + ComputeHeuristic(
				Current.Plane, Current.Layer, NX, NY,
				Request.GoalPlane, Request.GoalLayer, GoalNorm.X, GoalNorm.Y
			);
			Neighbor.ParentKey = CurrentKey;
			Neighbor.PortalUsedToReach = -1;

			AllNodes.Add(NeighborKey, Neighbor);
			HeapPush(OpenHeap, Neighbor);
		}

		// ----- Expand portal edges -----
		if (Request.bAllowPortals)
		{
			const FCoMTileData& CurrentTile = Map->GetTile(Current.Plane, Current.Layer, Current.X, Current.Y);

			if (CurrentTile.PortalID >= 0)
			{
				const FCoMPortal& Portal = LeyPortals->GetPortal(CurrentTile.PortalID);

				if (Portal.bAlwaysActive)
				{
					// Determine destination based on which end we are standing on
					ECoMPlane DestPlane;
					ECoMMapLayer DestLayer;
					FIntPoint DestPos;

					const bool bAtSource =
						Current.Plane == Portal.SourcePlane &&
						Current.Layer == Portal.SourceLayer &&
						Current.X == Portal.SourcePosition.X &&
						Current.Y == Portal.SourcePosition.Y;

					if (bAtSource)
					{
						DestPlane = Portal.DestPlane;
						DestLayer = Portal.DestLayer;
						DestPos = Portal.DestPosition;
					}
					else if (Portal.bBidirectional)
					{
						DestPlane = Portal.SourcePlane;
						DestLayer = Portal.SourceLayer;
						DestPos = Portal.SourcePosition;
					}
					else
					{
						// Standing on destination side of a one-way portal; no traversal
						goto SkipPortal;
					}

					{
						const uint64 PortalDestKey = PackNode(DestPlane, DestLayer, DestPos.X, DestPos.Y);

						if (!ClosedSet.Contains(PortalDestKey))
						{
							const FFixed64 PortalCost = FFixed64(1.0);
							const FFixed64 TentativeG = Current.GCost + PortalCost;

							const FPathNode* ExistingNode = AllNodes.Find(PortalDestKey);
							if (!ExistingNode || ExistingNode->GCost > TentativeG)
							{
								FPathNode PortalNode;
								PortalNode.Plane = DestPlane;
								PortalNode.Layer = DestLayer;
								PortalNode.X = DestPos.X;
								PortalNode.Y = DestPos.Y;
								PortalNode.GCost = TentativeG;
								PortalNode.FCost = TentativeG + ComputeHeuristic(
									DestPlane, DestLayer, DestPos.X, DestPos.Y,
									Request.GoalPlane, Request.GoalLayer, GoalNorm.X, GoalNorm.Y
								);
								PortalNode.ParentKey = CurrentKey;
								PortalNode.PortalUsedToReach = CurrentTile.PortalID;

								AllNodes.Add(PortalDestKey, PortalNode);
								HeapPush(OpenHeap, PortalNode);
							}
						}
					}
				}
			}
			SkipPortal:;
		}
	}

	// Open set exhausted, no path found
	return FailResult;
}
