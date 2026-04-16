// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacySubsystem.h — Diplomatic relations, treaties, reputation, AI diplomacy
// Phase 7 — Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMDiplomacySubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// DIPLOMATIC STATE BETWEEN TWO WIZARDS
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMDiplomaticRelation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WizardA = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WizardB = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMTreatyType CurrentTreaty = ECoMTreatyType::None;

    /** Reputation: -1000 (blood enemy) to +1000 (eternal ally). Decays toward 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Reputation = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TreatyTurns = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LastWarStartTurn = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TreatiesBroken = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Grievances;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GiftsGivenAtoB = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GiftsGivenBtoA = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFirstContactMade = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FirstContactTurn = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SharedEnemyBonus = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AggressionPenalty = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// TREATY PROPOSAL
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMTreatyProposal
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ProposalId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ProposerWizardId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetWizardId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMTreatyType ProposedTreaty = ECoMTreatyType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMResource, int32> OfferedResources;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OfferedMana = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Demands;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ProposalTurn = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bResponded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAccepted = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// AI DIPLOMATIC PERSONALITY
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMDiplomaticPersonality
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 Aggressiveness = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 Trustworthiness = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 TradeAffinity = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 Cunning = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 ScholarlyNature = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 Forgiveness = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMVictoryType PreferredVictory = ECoMVictoryType::None;
};

// ─────────────────────────────────────────────────────────────────────────────
// DIPLOMACY SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMDiplomacySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── Relationship Management ──────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    FCoMDiplomaticRelation& GetRelation(int32 WizardA, int32 WizardB);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    ECoMTreatyType GetTreatyBetween(int32 WizardA, int32 WizardB) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 GetReputation(int32 WizardA, int32 WizardB) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void ModifyReputation(int32 WizardA, int32 WizardB, int32 Delta, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool AreAtWar(int32 WizardA, int32 WizardB) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool AreAllied(int32 WizardA, int32 WizardB) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    TArray<int32> GetEnemies(int32 WizardId) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    TArray<int32> GetAllies(int32 WizardId) const;

    // ── Treaty Actions ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 ProposeTreaty(const FCoMTreatyProposal& Proposal);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool AcceptTreaty(int32 ProposalId);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void RejectTreaty(int32 ProposalId);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void BreakTreaty(int32 WizardA, int32 WizardB);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void DeclareWar(int32 AttackerId, int32 DefenderId);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 ProposePeace(int32 ProposerId, int32 TargetId,
                       const TMap<ECoMResource, int32>& Reparations);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void SendGift(int32 SenderId, int32 ReceiverId,
                  const TMap<ECoMResource, int32>& Resources, int32 Mana);

    // ── Spell Trading ────────────────────────────────────────────────────────

    /**
     * Propose a spell trade between two wizards.
     * Each wizard offers spells they know that the other doesn't.
     * Requires Peace or Alliance treaty. Improves reputation.
     * @return true if the trade was accepted (AI auto-evaluates; player gets UI prompt).
     */
    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool ProposeSpellTrade(int32 OffererWizardId, int32 TargetWizardId,
                           const TArray<FName>& OfferedSpells, const TArray<FName>& RequestedSpells);

    /**
     * Get spells that WizardA knows but WizardB doesn't (tradeable to B).
     * Only includes spells in realms where B has at least 1 book.
     */
    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    TArray<FName> GetTradeableSpells(int32 FromWizardId, int32 ToWizardId) const;

    /**
     * AI evaluates a spell trade offer. Returns true if acceptable.
     * Considers: spell rarity balance, reputation, personality, realm value.
     */
    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool AIEvaluateSpellTrade(int32 AIWizardId,
                              const TArray<FName>& OfferedToAI, const TArray<FName>& RequestedFromAI) const;

    // ── First Contact ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void MakeFirstContact(int32 WizardA, int32 WizardB, int32 CurrentTurn);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool HaveMet(int32 WizardA, int32 WizardB) const;

    // ── AI Diplomacy ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void SetPersonality(int32 WizardId, const FCoMDiplomaticPersonality& Personality);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 EvaluateProposal(int32 WizardId, const FCoMTreatyProposal& Proposal) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    ECoMDiplomacyAction DecideAIDiplomaticAction(int32 WizardId) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void ProcessAIDiplomacy(int32 CurrentTurn);

    // ── Turn Processing ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void ProcessTurn(int32 CurrentTurn);

    // ── War Weariness ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 GetWarWeariness(int32 WizardId, int32 CurrentTurn) const;

    // ── Casus Belli ──────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    bool HasCasusBelli(int32 AttackerId, int32 DefenderId) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    TArray<FString> GetCasusBelli(int32 AttackerId, int32 DefenderId) const;

    // ── Vassal System ────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void MakeVassal(int32 OverlordId, int32 VassalId);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    void FreeVassal(int32 VassalId);

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    int32 GetOverlord(int32 WizardId) const;

    UFUNCTION(BlueprintCallable, Category = "Diplomacy")
    TArray<int32> GetVassals(int32 OverlordId) const;

    // ── Save/Load Export/Import ───────────────────────────────────────────

    /** Export all diplomacy state for save serialization. */
    void ExportAll(TArray<FCoMDiplomaticRelation>& OutRelations,
                   TArray<FCoMTreatyProposal>& OutProposals,
                   TArray<FCoMDiplomaticPersonality>& OutPersonalities,
                   TArray<int32>& OutPersonalityWizardIDs) const;

    /** Import diplomacy state from save data (clears existing state). */
    void ImportAll(const TArray<FCoMDiplomaticRelation>& InRelations,
                   const TArray<FCoMTreatyProposal>& InProposals,
                   const TArray<FCoMDiplomaticPersonality>& InPersonalities,
                   const TArray<int32>& InPersonalityWizardIDs);

private:
    FIntPoint MakeRelationKey(int32 A, int32 B) const;
    void DecayReputations();
    void ExpireProposals(int32 CurrentTurn);
    void NotifyTreatyBroken(int32 Breaker, int32 Victim);

    UPROPERTY()
    TMap<FIntPoint, FCoMDiplomaticRelation> Relations;

    UPROPERTY()
    TArray<FCoMTreatyProposal> PendingProposals;

    UPROPERTY()
    TMap<int32, FCoMDiplomaticPersonality> Personalities;

    UPROPERTY()
    TMap<int32, int32> VassalMap;

    int32 NextProposalId = 1;
};
