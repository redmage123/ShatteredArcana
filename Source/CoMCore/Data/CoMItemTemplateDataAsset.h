#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMItemTemplateDataAsset.generated.h"

// Forward declarations
class UCoMSkillDataAsset;
class UCoMSpellDataAsset;

/**
 * Data asset defining an item template for magical items that can be equipped by heroes
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMItemTemplateDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMItemTemplateDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	ECoMItemSlot Slot = ECoMItemSlot::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Value")
	int32 GoldValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Value")
	int32 ManaValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects")
	TArray<FCoMStatModifier> StatBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects")
	TArray<TSoftObjectPtr<UCoMSkillDataAsset>> GrantedSkills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effects")
	TArray<TSoftObjectPtr<UCoMSpellDataAsset>> GrantedSpells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Classification")
	bool bArtifact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Classification")
	bool bUnique;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Quest")
	bool bQuestItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Quest")
	TArray<FName> QuestChainStepIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Magic")
	ECoMSpellRealm Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
