// Copyright Mythforge Studios. All Rights Reserved.
// CoMNetworkProxy.h — Implements ICoMPlayerCommand for remote players
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoMNetworkProxy.generated.h"

// ICoMPlayerCommand forward — defined in CoMCore/Turn/CoMCommandQueue.h
class ICoMPlayerCommand;
struct FCoMCommandPayload;

UCLASS()
class COMNETWORK_API UCoMNetworkProxy : public UObject
{
    GENERATED_BODY()
public:
    // Receives a wire packet and enqueues the decoded command for the wizard slot.
    void ReceiveRemoteCommand(int32 WizardIndex, const TArray<uint8>& WirePacket);

    UPROPERTY() int32 WizardIndex = -1;
};
