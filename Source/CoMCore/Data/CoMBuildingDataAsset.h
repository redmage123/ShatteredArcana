#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMBuildingDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;

/**
 * Data asset defining a city building with its costs, bonuses, and requirements
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMBuildingDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMBuildingDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FName BuildingID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Cost")
	int32 ProductionCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Cost")
	int32 UpkeepGold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Cost")
	int32 UpkeepMana;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 FoodBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 GoldBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 ProductionBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 ManaBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 ResearchBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 UnrestReduction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Defense")
	int32 DefenseBonus; // City walls/towers

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Economy")
	int32 PopulationCapBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Requirements")
	TArray<FName> RequiredBuildingIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	bool bUnique; // Only one per city

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Defense")
	bool bWall;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Defense")
	bool bTower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Units")
	TArray<TSoftObjectPtr<UCoMUnitSpecDataAsset>> EnabledUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Effects")
	TArray<FCoMStatModifier> CityEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
