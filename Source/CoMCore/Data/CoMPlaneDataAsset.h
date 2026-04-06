#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMPlaneDataAsset.generated.h"

/**
 * Data asset defining a plane of existence with its characteristics, hazards, and resources
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMPlaneDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMPlaneDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane")
	ECoMPlane PlaneID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Magic")
	ECoMSpellRealm AlignedRealm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Magic")
	FFixed64 ManaMultiplierAligned; // For aligned realm

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Magic")
	FFixed64 ManaMultiplierOther;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Hazards")
	TArray<FCoMStatModifier> PlaneHazards; // Damage to non-aligned

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Terrain")
	TArray<ECoMTerrain> UniqueTerrains;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Resources")
	TArray<ECoMResource> UniqueResources;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Atmosphere")
	FText AtmosphereDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Presentation")
	TSoftObjectPtr<UTexture2D> PlaneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plane|Presentation")
	TSoftObjectPtr<UTexture2D> PlaneMapBackground;
};
