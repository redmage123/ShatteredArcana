#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMExplorableSiteDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;
class UCoMSpellDataAsset;
class UCoMItemTemplateDataAsset;

/**
 * Data asset defining an explorable site (dungeon, lair, ruins) with guards and rewards
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMExplorableSiteDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMExplorableSiteDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site")
	FName SiteID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Classification")
	ECoMSiteType SiteType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Guards")
	int32 GuardStrengthMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Guards")
	int32 GuardStrengthMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Guards")
	TArray<TSoftObjectPtr<UCoMUnitSpecDataAsset>> GuardUnitTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	int32 GoldRewardMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	int32 GoldRewardMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	int32 ManaRewardMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	int32 ManaRewardMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	TArray<TSoftObjectPtr<UCoMSpellDataAsset>> PossibleSpellRewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Rewards")
	TArray<TSoftObjectPtr<UCoMItemTemplateDataAsset>> PossibleItemRewards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Quest")
	bool bHasQuestChain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Quest")
	TArray<FName> QuestStepIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Placement")
	TArray<ECoMPlane> AppearOnPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Placement")
	bool bCanAppearUnderdark;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Placement")
	bool bCanAppearUnderwater;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Site|Presentation")
	TSoftObjectPtr<UTexture2D> MapIcon;
};
