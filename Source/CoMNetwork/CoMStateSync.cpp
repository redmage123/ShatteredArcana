// Copyright Mythforge Studios. All Rights Reserved.
// CoMStateSync.cpp — Delta-encodes FCoMGameStateSnapshot (stub)

#include "CoMStateSync.h"

TArray<uint8> UCoMStateSync::EncodeDelta(
    const FCoMGameStateSnapshot& BaseState,
    const FCoMGameStateSnapshot& NewState)
{
    // TODO: implement delta encoding between BaseState and NewState
    return TArray<uint8>();
}

bool UCoMStateSync::ApplyDelta(
    const FCoMGameStateSnapshot& BaseState,
    const TArray<uint8>& DeltaPacket,
    FCoMGameStateSnapshot& OutNewState)
{
    // TODO: implement delta application to reconstruct NewState
    return false;
}
