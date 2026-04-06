#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMResourceNodeDataAsset.generated.h"

/**
 * Data asset defining a resource node type (strategic resource on the map)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMResourceNodeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMResourceNodeDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	ECoMResource ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Yield")
	int32 BaseYield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Yield")
	int32 MineProductionBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Value")
	int32 GoldValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Value")
	int32 ManaValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Plane")
	bool bPlaneSensitive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Plane")
	TArray<ECoMPlane> NativePlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Terrain")
	TArray<ECoMTerrain> ValidTerrains;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Rarity")
	bool bRare;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource|Presentation")
	TSoftObjectPtr<UTexture2D> NodeIcon;
};
