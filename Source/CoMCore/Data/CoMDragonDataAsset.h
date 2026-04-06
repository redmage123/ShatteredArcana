#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMDragonDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;
class UCoMSkillDataAsset;
class UCoMSpellDataAsset;

/**
 * Data asset defining a dragon type with its age progression and domain characteristics
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMDragonDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMDragonDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon")
	FName DragonTypeID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Plane")
	ECoMPlane NativePlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Magic")
	ECoMSpellRealm Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Combat")
	ECoMBreathType BreathType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Combat")
	int32 BreathPowerBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Age")
	TSoftObjectPtr<UCoMUnitSpecDataAsset> YoungSpec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Age")
	TSoftObjectPtr<UCoMUnitSpecDataAsset> AdultSpec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Age")
	TSoftObjectPtr<UCoMUnitSpecDataAsset> AncientSpec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Age")
	TSoftObjectPtr<UCoMUnitSpecDataAsset> ElderSpec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Skills")
	TArray<TSoftObjectPtr<UCoMSkillDataAsset>> InnateSkills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Role")
	bool bCanRuleDomain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Role")
	bool bCanBeCompanion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Role")
	bool bCanBeSummoned;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Summon")
	TSoftObjectPtr<UCoMSpellDataAsset> SummonSpell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Domain")
	int32 DomainInfluenceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Domain")
	TArray<FName> DomainArmyTypeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Domain")
	int32 TreasureGenerationRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dragon|Presentation")
	TSoftObjectPtr<USkeletalMesh> DragonMesh;
};
