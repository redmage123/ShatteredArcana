#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMShipTypeDataAsset.generated.h"

// Forward declarations
class UCoMSkillDataAsset;

/**
 * Data asset defining a ship type for naval transportation and combat
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMShipTypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMShipTypeDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FName ShipTypeID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Classification")
	ECoMShipType ShipCategory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Cost")
	int32 ProductionCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Cost")
	int32 UpkeepGold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Movement")
	int32 MovePoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 Defense;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 HitPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 Figures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Transport")
	int32 CargoCapacity; // Units it can transport

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	bool bCanBlockade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	bool bCanBombard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 BombardRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Combat")
	int32 BombardDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Skills")
	TArray<TSoftObjectPtr<UCoMSkillDataAsset>> ShipSkills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Movement")
	bool bOceanGoing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Movement")
	bool bRiverCapable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Movement")
	bool bUnderwaterCapable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ship|Presentation")
	TSoftObjectPtr<UStaticMesh> ShipMesh;
};
