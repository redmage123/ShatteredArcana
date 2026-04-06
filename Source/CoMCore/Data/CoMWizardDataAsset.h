#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMWizardDataAsset.generated.h"

// Forward declarations
class UCoMRetortDataAsset;
class UCoMSpellDataAsset;

/**
 * Data asset defining a wizard character with their starting configuration, personality, and AI behavior
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMWizardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMWizardDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard")
	FName WizardID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Lore")
	FText LoreBio; // Lore backstory

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Starting")
	ECoMPlane StartingPlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	TMap<ECoMSpellRealm, int32> SpellBookPicks; // Realm -> books

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Starting")
	TArray<TSoftObjectPtr<UCoMRetortDataAsset>> StartingRetorts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Starting")
	TArray<TSoftObjectPtr<UCoMSpellDataAsset>> StartingSpells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Starting")
	int32 StartingGold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Starting")
	int32 StartingMana;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	int32 CastingSkillBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	int32 ResearchBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Diplomacy")
	TMap<FName, int32> DiplomacyBias; // WizardID -> bias, positive = friendly

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|AI")
	bool bAIOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|AI")
	FName AIPersonality; // Aggressive/Expansionist/Diplomatic/etc.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Presentation")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wizard|Presentation")
	TSoftObjectPtr<UTexture2D> FortressTexture;
};
