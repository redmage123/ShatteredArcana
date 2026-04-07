// Copyright Mythforge Studios. All Rights Reserved.
// CoMSubsystemBridge.h — Cross-subsystem event wiring
// Connects Diplomacy ↔ Espionage ↔ Magic via delegates
// Phase 7 — Shattered Arcana
//
// Usage: Create a UCoMSubsystemBridge in your GameInstance or GameMode
// and call WireSubsystems() after all subsystems are initialized.
// The bridge listens to events from each subsystem and dispatches
// the appropriate cross-subsystem calls.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMSubsystemBridge.generated.h"

class UCoMDiplomacySubsystem;
class UCoMEspionageSubsystem;
class UCoMMagicSubsystem;

/**
 * Wires cross-subsystem events that the individual subsystems
 * cannot handle themselves without circular dependencies.
 *
 * Examples:
 * - Spy caught → diplomatic reputation penalty
 * - Treaty broken → espionage agents expelled
 * - Spell stolen → add to wizard's known spells
 * - War declared → cancel active espionage missions against new ally
 */
UCLASS()
class COMCORE_API UCoMSubsystemBridge : public UObject
{
    GENERATED_BODY()

public:
    /** Wire all cross-subsystem event handlers. Call once after all subsystems initialized. */
    void WireSubsystems(
        UCoMDiplomacySubsystem* Diplomacy,
        UCoMEspionageSubsystem* Espionage,
        UCoMMagicSubsystem* Magic);

    /** Process cross-subsystem turn effects. Call after all individual ProcessTurn calls. */
    void ProcessCrossSubsystemTurn(int32 CurrentTurn);

private:
    // ── Event handlers ──────────────────────────────────────────────────────

    /** When a spy is caught: apply diplomatic penalty to spy's owner. */
    void OnSpyCaught(int32 CaptorWizardId, int32 SpyOwnerWizardId);

    /** When an agent executes a captured spy: diplomatic penalty. */
    void OnSpyExecuted(int32 ExecutorWizardId, int32 SpyOwnerWizardId);

    /** When a spy steals tech: add spell to spy owner's known spells. */
    void OnTechStolen(int32 ThiefWizardId, int32 VictimWizardId, const FName& StolenSpellId);

    /** When war is declared: cancel missions between new enemies. */
    void OnWarDeclared(int32 AttackerId, int32 DefenderId);

    /** When espionage propaganda succeeds: modify diplomatic reputation. */
    void OnPropagandaSuccess(int32 InstigatorWizardId, int32 TargetWizardId);

    /** When an enchantment is dispelled: potential casus belli. */
    void OnEnchantmentDispelled(int32 DispellerWizardId, int32 VictimWizardId);

    // ── References ────────────────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<UCoMDiplomacySubsystem> DiplomacySub;

    UPROPERTY()
    TObjectPtr<UCoMEspionageSubsystem> EspionageSub;

    UPROPERTY()
    TObjectPtr<UCoMMagicSubsystem> MagicSub;
};
