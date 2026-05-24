// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnTimings.h -- Lightweight per-subsystem timing collector used by the
// playtest harness to produce a perf breakdown without pulling in Insight.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"

namespace CoMTurnTimings
{
	/** When true, COM_TIMED_TICK accumulates into Elapsed; otherwise it's a no-op. */
	COMCORE_API extern bool bEnabled;

	/** Accumulated wall-clock seconds per named phase. */
	COMCORE_API extern TMap<FName, double> Elapsed;

	/** Accumulated invocation counts per named phase. */
	COMCORE_API extern TMap<FName, int32> CallCounts;

	/** Reset all counters. Call before each game in the playtest batch. */
	COMCORE_API void Reset();

	/** Add a sample (called by COM_TIMED_TICK). */
	COMCORE_API void Add(FName Phase, double Seconds);
}

/**
 * Wrap a subsystem call to record its timing. When CoMTurnTimings::bEnabled is
 * false, the macro reduces to just running Code (no overhead beyond a single
 * branch).
 *
 * Usage:
 *   COM_TIMED_TICK("Cities", CitySub->ProcessCityTurn());
 */
#define COM_TIMED_TICK(Name, Code) \
	do { \
		if (CoMTurnTimings::bEnabled) \
		{ \
			const double _T0 = FPlatformTime::Seconds(); \
			Code; \
			CoMTurnTimings::Add(FName(TEXT(Name)), FPlatformTime::Seconds() - _T0); \
		} \
		else \
		{ \
			Code; \
		} \
	} while (0)
