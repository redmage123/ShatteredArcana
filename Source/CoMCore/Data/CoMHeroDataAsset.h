#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "CoMHeroDataAsset.generated.h"

// Forward declarations
class UCoMSkillDataAsset;
class UCoMItemTemplateDataAsset;

/**
 * Data asset defining a hero character with unique attributes, skills, and personality
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMHeroDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMHeroDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FName HeroID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Lore")
	FText BackstorySnippet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Classification")
	FGameplayTag RaceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Classification")
	ECoMHeroClass HeroClass = ECoMHeroClass::Fighter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Stats")
	int32 BaseAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Stats")
	int32 BaseDefense;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Stats")
	int32 BaseResistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Stats")
	int32 BaseHitPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Stats")
	int32 BaseMovePoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Magic")
	int32 CastingSkillBase; // 0 = non-caster

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Magic")
	ECoMSpellRealm SpellAlignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skills")
	TArray<TSoftObjectPtr<UCoMSkillDataAsset>> StartingSkills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Equipment")
	TSoftObjectPtr<UCoMItemTemplateDataAsset> StartingItem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Hiring")
	int32 HireCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Personality")
	TMap<FName, int32> PersonalityTraits; // Trait -> intensity

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Relationships")
	TArray<FName> LikedWizardIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Relationships")
	TArray<FName> DislikedWizardIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Relationships")
	TArray<FName> RelationshipIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Fame")
	bool bFamous;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Presentation")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Presentation")
	TSoftObjectPtr<USkeletalMesh> HeroMesh;
};
