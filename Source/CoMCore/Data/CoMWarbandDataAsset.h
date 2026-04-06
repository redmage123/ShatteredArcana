#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMWarbandDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;
class UCoMItemTemplateDataAsset;

/**
 * Data asset defining a warband (roaming enemy group) with behavior and loot
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMWarbandDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMWarbandDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband")
	FName WarbandID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband")
	FText LeaderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Classification")
	ECoMWarbandType WarbandType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Composition")
	TArray<TSoftObjectPtr<UCoMUnitSpecDataAsset>> PossibleUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Composition")
	int32 SizeMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Composition")
	int32 SizeMax;

	// TODO: Add ECoMWarbandBehavior to CoMEnums.h
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Behavior")
	int32 DefaultBehavior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Diplomacy")
	bool bCanNegotiate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Diplomacy")
	bool bCanBeHired;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Diplomacy")
	int32 HireGoldCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Diplomacy")
	int32 HireManaRep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Loot")
	TArray<TSoftObjectPtr<UCoMItemTemplateDataAsset>> PossibleLoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Loot")
	int32 GoldLootMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Loot")
	int32 GoldLootMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Placement")
	TArray<ECoMPlane> AppearOnPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Placement")
	bool bCanAppearUnderdark;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Placement")
	bool bNaval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warband|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
