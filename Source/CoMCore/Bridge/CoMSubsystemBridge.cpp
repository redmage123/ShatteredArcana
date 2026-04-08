// Copyright Mythforge Studios. All Rights Reserved.
// CoMSubsystemBridge.cpp — Cross-subsystem event wiring implementation
// Phase 7 — Shattered Arcana

#include "CoMSubsystemBridge.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"


void UCoMSubsystemBridge::WireSubsystems(
    UCoMDiplomacySubsystem* Diplomacy,
    UCoMEspionageSubsystem* Espionage,
    UCoMMagicSubsystem* Magic)
{
    DiplomacySub = Diplomacy;
    EspionageSub = Espionage;
    MagicSub = Magic;

    // NOTE: In a full implementation, each subsystem would fire delegates
    // for key events (OnSpyCaptured, OnWarDeclared, etc.) and we would
    // bind to them here. For now, ProcessCrossSubsystemTurn handles
    // the integration by checking state each turn.
    //
    // Example of how delegate binding would look:
    // Espionage->OnAgentCaptured.AddDynamic(this, &UCoMSubsystemBridge::HandleSpyCaught);
    // Diplomacy->OnWarDeclared.AddDynamic(this, &UCoMSubsystemBridge::HandleWarDeclared);
}

void UCoMSubsystemBridge::ProcessCrossSubsystemTurn(int32 CurrentTurn)
{
    if (!DiplomacySub || !EspionageSub || !MagicSub) return;

    // ── Espionage → Diplomacy: detected spy missions create reputation penalties ──
    // Check recent mission results for detected missions
    // For each wizard, check if their spies were caught this turn
    for (int32 WizardId = 0; WizardId < 16; ++WizardId) // Max 16 wizards
    {
        TArray<FCoMMissionResult> History = EspionageSub->GetMissionHistory(WizardId, 5);
        for (const FCoMMissionResult& Result : History)
        {
            if (Result.CompletedTurn != CurrentTurn) continue;

            if (Result.bDetected && !Result.bSuccess)
            {
                // Spy was detected — diplomatic penalty
                // Lookup agent from results
					FCoMSpyAgent* Agent = nullptr; // TODO: wire to espionage API
                if (Agent)
                {
                    OnSpyCaught(Agent->TargetWizardId, Agent->OwnerWizardId);
                }
            }

            // Successful tech theft → add spell to thief's repertoire
            if (Result.bSuccess && Result.Mission == ECoMAgentMission::Steal)
            {
                // Lookup agent from results
					FCoMSpyAgent* Agent = nullptr; // TODO: wire to espionage API
                if (Agent && Result.StolenIntel.Num() > 0)
                {
                    // Pick a random known spell from the victim and give it to the thief
                    FCoMWizardMagicState& VictimMagic = MagicSub->GetWizardMagic(Agent->TargetWizardId);
                    if (VictimMagic.KnownSpells.Num() > 0)
                    {
                        int32 RandIdx = FMath::RandRange(0, VictimMagic.KnownSpells.Num() - 1);
                        FName StolenSpell = VictimMagic.KnownSpells[RandIdx];
                        OnTechStolen(Agent->OwnerWizardId, Agent->TargetWizardId, StolenSpell);
                    }
                }
            }

            // Successful propaganda → reputation shift
            if (Result.bSuccess && Result.Mission == ECoMAgentMission::PropagandaCampaign)
            {
                // Lookup agent from results
					FCoMSpyAgent* Agent = nullptr; // TODO: wire to espionage API
                if (Agent)
                {
                    OnPropagandaSuccess(Agent->OwnerWizardId, Agent->TargetWizardId);
                }
            }
        }
    }

    // ── Magic → Diplomacy: enchantment maintenance as soft power ──
    // Wizards with more active enchantments project power,
    // which can intimidate neighbors or attract allies
    // (This is a design hook — implement when combat system is ready)

    // ── War weariness → AI peace proposals ──
    // If war weariness is very high, AI should propose peace
    for (int32 WizardId = 0; WizardId < 16; ++WizardId)
    {
        int32 Weariness = DiplomacySub->GetWarWeariness(WizardId, CurrentTurn);
        if (Weariness > 200) // Very war-weary
        {
            TArray<int32> Enemies = DiplomacySub->GetEnemies(WizardId);
            for (int32 EnemyId : Enemies)
            {
                // AI auto-proposes peace when extremely war-weary
                TMap<ECoMResource, int32> NoReparations;
                DiplomacySub->ProposePeace(WizardId, EnemyId, NoReparations);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// EVENT HANDLERS
// ─────────────────────────────────────────────────────────────────────────────

void UCoMSubsystemBridge::OnSpyCaught(int32 CaptorWizardId, int32 SpyOwnerWizardId)
{
    if (!DiplomacySub) return;

    // Getting caught spying is a serious diplomatic offense
    DiplomacySub->ModifyReputation(CaptorWizardId, SpyOwnerWizardId, -75,
        TEXT("Spy agent caught"));

    // Creates casus belli via aggression penalty
    FCoMDiplomaticRelation& Rel = DiplomacySub->GetRelation(CaptorWizardId, SpyOwnerWizardId);
    Rel.AggressionPenalty += 60;
}

void UCoMSubsystemBridge::OnSpyExecuted(int32 ExecutorWizardId, int32 SpyOwnerWizardId)
{
    if (!DiplomacySub) return;

    // Executing a spy is harsher than just catching one
    DiplomacySub->ModifyReputation(ExecutorWizardId, SpyOwnerWizardId, -50,
        TEXT("Executed our agent"));

    // But the executor also loses some standing with neutral parties
    // (executing prisoners is frowned upon)
    // This is handled by NotifyTreatyBroken-style global penalty
    // but less severe — just -10 with everyone who knows
}

void UCoMSubsystemBridge::OnTechStolen(int32 ThiefWizardId, int32 VictimWizardId, const FName& StolenSpellId)
{
    if (!MagicSub) return;

    FCoMWizardMagicState& ThiefMagic = MagicSub->GetWizardMagic(ThiefWizardId);
    if (!ThiefMagic.KnownSpells.Contains(StolenSpellId))
    {
        ThiefMagic.KnownSpells.Add(StolenSpellId);
    }
}

void UCoMSubsystemBridge::OnWarDeclared(int32 AttackerId, int32 DefenderId)
{
    if (!EspionageSub) return;

    // When war is declared, cancel any friendly espionage missions
    // (you're now openly at war, no need for subtlety)
    // This doesn't apply — you'd still spy during war.
    // But allies of the defender should expel the attacker's spies.
}

void UCoMSubsystemBridge::OnPropagandaSuccess(int32 InstigatorWizardId, int32 TargetWizardId)
{
    if (!DiplomacySub) return;

    // Successful propaganda lowers the target wizard's reputation with neighbors
    // Simulate: all wizards who have met the target get a small negative nudge
    for (int32 OtherId = 0; OtherId < 16; ++OtherId)
    {
        if (OtherId == InstigatorWizardId || OtherId == TargetWizardId) continue;
        if (DiplomacySub->HaveMet(OtherId, TargetWizardId))
        {
            DiplomacySub->ModifyReputation(OtherId, TargetWizardId, -15,
                TEXT("Negative propaganda"));
        }
    }
}

void UCoMSubsystemBridge::OnEnchantmentDispelled(int32 DispellerWizardId, int32 VictimWizardId)
{
    if (!DiplomacySub) return;

    // Dispelling someone's enchantment is an act of magical aggression
    DiplomacySub->ModifyReputation(DispellerWizardId, VictimWizardId, -40,
        TEXT("Dispelled our enchantment"));

    FCoMDiplomaticRelation& Rel = DiplomacySub->GetRelation(DispellerWizardId, VictimWizardId);
    Rel.AggressionPenalty += 30;
}
