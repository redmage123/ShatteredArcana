// Copyright Mythforge Studios. All Rights Reserved.
// CoMDeterministicRandom.h — PCG64-based deterministic PRNG for simulation
//
// Rules (§6.1 Deterministic Simulation):
//   - All simulation randomness MUST go through this class via UCoMTurnSubsystem::GetRNG()
//   - Never use FMath::Rand*, FMath::RandRange, or engine random anywhere in CoMCore
//   - State is fully UPROPERTY serializable — survives save/load and network sync
//   - Same seed + same call sequence → bit-identical results on x86 and ARM64
//
// Unit test requirement (see CoMDeterministicRandom.test.cpp):
//   FCoMDeterministicRandom a(12345), b(12345);
//   for (int i = 0; i < 1000; ++i) check(a.NextInt32() == b.NextInt32());

#pragma once
#include "CoreMinimal.h"
#include "CoMDeterministicRandom.generated.h"

/**
 * PCG64 deterministic PRNG — minimal, serializable, no UObject overhead.
 *
 * PCG64 reference: O'Neill, M.E. (2014). PCG: A Family of Simple Fast
 * Space-Efficient Statistically Good Algorithms for Random Number Generation.
 *
 * Internal state: two uint64 values (state + increment/sequence selector).
 * The increment is fixed per-instance at construction so sequence selectors
 * can be used to give each subsystem an independent stream from the same seed.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMDeterministicRandom
{
    GENERATED_BODY()

    // ── Construction ──────────────────────────────────────────────────────────

    FCoMDeterministicRandom() : State(0x853C49E6748FEA9BULL), Inc(0xDA3E39CB94B95BDBULL) {}

    /**
     * Seed constructor. Sequence selects an independent stream — use different
     * values per subsystem (e.g. 1 = combat, 2 = weather, 3 = world events)
     * to prevent cross-subsystem coupling without sacrificing reproducibility.
     */
    explicit FCoMDeterministicRandom(uint64 Seed, uint64 Sequence = 1)
    {
        State = 0;
        Inc = (Sequence << 1u) | 1u; // must be odd
        NextUInt32_Internal();        // advance once to mix seed in
        State += Seed;
        NextUInt32_Internal();
    }

    // ── Core random output ────────────────────────────────────────────────────

    /** Raw 32-bit output. All other methods derive from this. */
    uint32 NextUInt32()  { return NextUInt32_Internal(); }

    /** Uniform [0, Max). Max must be > 0. */
    int32  NextIntRange(int32 Min, int32 Max)
    {
        check(Max > Min);
        const uint32 Range = static_cast<uint32>(Max - Min);
        // Rejection sampling to avoid modulo bias.
        const uint32 Threshold = (~Range + 1u) % Range;
        uint32 R;
        do { R = NextUInt32_Internal(); } while (R < Threshold);
        return Min + static_cast<int32>(R % Range);
    }

    /** Uniform [0, 1) as a fixed-point fraction (scaled to int32 0–1000000). */
    int32 NextFraction_1M()
    {
        return static_cast<int32>(NextUInt32_Internal() % 1000001u);
    }

    /** True with probability Percent/100 (integer, e.g. 30 = 30%). */
    bool RollPercent(int32 Percent)
    {
        return NextIntRange(0, 100) < Percent;
    }

    /** Shuffle a TArray in-place (Fisher-Yates). */
    template<typename T>
    void Shuffle(TArray<T>& Array)
    {
        for (int32 I = Array.Num() - 1; I > 0; --I)
        {
            const int32 J = NextIntRange(0, I + 1);
            Array.Swap(I, J);
        }
    }

    // ── Serializable state ────────────────────────────────────────────────────
    // Both fields must be included in FCoMGameStateSnapshot for determinism.

    UPROPERTY(SaveGame) int64 State = 0;
    UPROPERTY(SaveGame) int64 Inc   = 0;

private:
    FORCEINLINE uint32 NextUInt32_Internal()
    {
        const uint64 OldState = State;
        State = OldState * 6364136223846793005ULL + Inc;
        const uint32 XorShifted = static_cast<uint32>(((OldState >> 18u) ^ OldState) >> 27u);
        const uint32 Rot = static_cast<uint32>(OldState >> 59u);
        return (XorShifted >> Rot) | (XorShifted << ((~Rot + 1u) & 31u));
    }
};
