#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMSpellDataAsset.generated.h"

// Forward declarations
class UCoMUnitSpecDataAsset;

/**
 * Data asset defining a spell with its costs, effects, and targeting parameters
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMSpellDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMSpellDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FName SpellID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Classification")
	ECoMSpellRealm Realm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Classification")
	ECoMSpellRarity Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Classification")
	ECoMSpellScope Scope;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Cost")
	int32 ResearchCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Cost")
	int32 CastingCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Cost")
	int32 UpkeepMana; // -1 if not ongoing

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Targeting")
	int32 Range; // 0 = self, -1 = global

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Type")
	bool bSummon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Type")
	bool bInstant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Type")
	bool bOngoing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Type")
	bool bEnchantment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Summon")
	TSoftObjectPtr<UCoMUnitSpecDataAsset> SummonedUnit; // If bSummon

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effects")
	TArray<FCoMStatModifier> TargetModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effects")
	TArray<FCoMStatModifier> CasterModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Damage")
	FFixed64 DamageBase; // 0 if no damage

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Damage")
	ECoMSpellRealm DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|AOE")
	bool bAOE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|AOE")
	int32 AOERadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Effects")
	FGameplayTag EffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

	// Soft path resolved to UNiagaraSystem by CoMRendering — keeps CoMCore rendering-free
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell|Presentation")
	FSoftObjectPath CastVFXPath;
};
