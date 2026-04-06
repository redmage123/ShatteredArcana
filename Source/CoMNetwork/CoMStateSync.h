// Copyright Mythforge Studios. All Rights Reserved.
// CoMStateSync.h — Delta-encodes FCoMGameStateSnapshot for reconnects/spectators
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoMStateSync.generated.h"

struct FCoMGameStateSnapshot;

UCLASS()
class COMNETWORK_API UCoMStateSync : public UObject
{
    GENERATED_BODY()
public:
    // Produce a delta packet from BaseState -> NewState.
    TArray<uint8> EncodeDelta(const FCoMGameStateSnapshot& BaseState,
                              const FCoMGameStateSnapshot& NewState);

    // Apply a delta packet to BaseState; returns the reconstructed new state.
    bool ApplyDelta(const FCoMGameStateSnapshot& BaseState,
                    const TArray<uint8>& DeltaPacket,
                    FCoMGameStateSnapshot& OutNewState);
};
