// Copyright Mythforge Studios. All Rights Reserved.
// CoMAntiCheatValidator.cpp — Server-side command legality checks (stub)

#include "CoMAntiCheatValidator.h"
#include "CoMCore/Turn/CoMCommandQueue.h"

ECoMValidationResult UCoMAntiCheatValidator::Validate(
    const FCoMCommandPayload& Command,
    const FCoMGameStateSnapshot& AuthoritativeState,
    int32 SubmittingWizardIndex) const
{
    // TODO: implement validation logic (resource checks, turn ownership, target legality)
    return ECoMValidationResult::Valid;
}
