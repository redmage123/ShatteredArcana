// Copyright Mythforge Studios. All Rights Reserved.
// CoMCommandSerializer.h — FCoMCommandPayload <-> wire format (compact binary)
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoMCommandSerializer.generated.h"

struct FCoMCommandPayload;

UCLASS()
class COMNETWORK_API UCoMCommandSerializer : public UObject
{
    GENERATED_BODY()
public:
    // Serialize payload to compact wire bytes.
    static TArray<uint8> Serialize(const FCoMCommandPayload& Payload);
    // Deserialize wire bytes back to a payload. Returns false on malformed input.
    static bool Deserialize(const TArray<uint8>& Wire, FCoMCommandPayload& OutPayload);
};
