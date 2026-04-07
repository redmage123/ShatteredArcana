// Copyright Mythforge Studios. All Rights Reserved.
// CoMMapUtils.h — Shared map coordinate utilities
// Consolidates WrapX, ClampY, and coordinate helpers used across subsystems

#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMConstants.h"

namespace CoM
{

/** Wrap X coordinate to [0, MAP_WIDTH) for cylindrical world topology. */
FORCEINLINE int32 WrapX(int32 X)
{
    return ((X % MAP_WIDTH) + MAP_WIDTH) % MAP_WIDTH;
}

/** Clamp Y coordinate to [0, MAP_HEIGHT). */
FORCEINLINE int32 ClampY(int32 Y)
{
    return FMath::Clamp(Y, 0, MAP_HEIGHT - 1);
}

/** Compute shortest signed delta between two X positions, accounting for wrap. */
FORCEINLINE int32 WrapXDelta(int32 From, int32 To)
{
    int32 Delta = WrapX(To) - WrapX(From);
    if (Delta > MAP_WIDTH / 2) Delta -= MAP_WIDTH;
    if (Delta < -MAP_WIDTH / 2) Delta += MAP_WIDTH;
    return Delta;
}

/** Normalize a tile position (wrap X, clamp Y). */
FORCEINLINE FIntPoint NormalizeTile(FIntPoint Tile)
{
    return FIntPoint(WrapX(Tile.X), ClampY(Tile.Y));
}

/** Compute all tiles within a circular radius of a center tile.
 *  Returns wrapped/clamped coordinates. */
inline TArray<FIntPoint> TilesInRadius(FIntPoint Center, int32 Radius)
{
    TArray<FIntPoint> Result;
    Center = NormalizeTile(Center);
    const int32 R2 = Radius * Radius;
    for (int32 DY = -Radius; DY <= Radius; ++DY)
    {
        for (int32 DX = -Radius; DX <= Radius; ++DX)
        {
            if (DX * DX + DY * DY <= R2)
            {
                Result.Add(NormalizeTile(FIntPoint(Center.X + DX, Center.Y + DY)));
            }
        }
    }
    return Result;
}

/** Collect all keys from a TMap into a TArray — use this to safely iterate
 *  a TMap when the loop body may add/remove entries. */
template<typename K, typename V>
TArray<K> SafeKeys(const TMap<K, V>& Map)
{
    TArray<K> Keys;
    Map.GetKeys(Keys);
    return Keys;
}

} // namespace CoM
