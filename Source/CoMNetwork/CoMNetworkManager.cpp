// Copyright Mythforge Studios. All Rights Reserved.
// CoMNetworkManager.cpp — Session creation, connection state, lobby
// Owner: Network Architect
//
// Sprint 1 state: stubs only — logs + state transitions.
// Real OSS calls (EOS/LAN/Steam) deferred until transport layer selection.
// See: engineering/network/multiplayer-readiness.md §Transport Layer Choice

#include "CoMNetworkManager.h"
#include "CoreTypes/CoMConstants.h"

DEFINE_LOG_CATEGORY_STATIC(LogCoMNetwork, Log, All);

void UCoMNetworkManager::HostSession(int32 MaxWizards)
{
    if (State != ECoMNetworkState::Offline)
    {
        UE_LOG(LogCoMNetwork, Warning,
            TEXT("HostSession called while not Offline (current state: %d). Ignoring."),
            static_cast<int32>(State));
        return;
    }

    if (MaxWizards < 2 || MaxWizards > CoM::MAX_WIZARDS)
    {
        UE_LOG(LogCoMNetwork, Warning,
            TEXT("HostSession: MaxWizards=%d out of range [2, %d]. Clamping."),
            MaxWizards, CoM::MAX_WIZARDS);
        MaxWizards = FMath::Clamp(MaxWizards, 2, CoM::MAX_WIZARDS);
    }

    // TODO (Sprint 2+): IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
    // Session->CreateSession(0, NAME_GameSession, SessionSettings);
    // Bind OnCreateSessionComplete delegate before returning.

    UE_LOG(LogCoMNetwork, Log,
        TEXT("HostSession: MaxWizards=%d — transitioning Offline → Hosting. "
             "OSS session creation deferred pending transport selection (EOS/LAN/Steam)."),
        MaxWizards);

    State = ECoMNetworkState::Hosting;
}

void UCoMNetworkManager::JoinSession(const FString& SessionId)
{
    if (State != ECoMNetworkState::Offline)
    {
        UE_LOG(LogCoMNetwork, Warning,
            TEXT("JoinSession called while not Offline (current state: %d). Ignoring."),
            static_cast<int32>(State));
        return;
    }

    if (SessionId.IsEmpty())
    {
        UE_LOG(LogCoMNetwork, Warning, TEXT("JoinSession: empty SessionId. Ignoring."));
        return;
    }

    // TODO (Sprint 2+): IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
    // FOnlineSessionSearchResult SearchResult = FindSessionById(SessionId);
    // Session->JoinSession(0, NAME_GameSession, SearchResult);
    // Bind OnJoinSessionComplete delegate before returning.

    UE_LOG(LogCoMNetwork, Log,
        TEXT("JoinSession: SessionId='%s' — transitioning Offline → Connected. "
             "OSS join logic deferred pending transport selection."),
        *SessionId);

    State = ECoMNetworkState::Connected;
}

void UCoMNetworkManager::LeaveSession()
{
    if (State == ECoMNetworkState::Offline)
    {
        UE_LOG(LogCoMNetwork, Log, TEXT("LeaveSession: already Offline. No-op."));
        return;
    }

    // TODO (Sprint 2+): IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
    // Session->DestroySession(NAME_GameSession);

    UE_LOG(LogCoMNetwork, Log,
        TEXT("LeaveSession: transitioning %d → Offline."),
        static_cast<int32>(State));

    State = ECoMNetworkState::Offline;
}
