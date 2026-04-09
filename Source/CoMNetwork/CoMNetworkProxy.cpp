// Copyright Mythforge Studios. All Rights Reserved.
// CoMNetworkProxy.cpp — Remote player command proxy (stub)

#include "CoMNetworkProxy.h"
#include "CoMCommandSerializer.h"
#include "CoMCore/Turn/CoMCommandQueue.h"

void UCoMNetworkProxy::ReceiveRemoteCommand(int32 InWizardIndex, const TArray<uint8>& WirePacket)
{
    // TODO: deserialize WirePacket via UCoMCommandSerializer, validate, and enqueue
    FCoMCommandPayload Payload;
    if (!UCoMCommandSerializer::Deserialize(WirePacket, Payload))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMNetworkProxy::ReceiveRemoteCommand — failed to deserialize packet for Wizard %d"),
            InWizardIndex);
        return;
    }

    // TODO: enqueue Payload into the command queue for the appropriate turn
}
