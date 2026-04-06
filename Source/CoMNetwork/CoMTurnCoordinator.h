// Copyright Mythforge Studios. All Rights Reserved.
// CoMTurnCoordinator.h — Hosts simultaneous turn phase; collects all wizard queues
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoMTurnCoordinator.generated.h"

UCLASS()
class COMNETWORK_API UCoMTurnCoordinator : public UObject
{
    GENERATED_BODY()
public:
    // Begin the command-collection phase for a new turn.
    void BeginCommandPhase(int32 ActiveWizardCount);

    // Called when a wizard (human, AI, or network proxy) signals ready.
    void MarkWizardReady(int32 WizardIndex);

    // Returns true when all wizards have submitted commands.
    bool AllWizardsReady() const;

    // Broadcast lock signal to all connected clients.
    void BroadcastLockPhase();

private:
    int32 ExpectedWizards = 0;
    TArray<bool> ReadyFlags; // indexed by wizard slot
};
