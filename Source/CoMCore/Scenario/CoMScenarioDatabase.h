// Copyright Shattered Arcana. All Rights Reserved.
// CoMScenarioDatabase.h -- Curated start positions for challenge / scenario mode.
//
// Each scenario tweaks the playtest bootstrap (wizard count, seed, per-wizard
// starting bonuses and personality bias) to produce a distinct opening
// narrative. They're handwritten content — five entries adds 5-15 hours of
// player content with no procgen work.

#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMScenarioDatabase.generated.h"

USTRUCT(BlueprintType)
struct COMCORE_API FCoMScenarioWizardBonus
{
	GENERATED_BODY()

	/** Wizard slot 0..13. The first entry is conventionally the human player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WizardIndex = -1;

	/** Bonus cities on top of the BootstrapGame default of 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ExtraCities = 0;

	/** Bonus starting mana on top of the bootstrap default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ExtraMana = 0;

	/** Bonus starting gold on top of the bootstrap default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ExtraGold = 0;

	/** Pre-known signature global (e.g. "Just_Cause" or "Eternal_Night"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StartingGlobal;

	/** Aggressiveness multiplier for AI archetype injection (0..2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AggressionMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct COMCORE_API FCoMScenarioDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ScenarioID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Synopsis;

	/** Number of wizards in the game (player + AI). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 NumWizards = 4;

	/** Seed override; if 0, BootstrapGame's default is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Seed = 0;

	/** Hard turn cap. Lower numbers create a "speed run" feel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxTurns = 400;

	/** Per-wizard tweaks. Indexed by slot, not array position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCoMScenarioWizardBonus> WizardBonuses;

	/** Restrict to a single victory condition; ECoMVictoryType::None = any. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMVictoryType RequiredVictoryType = ECoMVictoryType::None;
};

class COMCORE_API CoMScenarioDatabase
{
public:
	/** Return all built-in scenarios (lazily registered on first call). */
	static const TArray<FCoMScenarioDef>& GetAll();

	/** Find by ID, or nullptr. */
	static const FCoMScenarioDef* Find(FName ScenarioID);
};
