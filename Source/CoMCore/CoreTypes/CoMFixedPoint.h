// Copyright Mythforge Studios. All Rights Reserved.
// CoMFixedPoint.h — Deterministic 64-bit fixed-point type for simulation math.
//
// Use FFixed64 instead of float or double in any struct that feeds game simulation.
// Platform-agnostic: identical bit-results on all compilers / CPU architectures.
//
// COM-NET-P1-2: Float sweep — replaces all simulation float fields.

#pragma once
#include "CoreMinimal.h"
#include "CoMFixedPoint.generated.h"

/**
 * 64-bit fixed-point number — Q47.16 format (16 fractional bits).
 *
 *   Range:     ±140,737,488,355,327 (integer part)
 *   Precision: 1/65536 ≈ 0.0000153  (~4.5 significant decimal places)
 *
 * WHY FIXED-POINT?
 *   IEEE 754 float arithmetic is NOT deterministic across platforms.
 *   x87 vs SSE extended precision, FTZ/DAZ flush-to-zero flags, and
 *   compiler vectorisation all produce different results on different
 *   machines. FFixed64 uses integer arithmetic only and produces
 *   bit-identical results on all supported platforms (Windows/Linux/macOS,
 *   x64/ARM64), which is a hard requirement for the determinism CI gate.
 *
 * RULES:
 *   ✅  Use FFixed64 in all simulation structs (combat, economy, magic).
 *   ✅  Use ToFloat() only for display / debug — never back into simulation.
 *   ✅  Use FFixed64(float f) only at DataAsset load time, not per-turn.
 *   ❌  Never compare via ToFloat() — compare RawValue directly.
 *   ❌  Never use float/double in structs that enter FCoMGameStateSnapshot.
 *
 * USAGE:
 *   FFixed64 x(1);                  // 1.0
 *   FFixed64 y = FFixed64::Half();  // 0.5
 *   FFixed64 z = x + y;             // 1.5
 *   float f  = z.ToFloat();         // for UI display only
 */
USTRUCT(BlueprintType)
struct COMCORE_API FFixed64
{
	GENERATED_BODY()

	// ── Format constants ──────────────────────────────────────────────────────
	static constexpr int32 FRAC_BITS = 16;
	static constexpr int64 ONE       = 1LL << FRAC_BITS; // 65536

	// ── Storage ───────────────────────────────────────────────────────────────
	/** Raw fixed-point bits. 1.0 = 65536, 0.5 = 32768. Serialised by UE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FixedPoint",
	          meta = (ToolTip = "Raw Q47.16 bits. Divide by 65536 to get real value."))
	int64 RawValue = 0;

	// ── Constructors ──────────────────────────────────────────────────────────

	FFixed64() = default;

	/** Construct from integer. FFixed64(1) == 1.0, FFixed64(100) == 100.0 */
	FORCEINLINE explicit FFixed64(int32 Int)
		: RawValue(static_cast<int64>(Int) * ONE) {}

	FORCEINLINE explicit FFixed64(int64 Int)
		: RawValue(Int * ONE) {}

	/**
	 * Construct from float — use ONLY for DataAsset load / editor init.
	 * NEVER call inside per-turn simulation loops.
	 */
	FORCEINLINE explicit FFixed64(float F)
		: RawValue(static_cast<int64>(
			F * static_cast<float>(ONE) + (F >= 0.0f ? 0.5f : -0.5f))) {}

	FORCEINLINE explicit FFixed64(double D)
		: RawValue(static_cast<int64>(
			D * static_cast<double>(ONE) + (D >= 0.0 ? 0.5 : -0.5))) {}

	/** Internal: construct directly from raw bits (serialisation / arithmetic). */
	static FORCEINLINE FFixed64 FromRaw(int64 Raw) { FFixed64 R; R.RawValue = Raw; return R; }

	// ── Conversion ────────────────────────────────────────────────────────────

	/** For UI display / debug only. Never feed result back into simulation. */
	FORCEINLINE float  ToFloat()  const { return static_cast<float>(RawValue)  / static_cast<float>(ONE); }
	FORCEINLINE double ToDouble() const { return static_cast<double>(RawValue) / static_cast<double>(ONE); }
	FORCEINLINE int32  ToInt32()  const { return static_cast<int32>(RawValue >> FRAC_BITS); }
	FORCEINLINE int64  ToInt64()  const { return RawValue >> FRAC_BITS; }

	// ── Arithmetic ────────────────────────────────────────────────────────────

	FORCEINLINE FFixed64 operator+(const FFixed64& O) const { return FromRaw(RawValue + O.RawValue); }
	FORCEINLINE FFixed64 operator-(const FFixed64& O) const { return FromRaw(RawValue - O.RawValue); }
	FORCEINLINE FFixed64 operator-() const { return FromRaw(-RawValue); }

	/** Multiply: result = (a * b) with correct fractional scaling. */
	FORCEINLINE FFixed64 operator*(const FFixed64& O) const
	{
		return FromRaw((RawValue * O.RawValue) >> FRAC_BITS);
	}

	/** Divide: safe (returns 0 on divide-by-zero). */
	FORCEINLINE FFixed64 operator/(const FFixed64& O) const
	{
		return FromRaw(O.RawValue != 0 ? (RawValue << FRAC_BITS) / O.RawValue : 0);
	}

	FORCEINLINE FFixed64& operator+=(const FFixed64& O) { RawValue += O.RawValue; return *this; }
	FORCEINLINE FFixed64& operator-=(const FFixed64& O) { RawValue -= O.RawValue; return *this; }
	FORCEINLINE FFixed64& operator*=(const FFixed64& O)
	{
		RawValue = (RawValue * O.RawValue) >> FRAC_BITS;
		return *this;
	}
	FORCEINLINE FFixed64& operator/=(const FFixed64& O)
	{
		RawValue = O.RawValue != 0 ? (RawValue << FRAC_BITS) / O.RawValue : 0;
		return *this;
	}

	/** Scalar multiply/divide by integer (no precision loss). */
	FORCEINLINE FFixed64  operator*(int32 S) const { return FromRaw(RawValue * S); }
	FORCEINLINE FFixed64  operator/(int32 S) const { return FromRaw(S != 0 ? RawValue / S : 0); }
	FORCEINLINE FFixed64& operator*=(int32 S) { RawValue *= S; return *this; }
	FORCEINLINE FFixed64& operator/=(int32 S) { if (S) RawValue /= S; return *this; }

	// ── Comparison ────────────────────────────────────────────────────────────

	FORCEINLINE bool operator==(const FFixed64& O) const { return RawValue == O.RawValue; }
	FORCEINLINE bool operator!=(const FFixed64& O) const { return RawValue != O.RawValue; }
	FORCEINLINE bool operator< (const FFixed64& O) const { return RawValue <  O.RawValue; }
	FORCEINLINE bool operator<=(const FFixed64& O) const { return RawValue <= O.RawValue; }
	FORCEINLINE bool operator> (const FFixed64& O) const { return RawValue >  O.RawValue; }
	FORCEINLINE bool operator>=(const FFixed64& O) const { return RawValue >= O.RawValue; }

	FORCEINLINE bool IsZero()     const { return RawValue == 0; }
	FORCEINLINE bool IsPositive() const { return RawValue >  0; }
	FORCEINLINE bool IsNegative() const { return RawValue <  0; }

	// ── Common constants ──────────────────────────────────────────────────────

	/** 0.0 */
	static FORCEINLINE FFixed64 Zero()     { return FFixed64(); }
	/** 1.0 */
	static FORCEINLINE FFixed64 One()      { return FFixed64(1); }
	/** 0.5 */
	static FORCEINLINE FFixed64 Half()     { return FromRaw(ONE / 2); }
	/** 0.25 */
	static FORCEINLINE FFixed64 Quarter()  { return FromRaw(ONE / 4); }
	/** 0.1  (nearest representable: ~0.100006) */
	static FORCEINLINE FFixed64 Tenth()    { return FromRaw(6554); }
	/** 0.05 (nearest representable: ~0.050003) */
	static FORCEINLINE FFixed64 Twentieth(){ return FromRaw(3277); }
	/** Maximum representable value. */
	static FORCEINLINE FFixed64 Max()      { FFixed64 R; R.RawValue = INT64_MAX; return R; }
	/** Minimum representable value (most negative). */
	static FORCEINLINE FFixed64 Min()      { FFixed64 R; R.RawValue = INT64_MIN; return R; }
};
