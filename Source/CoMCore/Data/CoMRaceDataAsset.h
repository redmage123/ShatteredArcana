#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMRaceDataAsset.generated.h"

// Forward declarations
class UCoMBuildingDataAsset;
class UCoMUnitSpecDataAsset;

/**
 * Data asset defining a playable race with its characteristics, bonuses, and unique units/buildings
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMRaceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMRaceDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race")
	FName RaceID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race")
	ECoMRace RaceEnum;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Plane")
	ECoMPlane NativePlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Plane")
	ECoMMapLayer PreferredLayer; // Surface/Underdark/Underwater

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 PopGrowthBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 FoodMultiplier; // Percent, 100 = normal

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 ProductionMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 GoldMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 ManaMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Economy")
	int32 ResearchMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Content")
	TArray<TSoftObjectPtr<UCoMBuildingDataAsset>> UniqueBuildings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Content")
	TArray<TSoftObjectPtr<UCoMUnitSpecDataAsset>> UniqueUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Bonuses")
	TArray<FCoMStatModifier> CityBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Environment")
	bool bAquatic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Environment")
	bool bUnderdark;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Presentation")
	TSoftObjectPtr<UTexture2D> RaceIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Race|Presentation")
	TSoftObjectPtr<UTexture2D> BannerTexture;
};
