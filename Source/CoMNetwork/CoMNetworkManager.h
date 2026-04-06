// Copyright Mythforge Studios. All Rights Reserved.
// CoMNetworkManager.h — Session creation, connection state, lobby
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMNetworkManager.generated.h"

UENUM(BlueprintType)
enum class ECoMNetworkState : uint8
{
    Offline,
    Hosting,
    Connected,
    Reconnecting,
};

UCLASS()
class COMNETWORK_API UCoMNetworkManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void HostSession(int32 MaxWizards);
    UFUNCTION(BlueprintCallable) void JoinSession(const FString& SessionId);
    UFUNCTION(BlueprintCallable) void LeaveSession();
    UFUNCTION(BlueprintPure)     ECoMNetworkState GetState() const { return State; }

private:
    ECoMNetworkState State = ECoMNetworkState::Offline;
};
