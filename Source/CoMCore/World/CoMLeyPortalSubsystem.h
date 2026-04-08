// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMLeyPortalSubsystem.generated.h"

class UCoMWorldMapSubsystem;

/**
 * Combined Ley Line and Portal subsystem.
 * Generates deterministic ley-line networks (Voronoi-relaxed graph per plane)
 * and three portal families (inter-plane, Underdark entrances, underwater access).
 */
UCLASS()
class COMCORE_API UCoMLeyPortalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --------------- Generation ---------------

	/** Generate 15-25 ley lines per plane surface using Voronoi-relaxed graph. */
	UFUNCTION(BlueprintCallable, Category = "CoM|LeyLines")
	void GenerateLeyLines(UCoMWorldMapSubsystem* Map, UPARAM(ref) FRandomStream& Rng);

	/**
	 * Place inter-plane natural portals, Underdark entrances, and underwater
	 * access portals. Must be called after GenerateLeyLines.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Portals")
	void GeneratePortals(UCoMWorldMapSubsystem* Map, UPARAM(ref) FRandomStream& Rng);

	// --------------- Queries ---------------








	// Query helpers
	const FCoMLeyLine* GetLeyLine(int32 LeyLineID) const;
	const FCoMPortal* GetPortal(int32 PortalID) const;
	TArray<const FCoMLeyLine*> GetLeyLinesOnPlane(ECoMPlane Plane) const;
	TArray<const FCoMPortal*> GetPortalsOnPlane(ECoMPlane Plane) const;
	TArray<const FCoMLeyIntersection*> GetIntersectionsOnPlane(ECoMPlane Plane) const;
	const FCoMPortal* FindNearestPortal(ECoMPlane Plane, FIntPoint Position) const;
private:
	// --------------- Storage ---------------

	UPROPERTY()
	TArray<FCoMLeyLine> AllLeyLines;

	UPROPERTY()
	TArray<FCoMLeyIntersection> AllIntersections;

	UPROPERTY()
	TArray<FCoMPortal> AllPortals;

	// --------------- Internal helpers ---------------

	int32 NextLeyLineID = 1;
	int32 NextIntersectionID = 1;
	int32 NextPortalID = 1;

	/** Generate random node positions on the surface layer, avoiding ocean. */
	TArray<FIntPoint> GenerateNodes(UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
									int32 Count, FRandomStream& Rng) const;

	/** Lloyd relaxation: move each node toward the centroid of its Voronoi cell. */
	void LloydRelax(TArray<FIntPoint>& Nodes, UCoMWorldMapSubsystem* Map,
					ECoMPlane Plane, int32 Iterations) const;

	/** Connect nodes using nearest-neighbor Delaunay-like connections. */
	TArray<TPair<int32, int32>> BuildEdges(const TArray<FIntPoint>& Nodes,
										   int32 MaxNeighbors) const;

	/** Trace a tile path between two points using Bresenham interpolation. */
	TArray<FIntPoint> TracePath(FIntPoint From, FIntPoint To,
								UCoMWorldMapSubsystem* Map, ECoMPlane Plane) const;

	/** Detect intersections where 3+ ley lines share a tile. */
	void DetectIntersections(ECoMPlane Plane);

	/** Wrap X coordinate for the toroidal map. */
	static int32 WrapX(int32 X);

	/** Manhattan distance respecting WrapX. */
	static int32 WrappedDist(FIntPoint A, FIntPoint B);

	/** Check if a tile is passable for ley lines (not ocean). */
	bool IsTilePassable(UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
						int32 X, int32 Y) const;

	/** Check if a tile is mountainous (for Underdark entrances). */
	bool IsMountainTile(UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
						int32 X, int32 Y) const;

	/** Check if a tile is coastal (for underwater access). */
	bool IsCoastalTile(UCoMWorldMapSubsystem* Map, ECoMPlane Plane,
					   int32 X, int32 Y) const;

	/** Pick a random spell realm alignment. */
	ECoMSpellRealm RandomAlignment(FRandomStream& Rng) const;

	/** Pick a random plane other than the given one. */
	ECoMPlane RandomOtherPlane(ECoMPlane Exclude, FRandomStream& Rng) const;

	/** Create a portal pair and register on tiles. Returns the source portal ID. */
	int32 CreatePortalPair(UCoMWorldMapSubsystem* Map,
						   ECoMPortalType Type,
						   ECoMPlane SrcPlane, ECoMMapLayer SrcLayer, FIntPoint SrcPos,
						   ECoMPlane DstPlane, ECoMMapLayer DstLayer, FIntPoint DstPos,
						   bool bBidirectional);
};
