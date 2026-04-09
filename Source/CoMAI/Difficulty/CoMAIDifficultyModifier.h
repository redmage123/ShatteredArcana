// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIDifficultyModifier.h -- Scales AI bonuses based on game difficulty.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMAITypes.h"
#include "CoMAIDifficultyModifier.generated.h"

/**
 * UCoMAIDifficultyModifier
 *
 * Pure data object that converts a difficulty level into concrete multipliers
 * and bonuses applied to AI wizards. Used by the AI subsystem and city/combat
 * subsystems to scale AI performance.
 *
 * Difficulty levels:
 *   Easy       -- AI gets penalties (0.75x income, 0.9x combat)
 *   Normal     -- No modifiers
 *   Hard       -- +25% income, +10% combat
 *   Lunatic    -- +50% income, +25% combat, +1 starting city
 *   Impossible -- +100% income, +50% combat, +2 starting cities
 */
UCLASS()
class COMAI_API UCoMAIDifficultyModifier : public UObject
{
	GENERATED_BODY()

public:
	/** Income multiplier applied to AI gold, mana, and production income per turn. */
	UFUNCTION(BlueprintCallable, Category = "CoMAI|Difficulty")
	static float GetIncomeMultiplier(ECoMAIDifficulty Difficulty);

	/** Combat power multiplier applied to AI unit damage and defense. */
	UFUNCTION(BlueprintCallable, Category = "CoMAI|Difficulty")
	static float GetCombatMultiplier(ECoMAIDifficulty Difficulty);

	/** Number of extra cities the AI starts with at game begin. */
	UFUNCTION(BlueprintCallable, Category = "CoMAI|Difficulty")
	static int32 GetExtraCities(ECoMAIDifficulty Difficulty);

	/** Research speed multiplier applied to AI spell research progress. */
	UFUNCTION(BlueprintCallable, Category = "CoMAI|Difficulty")
	static float GetResearchMultiplier(ECoMAIDifficulty Difficulty);
};
