#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMTerrainDataAsset.generated.h"

/**
 * Data asset defining a terrain type with its movement costs, yields, and characteristics
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMTerrainDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMTerrainDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	ECoMTerrain TerrainType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Movement")
	FFixed64 MoveCostGround;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Movement")
	FFixed64 MoveCostFlying;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Movement")
	FFixed64 MoveCostSwimming;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Movement")
	bool bImpassableGround;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Movement")
	bool bImpassableNaval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Yield")
	int32 FoodYield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Yield")
	int32 GoldYield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Yield")
	int32 ManaYield;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Magic")
	FFixed64 CorruptionResistance; // 0-1

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Magic")
	ECoMSpellRealm NaturalAlignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Resources")
	TArray<ECoMResource> PossibleResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Plane")
	TArray<ECoMPlane> OccursOnPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Presentation")
	TSoftObjectPtr<UTexture2D> OverworldTile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Presentation")
	TSoftObjectPtr<UMaterialInterface> TerrainMaterial;
};
