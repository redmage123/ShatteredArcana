#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMUnitSpecDataAsset.generated.h"

// Forward declarations
class UCoMSkillDataAsset;

/**
 * Data asset defining a unit specification (template for creating unit instances)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMUnitSpecDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMUnitSpecDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FName UnitSpecID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Classification")
	ECoMRace Race;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Classification")
	ECoMUnitCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
	int32 ProductionCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
	int32 UpkeepGold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
	int32 UpkeepFood;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
	int32 UpkeepMana;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
	int32 MovePoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	int32 Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	int32 Defense;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	int32 Resistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	int32 HitPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	int32 Figures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Ranged")
	int32 MissileAttack; // 0 = melee only

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Ranged")
	int32 RangedShots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
	bool bFlying;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
	bool bSwimming;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
	bool bUnderwater;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
	bool bNonCorporeal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Skills")
	TArray<TSoftObjectPtr<UCoMSkillDataAsset>> Skills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Stats")
	TArray<FCoMStatModifier> BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Magic")
	ECoMSpellRealm MagicAlignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Classification")
	bool bHero;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Presentation")
	TSoftObjectPtr<UStaticMesh> Mesh;
};
