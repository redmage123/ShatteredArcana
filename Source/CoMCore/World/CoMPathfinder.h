#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMEnums.h"
#include "CoMStructs.h"
#include "CoMConstants.h"
#include "CoMPathfinder.generated.h"

class UCoMWorldMapSubsystem;
class UCoMLeyPortalSubsystem;

USTRUCT(BlueprintType)
struct COMCORE_API FCoMPathSegment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ECoMPlane Plane;

	UPROPERTY(BlueprintReadOnly)
	ECoMMapLayer Layer;

	UPROPERTY(BlueprintReadOnly)
	TArray<FIntPoint> Tiles;

	UPROPERTY(BlueprintReadOnly)
	FFixed64 SegmentCost;

	UPROPERTY(BlueprintReadOnly)
	int32 PortalUsed = -1;
};

USTRUCT(BlueprintType)
struct COMCORE_API FCoMPathResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FCoMPathSegment> Segments;

	UPROPERTY(BlueprintReadOnly)
	FFixed64 TotalCost;

	UPROPERTY(BlueprintReadOnly)
	bool bFound = false;
};

USTRUCT(BlueprintType)
struct COMCORE_API FCoMPathRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	ECoMPlane StartPlane;

	UPROPERTY(BlueprintReadWrite)
	ECoMMapLayer StartLayer;

	UPROPERTY(BlueprintReadWrite)
	FIntPoint StartPos;

	UPROPERTY(BlueprintReadWrite)
	ECoMPlane GoalPlane;

	UPROPERTY(BlueprintReadWrite)
	ECoMMapLayer GoalLayer;

	UPROPERTY(BlueprintReadWrite)
	FIntPoint GoalPos;

	UPROPERTY(BlueprintReadWrite)
	int32 WizardIndex = 0;

	UPROPERTY(BlueprintReadWrite)
	bool bAllowPortals = true;

	UPROPERTY(BlueprintReadWrite)
	bool bAllowUnexplored = false;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxSearchNodes = 50000;
};

UCLASS()
class COMCORE_API UCoMPathfinder : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoM|Pathfinding")
	FCoMPathResult FindPath(
		UCoMWorldMapSubsystem* Map,
		UCoMLeyPortalSubsystem* LeyPortals,
		const FCoMPathRequest& Request
	);

private:
	struct FPathNode
	{
		ECoMPlane Plane;
		ECoMMapLayer Layer;
		int32 X;
		int32 Y;
		FFixed64 GCost;
		FFixed64 FCost;
		uint64 ParentKey;
		int32 PortalUsedToReach; // Portal ID used to arrive at this node, -1 if none

		bool operator>(const FPathNode& Other) const
		{
			return FCost > Other.FCost;
		}
	};

	static uint64 PackNode(ECoMPlane Plane, ECoMMapLayer Layer, int32 X, int32 Y);
	static FFixed64 GetTerrainBaseCost(ECoMTerrain Terrain);
	static bool IsTerrainImpassable(ECoMTerrain Terrain);
	static FFixed64 ComputeHeuristic(
		ECoMPlane FromPlane, ECoMMapLayer FromLayer, int32 FromX, int32 FromY,
		ECoMPlane GoalPlane, ECoMMapLayer GoalLayer, int32 GoalX, int32 GoalY
	);

	void HeapPush(TArray<FPathNode>& Heap, FPathNode Node);
	FPathNode HeapPop(TArray<FPathNode>& Heap);

	FCoMPathResult ReconstructPath(
		const TMap<uint64, FPathNode>& AllNodes,
		uint64 GoalKey,
		uint64 StartKey
	);
};
