// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIDifficultyModifier.cpp -- Difficulty scaling tables for AI wizards.

#include "Difficulty/CoMAIDifficultyModifier.h"

// ---------------------------------------------------------------------------
// Income multiplier: Easy=0.75, Normal=1.0, Hard=1.25, Lunatic=1.5, Impossible=2.0
// ---------------------------------------------------------------------------

float UCoMAIDifficultyModifier::GetIncomeMultiplier(ECoMAIDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECoMAIDifficulty::Easy:       return 0.75f;
	case ECoMAIDifficulty::Normal:     return 1.00f;
	case ECoMAIDifficulty::Hard:       return 1.25f;
	case ECoMAIDifficulty::Lunatic:    return 1.50f;
	case ECoMAIDifficulty::Impossible: return 2.00f;
	default:                           return 1.00f;
	}
}

// ---------------------------------------------------------------------------
// Combat multiplier: Easy=0.9, Normal=1.0, Hard=1.1, Lunatic=1.25, Impossible=1.5
// ---------------------------------------------------------------------------

float UCoMAIDifficultyModifier::GetCombatMultiplier(ECoMAIDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECoMAIDifficulty::Easy:       return 0.90f;
	case ECoMAIDifficulty::Normal:     return 1.00f;
	case ECoMAIDifficulty::Hard:       return 1.10f;
	case ECoMAIDifficulty::Lunatic:    return 1.25f;
	case ECoMAIDifficulty::Impossible: return 1.50f;
	default:                           return 1.00f;
	}
}

// ---------------------------------------------------------------------------
// Extra starting cities: Easy/Normal/Hard=0, Lunatic=1, Impossible=2
// ---------------------------------------------------------------------------

int32 UCoMAIDifficultyModifier::GetExtraCities(ECoMAIDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECoMAIDifficulty::Easy:       return 0;
	case ECoMAIDifficulty::Normal:     return 0;
	case ECoMAIDifficulty::Hard:       return 0;
	case ECoMAIDifficulty::Lunatic:    return 1;
	case ECoMAIDifficulty::Impossible: return 2;
	default:                           return 0;
	}
}

// ---------------------------------------------------------------------------
// Research multiplier: Easy=0.75, Normal=1.0, Hard=1.25, Lunatic=1.5, Impossible=2.0
// ---------------------------------------------------------------------------

float UCoMAIDifficultyModifier::GetResearchMultiplier(ECoMAIDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case ECoMAIDifficulty::Easy:       return 0.75f;
	case ECoMAIDifficulty::Normal:     return 1.00f;
	case ECoMAIDifficulty::Hard:       return 1.25f;
	case ECoMAIDifficulty::Lunatic:    return 1.50f;
	case ECoMAIDifficulty::Impossible: return 2.00f;
	default:                           return 1.00f;
	}
}
