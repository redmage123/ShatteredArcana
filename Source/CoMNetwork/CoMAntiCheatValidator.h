// Copyright Mythforge Studios. All Rights Reserved.
// CoMAntiCheatValidator.h — Server-side command legality checks
// Owner: Network Architect

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoMAntiCheatValidator.generated.h"

struct FCoMCommandPayload;
struct FCoMGameStateSnapshot;

UENUM(BlueprintType)
enum class ECoMValidationResult : uint8
{
    Valid,
    IllegalTarget,
    InsufficientResources,
    OutOfTurn,
    Malformed,
};

UCLASS()
class COMNETWORK_API UCoMAntiCheatValidator : public UObject
{
    GENERATED_BODY()
public:
    // Validate a command against the authoritative server state.
    // All checks are pure / deterministic — no I/O.
    ECoMValidationResult Validate(const FCoMCommandPayload& Command,
                                  const FCoMGameStateSnapshot& AuthoritativeState,
                                  int32 SubmittingWizardIndex) const;
};
