// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMTurnCoordinator.cpp — Simultaneous command collection phase coordinator.
//
// Lifecycle per turn:
//   1. UCoMTurnSubsystem calls BeginCommandPhase(N) at WorldProcessing start.
//   2. Human controllers, AI controllers, and UCoMNetworkProxy each call
//      MarkWizardReady() when their wizard submits EndTurn.
//   3. UCoMTurnSubsystem polls AllWizardsReady() before advancing to CombatResolution.
//   4. BroadcastLockPhase() signals all clients that command submission is closed.
//
// Thread safety: all methods called on the game thread only (UE5 guarantee for
// UObject subsystems). No locking needed.
//
// Owner: Network Architect

#include "CoMTurnCoordinator.h"

// ─────────────────────────────────────────────────────────────────────────────

void UCoMTurnCoordinator::BeginCommandPhase(int32 ActiveWizardCount)
{
    if (ActiveWizardCount <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMTurnCoordinator::BeginCommandPhase — ActiveWizardCount=%d is invalid. "
                 "Clamping to 1."),
            ActiveWizardCount);
        ActiveWizardCount = 1;
    }

    ExpectedWizards = ActiveWizardCount;
    ReadyFlags.Init(false, ActiveWizardCount);  // All slots start as not-ready.

    UE_LOG(LogTemp, Log,
        TEXT("UCoMTurnCoordinator: Command phase started. Waiting on %d wizards."),
        ExpectedWizards);
}

void UCoMTurnCoordinator::MarkWizardReady(int32 WizardIndex)
{
    if (!ReadyFlags.IsValidIndex(WizardIndex))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UCoMTurnCoordinator::MarkWizardReady — WizardIndex=%d out of range "
                 "(expected 0..%d). Ignoring."),
            WizardIndex, ExpectedWizards - 1);
        return;
    }

    if (ReadyFlags[WizardIndex])
    {
        // Duplicate ready signal — benign (network retry path), but worth logging.
        UE_LOG(LogTemp, Verbose,
            TEXT("UCoMTurnCoordinator::MarkWizardReady — Wizard %d already marked ready."),
            WizardIndex);
        return;
    }

    ReadyFlags[WizardIndex] = true;

    const int32 ReadyCount = [this](){ int32 N=0; for(bool b : ReadyFlags) if(b) N++; return N; }();
    UE_LOG(LogTemp, Log,
        TEXT("UCoMTurnCoordinator: Wizard %d ready. (%d / %d)"),
        WizardIndex, ReadyCount, ExpectedWizards);
}

bool UCoMTurnCoordinator::AllWizardsReady() const
{
    if (ReadyFlags.IsEmpty())
    {
        // BeginCommandPhase not yet called — treat as not ready.
        return false;
    }
    // All slots must be true.
    for (const bool bReady : ReadyFlags)
    {
        if (!bReady) { return false; }
    }
    return true;
}

void UCoMTurnCoordinator::BroadcastLockPhase()
{
    // Sprint 1 stub: log the lock signal. Real implementation (Sprint 2+) will
    // send a "LOCK" packet via UCoMNetworkProxy to all connected clients so they
    // stop accepting new commands for this turn.
    //
    // TODO (Sprint 2+): iterate registered UCoMNetworkProxy list and call
    //   Proxy->SendLockPhaseSignal(CurrentTurnNumber);
    UE_LOG(LogTemp, Log,
        TEXT("UCoMTurnCoordinator::BroadcastLockPhase — Lock signal sent "
             "(stub: %d wizards notified)."),
        ExpectedWizards);
}
