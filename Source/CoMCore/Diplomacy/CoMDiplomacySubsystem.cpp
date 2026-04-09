// Copyright Mythforge Studios. All Rights Reserved.
// CoMDiplomacySubsystem.cpp — Full implementation
// Phase 7 — Shattered Arcana

#include "CoMDiplomacySubsystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

void UCoMDiplomacySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Relations.Empty();
    PendingProposals.Empty();
    Personalities.Empty();
    VassalMap.Empty();
    NextProposalId = 1;
}

void UCoMDiplomacySubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

FIntPoint UCoMDiplomacySubsystem::MakeRelationKey(int32 A, int32 B) const
{
    return FIntPoint(FMath::Min(A, B), FMath::Max(A, B));
}

// ─────────────────────────────────────────────────────────────────────────────
// RELATIONSHIP MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

FCoMDiplomaticRelation& UCoMDiplomacySubsystem::GetRelation(int32 WizardA, int32 WizardB)
{
    FIntPoint Key = MakeRelationKey(WizardA, WizardB);
    if (!Relations.Contains(Key))
    {
        FCoMDiplomaticRelation NewRelation;
        NewRelation.WizardA = Key.X;
        NewRelation.WizardB = Key.Y;
        Relations.Add(Key, NewRelation);
    }
    return Relations[Key];
}

ECoMTreatyType UCoMDiplomacySubsystem::GetTreatyBetween(int32 WizardA, int32 WizardB) const
{
    FIntPoint Key = MakeRelationKey(WizardA, WizardB);
    if (const FCoMDiplomaticRelation* Rel = Relations.Find(Key))
    {
        return Rel->CurrentTreaty;
    }
    return ECoMTreatyType::None;
}

int32 UCoMDiplomacySubsystem::GetReputation(int32 WizardA, int32 WizardB) const
{
    FIntPoint Key = MakeRelationKey(WizardA, WizardB);
    if (const FCoMDiplomaticRelation* Rel = Relations.Find(Key))
    {
        return Rel->Reputation;
    }
    return 0;
}

void UCoMDiplomacySubsystem::ModifyReputation(int32 WizardA, int32 WizardB, int32 Delta, const FString& Reason)
{
    FCoMDiplomaticRelation& Rel = GetRelation(WizardA, WizardB);
    Rel.Reputation = FMath::Clamp(Rel.Reputation + Delta, -1000, 1000);

    if (!Reason.IsEmpty())
    {
        Rel.Grievances.Add(FString::Printf(TEXT("Turn: Rep %+d — %s"), Delta, *Reason));
        // Keep grievance list manageable
        if (Rel.Grievances.Num() > 20)
        {
            Rel.Grievances.RemoveAt(0);
        }
    }
}

bool UCoMDiplomacySubsystem::AreAtWar(int32 WizardA, int32 WizardB) const
{
    return GetTreatyBetween(WizardA, WizardB) == ECoMTreatyType::War;
}

bool UCoMDiplomacySubsystem::AreAllied(int32 WizardA, int32 WizardB) const
{
    ECoMTreatyType Treaty = GetTreatyBetween(WizardA, WizardB);
    return Treaty == ECoMTreatyType::MilitaryAlliance || Treaty == ECoMTreatyType::DefensivePact;
}

TArray<int32> UCoMDiplomacySubsystem::GetEnemies(int32 WizardId) const
{
    TArray<int32> Enemies;
    for (const auto& Pair : Relations)
    {
        if (Pair.Value.CurrentTreaty == ECoMTreatyType::War)
        {
            if (Pair.Value.WizardA == WizardId) Enemies.Add(Pair.Value.WizardB);
            else if (Pair.Value.WizardB == WizardId) Enemies.Add(Pair.Value.WizardA);
        }
    }
    return Enemies;
}

TArray<int32> UCoMDiplomacySubsystem::GetAllies(int32 WizardId) const
{
    TArray<int32> Allies;
    for (const auto& Pair : Relations)
    {
        if (Pair.Value.CurrentTreaty == ECoMTreatyType::MilitaryAlliance ||
            Pair.Value.CurrentTreaty == ECoMTreatyType::DefensivePact)
        {
            if (Pair.Value.WizardA == WizardId) Allies.Add(Pair.Value.WizardB);
            else if (Pair.Value.WizardB == WizardId) Allies.Add(Pair.Value.WizardA);
        }
    }
    return Allies;
}

// ─────────────────────────────────────────────────────────────────────────────
// TREATY ACTIONS
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMDiplomacySubsystem::ProposeTreaty(const FCoMTreatyProposal& Proposal)
{
    FCoMTreatyProposal NewProposal = Proposal;
    NewProposal.ProposalId = NextProposalId++;
    NewProposal.bResponded = false;
    NewProposal.bAccepted = false;
    PendingProposals.Add(NewProposal);
    return NewProposal.ProposalId;
}

bool UCoMDiplomacySubsystem::AcceptTreaty(int32 ProposalId)
{
    for (FCoMTreatyProposal& Prop : PendingProposals)
    {
        if (Prop.ProposalId == ProposalId && !Prop.bResponded)
        {
            Prop.bResponded = true;
            Prop.bAccepted = true;

            // Apply the treaty
            FCoMDiplomaticRelation& Rel = GetRelation(Prop.ProposerWizardId, Prop.TargetWizardId);
            ECoMTreatyType OldTreaty = Rel.CurrentTreaty;
            Rel.CurrentTreaty = Prop.ProposedTreaty;
            Rel.TreatyTurns = 0;

            // Reputation boost for accepting
            ModifyReputation(Prop.ProposerWizardId, Prop.TargetWizardId, 50,
                TEXT("Treaty accepted"));

            // If peace treaty, end war
            if (Prop.ProposedTreaty == ECoMTreatyType::NonAggression)
            {
                if (OldTreaty == ECoMTreatyType::War)
                {
                    Rel.LastWarStartTurn = -1;
                }
            }

            return true;
        }
    }
    return false;
}

void UCoMDiplomacySubsystem::RejectTreaty(int32 ProposalId)
{
    for (FCoMTreatyProposal& Prop : PendingProposals)
    {
        if (Prop.ProposalId == ProposalId && !Prop.bResponded)
        {
            Prop.bResponded = true;
            Prop.bAccepted = false;

            // Small reputation hit for rejection
            ModifyReputation(Prop.ProposerWizardId, Prop.TargetWizardId, -10,
                TEXT("Treaty proposal rejected"));
            return;
        }
    }
}

void UCoMDiplomacySubsystem::BreakTreaty(int32 WizardA, int32 WizardB)
{
    FCoMDiplomaticRelation& Rel = GetRelation(WizardA, WizardB);

    if (Rel.CurrentTreaty == ECoMTreatyType::None || Rel.CurrentTreaty == ECoMTreatyType::War)
    {
        return; // Nothing to break
    }

    ECoMTreatyType BrokenTreaty = Rel.CurrentTreaty;
    Rel.CurrentTreaty = ECoMTreatyType::None;
    Rel.TreatiesBroken++;
    Rel.TreatyTurns = 0;

    // Severe reputation penalty based on treaty type
    int32 Penalty = -100;
    if (BrokenTreaty == ECoMTreatyType::MilitaryAlliance) Penalty = -300;
    else if (BrokenTreaty == ECoMTreatyType::DefensivePact) Penalty = -250;
    else if (BrokenTreaty == ECoMTreatyType::NonAggression) Penalty = -150;
    else if (BrokenTreaty == ECoMTreatyType::TradeAgreement) Penalty = -75;

    // Extra penalty for repeat offenders
    Penalty -= Rel.TreatiesBroken * 25;

    ModifyReputation(WizardA, WizardB, Penalty,
        FString::Printf(TEXT("Broke %d treaty"), static_cast<int32>(BrokenTreaty)));

    // Notify all other wizards — treaty breakers get reputation hits globally
    NotifyTreatyBroken(WizardA, WizardB);
}

void UCoMDiplomacySubsystem::DeclareWar(int32 AttackerId, int32 DefenderId)
{
    FCoMDiplomaticRelation& Rel = GetRelation(AttackerId, DefenderId);

    if (Rel.CurrentTreaty == ECoMTreatyType::War)
    {
        return; // Already at war
    }

    // If breaking an existing treaty, apply treaty-break penalties first
    if (Rel.CurrentTreaty != ECoMTreatyType::None && Rel.CurrentTreaty != ECoMTreatyType::War)
    {
        BreakTreaty(AttackerId, DefenderId);
    }

    Rel.CurrentTreaty = ECoMTreatyType::War;
    Rel.LastWarStartTurn = 0; // Will be set properly in ProcessTurn
    Rel.TreatyTurns = 0;

    // Base war declaration penalty
    int32 Penalty = -200;

    // Extra penalty if no casus belli
    if (!HasCasusBelli(AttackerId, DefenderId))
    {
        Penalty -= 100;
        Rel.Grievances.Add(TEXT("Unprovoked war declaration"));

        // All other wizards lose respect for unprovoked aggression
        // Collect IDs first to avoid TMap iterator invalidation from ModifyReputation
        TArray<int32> OtherIds;
        for (const auto& Pair : Relations)
        {
            int32 OtherId = -1;
            if (Pair.Value.WizardA == AttackerId) OtherId = Pair.Value.WizardB;
            else if (Pair.Value.WizardB == AttackerId) OtherId = Pair.Value.WizardA;

            if (OtherId >= 0 && OtherId != DefenderId)
            {
                OtherIds.Add(OtherId);
            }
        }
        for (int32 OtherId : OtherIds)
        {
            ModifyReputation(AttackerId, OtherId, -50,
                TEXT("Unprovoked war against neighbor"));
        }
    }

    ModifyReputation(AttackerId, DefenderId, Penalty, TEXT("War declared"));

    // Allies of defender may join the war
    TArray<int32> DefenderAllies = GetAllies(DefenderId);
    for (int32 AllyId : DefenderAllies)
    {
        if (AllyId != AttackerId)
        {
            // Mutual defense pact triggers automatic war
            if (GetTreatyBetween(DefenderId, AllyId) == ECoMTreatyType::DefensivePact)
            {
                FCoMDiplomaticRelation& AllyRel = GetRelation(AttackerId, AllyId);
                AllyRel.CurrentTreaty = ECoMTreatyType::War;
                AllyRel.LastWarStartTurn = 0;
                ModifyReputation(AttackerId, AllyId, -150,
                    TEXT("Attacked our ally (mutual defense triggered)"));
            }
        }
    }
}

int32 UCoMDiplomacySubsystem::ProposePeace(int32 ProposerId, int32 TargetId,
                                             const TMap<ECoMResource, int32>& Reparations)
{
    FCoMTreatyProposal Proposal;
    Proposal.ProposerWizardId = ProposerId;
    Proposal.TargetWizardId = TargetId;
    Proposal.ProposedTreaty = ECoMTreatyType::NonAggression;
    Proposal.OfferedResources = Reparations;
    return ProposeTreaty(Proposal);
}

void UCoMDiplomacySubsystem::SendGift(int32 SenderId, int32 ReceiverId,
                                       const TMap<ECoMResource, int32>& Resources, int32 Mana)
{
    FCoMDiplomaticRelation& Rel = GetRelation(SenderId, ReceiverId);

    // Calculate gift value
    int32 GiftValue = Mana;
    for (const auto& Pair : Resources)
    {
        GiftValue += Pair.Value * 10; // Each resource unit worth ~10 rep points
    }

    // Diminishing returns — each subsequent gift is worth less (integer math)
    int32 PriorGifts = (SenderId == Rel.WizardA) ? Rel.GiftsGivenAtoB : Rel.GiftsGivenBtoA;
    int32 RepBonus = FMath::Min(GiftValue * 100 / (100 + PriorGifts * 20), 100);

    ModifyReputation(SenderId, ReceiverId, RepBonus,
        FString::Printf(TEXT("Gift worth %d"), GiftValue));

    if (SenderId == Rel.WizardA) Rel.GiftsGivenAtoB++;
    else Rel.GiftsGivenBtoA++;
}

// ─────────────────────────────────────────────────────────────────────────────
// FIRST CONTACT
// ─────────────────────────────────────────────────────────────────────────────

void UCoMDiplomacySubsystem::MakeFirstContact(int32 WizardA, int32 WizardB, int32 CurrentTurn)
{
    FCoMDiplomaticRelation& Rel = GetRelation(WizardA, WizardB);
    if (!Rel.bFirstContactMade)
    {
        Rel.bFirstContactMade = true;
        Rel.FirstContactTurn = CurrentTurn;
        // Neutral starting reputation
        Rel.Reputation = 0;
    }
}

bool UCoMDiplomacySubsystem::HaveMet(int32 WizardA, int32 WizardB) const
{
    FIntPoint Key = MakeRelationKey(WizardA, WizardB);
    if (const FCoMDiplomaticRelation* Rel = Relations.Find(Key))
    {
        return Rel->bFirstContactMade;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// AI DIPLOMACY
// ─────────────────────────────────────────────────────────────────────────────

void UCoMDiplomacySubsystem::SetPersonality(int32 WizardId, const FCoMDiplomaticPersonality& Personality)
{
    Personalities.Add(WizardId, Personality);
}

int32 UCoMDiplomacySubsystem::EvaluateProposal(int32 WizardId, const FCoMTreatyProposal& Proposal) const
{
    int32 Score = 0;

    const FCoMDiplomaticPersonality* PersonalityPtr = Personalities.Find(WizardId);
    FCoMDiplomaticPersonality Personality;
    if (PersonalityPtr) Personality = *PersonalityPtr;

    // Base score from reputation
    int32 Rep = GetReputation(WizardId, Proposal.ProposerWizardId);
    Score += Rep / 10; // -100 to +100

    // Treaty type preferences
    switch (Proposal.ProposedTreaty)
    {
    case ECoMTreatyType::TradeAgreement:
        Score += Personality.TradeAffinity - 30; // Trade-lovers accept more easily
        break;
    case ECoMTreatyType::NonAggression:
        Score += 20; // Generally safe to accept
        break;
    case ECoMTreatyType::MilitaryAlliance:
        Score += (Personality.Trustworthiness - 50); // Trustworthy AIs value alliances
        Score -= Personality.Aggressiveness / 3; // Aggressive AIs don't like being tied down
        break;
    case ECoMTreatyType::DefensivePact:
        Score += 10;
        // Check if there's a shared enemy
        {
            TArray<int32> MyEnemies;
            for (const auto& Pair : Relations)
            {
                if (Pair.Value.CurrentTreaty == ECoMTreatyType::War)
                {
                    if (Pair.Value.WizardA == WizardId) MyEnemies.Add(Pair.Value.WizardB);
                    else if (Pair.Value.WizardB == WizardId) MyEnemies.Add(Pair.Value.WizardA);
                }
            }
            for (const auto& Pair : Relations)
            {
                if (Pair.Value.CurrentTreaty == ECoMTreatyType::War)
                {
                    int32 EnemyId = -1;
                    if (Pair.Value.WizardA == Proposal.ProposerWizardId) EnemyId = Pair.Value.WizardB;
                    else if (Pair.Value.WizardB == Proposal.ProposerWizardId) EnemyId = Pair.Value.WizardA;
                    if (MyEnemies.Contains(EnemyId)) Score += 50; // Shared enemy bonus
                }
            }
        }
        break;
    case ECoMTreatyType::None:
        {
            // War weariness makes peace more attractive
            // (Simplified — would need CurrentTurn parameter for full calculation)
            Score += 30;
            if (Personality.Aggressiveness < 40) Score += 20;
        }
        break;
    default:
        break;
    }

    // Resource sweeteners
    int32 ResourceValue = Proposal.OfferedMana;
    for (const auto& Pair : Proposal.OfferedResources)
    {
        ResourceValue += Pair.Value * 10;
    }
    Score += ResourceValue / 5;

    // Trust factor — broken treaties make AI suspicious
    FIntPoint Key = MakeRelationKey(WizardId, Proposal.ProposerWizardId);
    if (const FCoMDiplomaticRelation* Rel = Relations.Find(Key))
    {
        Score -= Rel->TreatiesBroken * 30;
    }

    // Forgiveness modifies grudge impact
    Score += Personality.Forgiveness / 5;

    return Score;
}

ECoMDiplomacyAction UCoMDiplomacySubsystem::DecideAIDiplomaticAction(int32 WizardId) const
{
    const FCoMDiplomaticPersonality* PersonalityPtr = Personalities.Find(WizardId);
    if (!PersonalityPtr) return ECoMDiplomacyAction::MAX;

    const FCoMDiplomaticPersonality& P = *PersonalityPtr;

    // Count current wars and alliances
    int32 NumWars = GetEnemies(WizardId).Num();
    int32 NumAllies = GetAllies(WizardId).Num();

    // Too many wars — seek peace
    if (NumWars >= 2 && P.Aggressiveness < 80)
    {
        return ECoMDiplomacyAction::OfferPeace;
    }

    // No allies and aggressive — seek alliance against strong neighbor
    if (NumAllies == 0 && P.TradeAffinity > 30)
    {
        return ECoMDiplomacyAction::ProposeTreaty;
    }

    // High trade affinity — propose trade
    if (P.TradeAffinity > 60 && NumWars == 0)
    {
        return ECoMDiplomacyAction::ProposeResourceTrade;
    }

    // Aggressive with no wars — consider declaring war
    if (P.Aggressiveness > 70 && NumWars == 0)
    {
        return ECoMDiplomacyAction::DeclareWar;
    }

    // Cunning — use espionage instead of diplomacy
    if (P.Cunning > 60)
    {
        return ECoMDiplomacyAction::ThreatenWar;
    }

    return ECoMDiplomacyAction::OfferGift;
}

void UCoMDiplomacySubsystem::ProcessAIDiplomacy(int32 CurrentTurn)
{
    // Process pending proposals for AI wizards
    for (FCoMTreatyProposal& Prop : PendingProposals)
    {
        if (Prop.bResponded) continue;

        // Check if target is AI
        if (Personalities.Contains(Prop.TargetWizardId))
        {
            int32 Score = EvaluateProposal(Prop.TargetWizardId, Prop);
            if (Score > 0)
            {
                AcceptTreaty(Prop.ProposalId);
            }
            else
            {
                RejectTreaty(Prop.ProposalId);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING
// ─────────────────────────────────────────────────────────────────────────────

void UCoMDiplomacySubsystem::ProcessTurn(int32 CurrentTurn)
{
    // 1. Increment treaty duration
    for (auto& Pair : Relations)
    {
        if (Pair.Value.CurrentTreaty != ECoMTreatyType::None)
        {
            Pair.Value.TreatyTurns++;
        }
        // Update war start turn if needed
        if (Pair.Value.CurrentTreaty == ECoMTreatyType::War && Pair.Value.LastWarStartTurn <= 0)
        {
            Pair.Value.LastWarStartTurn = CurrentTurn;
        }
    }

    // 2. Decay reputations toward zero
    DecayReputations();

    // 3. Decay temporary modifiers
    for (auto& Pair : Relations)
    {
        if (Pair.Value.SharedEnemyBonus > 0)
            Pair.Value.SharedEnemyBonus = FMath::Max(0, Pair.Value.SharedEnemyBonus - 5);
        if (Pair.Value.AggressionPenalty > 0)
            Pair.Value.AggressionPenalty = FMath::Max(0, Pair.Value.AggressionPenalty - 3);
    }

    // 4. Expire old proposals
    ExpireProposals(CurrentTurn);

    // 5. Process AI diplomacy
    ProcessAIDiplomacy(CurrentTurn);

    // 6. Trade treaties generate passive reputation gain
    // Collect trade pairs first to avoid TMap iterator invalidation from ModifyReputation
    TArray<TPair<int32, int32>> TradePairs;
    for (const auto& Pair : Relations)
    {
        if (Pair.Value.CurrentTreaty == ECoMTreatyType::TradeAgreement)
        {
            TradePairs.Add(TPair<int32, int32>(Pair.Value.WizardA, Pair.Value.WizardB));
        }
    }
    for (const auto& TradePair : TradePairs)
    {
        ModifyReputation(TradePair.Key, TradePair.Value, 2,
            TEXT("Ongoing trade"));
    }
}

void UCoMDiplomacySubsystem::DecayReputations()
{
    for (auto& Pair : Relations)
    {
        int32& Rep = Pair.Value.Reputation;
        if (Rep > 0) Rep = FMath::Max(0, Rep - 1);
        else if (Rep < 0) Rep = FMath::Min(0, Rep + 1);
    }
}

void UCoMDiplomacySubsystem::ExpireProposals(int32 CurrentTurn)
{
    PendingProposals.RemoveAll([CurrentTurn](const FCoMTreatyProposal& Prop)
    {
        return Prop.bResponded || (CurrentTurn - Prop.ProposalTurn > 5);
    });
}

void UCoMDiplomacySubsystem::NotifyTreatyBroken(int32 Breaker, int32 Victim)
{
    // Collect IDs first to avoid TMap iterator invalidation from ModifyReputation
    TArray<int32> OtherIds;
    for (const auto& Pair : Relations)
    {
        int32 OtherId = -1;
        if (Pair.Value.WizardA == Breaker && Pair.Value.WizardB != Victim)
            OtherId = Pair.Value.WizardB;
        else if (Pair.Value.WizardB == Breaker && Pair.Value.WizardA != Victim)
            OtherId = Pair.Value.WizardA;

        if (OtherId >= 0 && Pair.Value.bFirstContactMade)
        {
            OtherIds.Add(OtherId);
        }
    }
    for (int32 OtherId : OtherIds)
    {
        // Treaty breakers lose trust with everyone who knows about it
        ModifyReputation(Breaker, OtherId, -30,
            TEXT("Broke treaty with another wizard"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WAR WEARINESS
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMDiplomacySubsystem::GetWarWeariness(int32 WizardId, int32 CurrentTurn) const
{
    int32 TotalWeariness = 0;

    for (const auto& Pair : Relations)
    {
        if (Pair.Value.CurrentTreaty != ECoMTreatyType::War) continue;
        if (Pair.Value.WizardA != WizardId && Pair.Value.WizardB != WizardId) continue;

        int32 WarDuration = CurrentTurn - Pair.Value.LastWarStartTurn;
        if (WarDuration < 0) WarDuration = 0;

        // Weariness increases quadratically with war duration
        // Starts at 0, reaches 100 after ~20 turns of war
        TotalWeariness += FMath::Min(100, (WarDuration * WarDuration) / 4);
    }

    return FMath::Min(TotalWeariness, 500); // Cap at 500
}

// ─────────────────────────────────────────────────────────────────────────────
// CASUS BELLI
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMDiplomacySubsystem::HasCasusBelli(int32 AttackerId, int32 DefenderId) const
{
    return GetCasusBelli(AttackerId, DefenderId).Num() > 0;
}

TArray<FString> UCoMDiplomacySubsystem::GetCasusBelli(int32 AttackerId, int32 DefenderId) const
{
    TArray<FString> Reasons;

    FIntPoint Key = MakeRelationKey(AttackerId, DefenderId);
    const FCoMDiplomaticRelation* Rel = Relations.Find(Key);
    if (!Rel) return Reasons;

    // Broken treaty
    if (Rel->TreatiesBroken > 0)
    {
        Reasons.Add(TEXT("Broken treaty"));
    }

    // Very negative reputation
    if (Rel->Reputation < -300)
    {
        Reasons.Add(TEXT("Deep enmity"));
    }

    // Aggression penalty (e.g. border violations)
    if (Rel->AggressionPenalty > 50)
    {
        Reasons.Add(TEXT("Territorial aggression"));
    }

    // Defender attacked our ally
    TArray<int32> MyAllies = GetAllies(AttackerId);
    for (int32 AllyId : MyAllies)
    {
        if (AreAtWar(DefenderId, AllyId))
        {
            Reasons.Add(FString::Printf(TEXT("Attacked our ally (Wizard %d)"), AllyId));
        }
    }

    // Defender is holding our vassal
    TArray<int32> MyVassals = GetVassals(AttackerId);
    // Check if defender conquered any of our vassals (simplified check)
    for (int32 VassalId : MyVassals)
    {
        if (AreAtWar(DefenderId, VassalId))
        {
            Reasons.Add(TEXT("Attacked our vassal"));
        }
    }

    return Reasons;
}

// ─────────────────────────────────────────────────────────────────────────────
// VASSAL SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

void UCoMDiplomacySubsystem::MakeVassal(int32 OverlordId, int32 VassalId)
{
    // Can't be vassal if already a vassal of someone else
    if (VassalMap.Contains(VassalId))
    {
        FreeVassal(VassalId);
    }

    VassalMap.Add(VassalId, OverlordId);

    // Set treaty to vassalage
    FCoMDiplomaticRelation& Rel = GetRelation(OverlordId, VassalId);
    Rel.CurrentTreaty = ECoMTreatyType::VassalTreaty;
    Rel.TreatyTurns = 0;

    // Large reputation boost between overlord and vassal
    ModifyReputation(OverlordId, VassalId, 200, TEXT("Vassalage established"));
}

void UCoMDiplomacySubsystem::FreeVassal(int32 VassalId)
{
    if (int32* OverlordPtr = VassalMap.Find(VassalId))
    {
        int32 OverlordId = *OverlordPtr;
        VassalMap.Remove(VassalId);

        FCoMDiplomaticRelation& Rel = GetRelation(OverlordId, VassalId);
        Rel.CurrentTreaty = ECoMTreatyType::None;

        // Reputation hit for the overlord (vassal rebellion)
        ModifyReputation(OverlordId, VassalId, -100, TEXT("Vassal freed/rebelled"));
    }
}

int32 UCoMDiplomacySubsystem::GetOverlord(int32 WizardId) const
{
    if (const int32* OverlordPtr = VassalMap.Find(WizardId))
    {
        return *OverlordPtr;
    }
    return -1;
}

TArray<int32> UCoMDiplomacySubsystem::GetVassals(int32 OverlordId) const
{
    TArray<int32> Vassals;
    for (const auto& Pair : VassalMap)
    {
        if (Pair.Value == OverlordId)
        {
            Vassals.Add(Pair.Key);
        }
    }
    return Vassals;
}
