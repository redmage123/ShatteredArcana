// Copyright Mythforge Studios. All Rights Reserved.
// CoMAITypes.h -- Shared types for the CoMAI module.

#pragma once

#include "CoreMinimal.h"
#include "CoMAITypes.generated.h"

// ---------------------------------------------------------------------------
// AI Priority -- what the strategic planner says to focus on this turn
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECoMAIPriority : uint8
{
	Expand         UMETA(DisplayName = "Expand"),          // Found new cities, claim territory
	BuildEconomy   UMETA(DisplayName = "Build Economy"),   // Gold/production infrastructure
	BuildMilitary  UMETA(DisplayName = "Build Military"),  // Train units, fortify
	Research       UMETA(DisplayName = "Research"),         // Spell research and mana buildings
	Diplomacy      UMETA(DisplayName = "Diplomacy"),       // Seek alliances, avoid wars
	Survive        UMETA(DisplayName = "Survive"),          // Under existential threat

	MAX UMETA(Hidden)
};

// ---------------------------------------------------------------------------
// AI Difficulty -- set at game start, scales bonuses for AI wizards
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECoMAIDifficulty : uint8
{
	Easy,       // AI gets penalties
	Normal,     // No modifiers
	Hard,       // AI gets +25% income, +10% combat
	Lunatic,    // AI gets +50% income, +25% combat, starts with extra city
	Impossible, // AI gets +100% income, +50% combat, starts with 2 extra cities

	MAX UMETA(Hidden)
};

// ---------------------------------------------------------------------------
// AI Strategy -- output of the strategic planner, consumed by the executor
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCoMAIStrategy
{
	GENERATED_BODY()

	/** The single most important goal this turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMAIPriority TopPriority = ECoMAIPriority::BuildEconomy;

	/** Fraction of resources allocated to military (0.0 - 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MilitaryBudget = 0.3f;

	/** Fraction of resources allocated to research (0.0 - 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ResearchBudget = 0.3f;

	/** Fraction of resources allocated to economy/infrastructure (0.0 - 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EconomyBudget = 0.4f;

	/** Wizard indices that pose a threat to us (at war, near our borders, strong). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ThreatWizardIds;

	/** Wizard indices we are allied with or have non-aggression pacts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> AllyWizardIds;

	/** City ID we should expand toward (-1 = no expansion target). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ExpansionTargetCityId = -1;

	/** Our total military power rating this turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OurMilitaryPower = 0.0f;

	/** The strongest enemy's military power. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StrongestEnemyPower = 0.0f;

	/** Game phase estimate: 0 = early, 1 = mid, 2 = late. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GamePhaseEstimate = 0;
};
