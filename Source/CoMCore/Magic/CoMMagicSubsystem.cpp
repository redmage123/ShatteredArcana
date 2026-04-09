
#include "CoMMagicSubsystem.h"
#include "CoMCore/Data/CoMSpellDatabase.h"









// Copyright Mythforge Studios. All Rights Reserved.
// CoMMagicSubsystem.cpp — Full implementation
// Phase 7 — Shattered Arcana
// ─────────────────────────────────────────────────────────────────────────────
// LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────
void UCoMMagicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WizardMagicStates.Empty();
    ActiveCastings.Empty();
    ActiveRituals.Empty();
    ActiveRunes.Empty();
    NextRuneInstanceId = 1;
    RngStream.Initialize(0x4D616769);
}
void UCoMMagicSubsystem::Deinitialize()
{
    Super::Deinitialize();
}
// ─────────────────────────────────────────────────────────────────────────────
// WIZARD MAGIC STATE
// ─────────────────────────────────────────────────────────────────────────────
FCoMWizardMagicState& UCoMMagicSubsystem::GetWizardMagic(int32 WizardId)
{
    if (!WizardMagicStates.Contains(WizardId))
    {
        FCoMWizardMagicState NewState;
        NewState.WizardId = WizardId;
        WizardMagicStates.Add(WizardId, NewState);
    }
    return WizardMagicStates[WizardId];
}
int32 UCoMMagicSubsystem::GetCurrentMana(int32 WizardId) const
{
    if (const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId))
    {
        return State->CurrentMana;
    }
    return 0;
}
int32 UCoMMagicSubsystem::GetManaPerTurn(int32 WizardId) const
{
    return CalculateManaIncome(WizardId);
}
void UCoMMagicSubsystem::SetResearchAllocation(int32 WizardId, int32 ManaPerTurn)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    State.ResearchAllocation = FMath::Max(0, ManaPerTurn);
}
// ─────────────────────────────────────────────────────────────────────────────
// MANA NODES
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::ClaimManaNode(int32 WizardId, FIntPoint NodeTile)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    // Check not already claimed by this wizard
    if (State.ControlledNodes.Contains(NodeTile))
    {
        return false;
    }
    // Check not claimed by another wizard
    for (const auto& Pair : WizardMagicStates)
    {
        if (Pair.Key != WizardId && Pair.Value.ControlledNodes.Contains(NodeTile))
        {
            return false; // Already claimed
        }
    }
    State.ControlledNodes.Add(NodeTile);
    return true;
}
void UCoMMagicSubsystem::ReleaseManaNode(int32 WizardId, FIntPoint NodeTile)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    State.ControlledNodes.Remove(NodeTile);
}
int32 UCoMMagicSubsystem::GetNodeManaOutput(FIntPoint NodeTile, ECoMSpellRealm WizardRealm) const
{
    // Base mana output per node
    int32 Output = 5;
    // Realm-matched nodes produce double
    // (In full implementation, nodes would have their own realm type stored in world data)
    // For now, all nodes produce base output
    return Output;
}
// ─────────────────────────────────────────────────────────────────────────────
// SPELL CASTING
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::CanCastSpell(int32 WizardId, FName SpellId, const FCoMSpellCast& CastParams) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return false;
    // Must know the spell
    if (!State->KnownSpells.Contains(SpellId)) return false;
    // Must have enough mana
    if (State->CurrentMana < CastParams.ManaCost) return false;
    // Can't cast while already casting (overworld)
    if (CastParams.Scope != ECoMSpellScope::Combat && ActiveCastings.Contains(WizardId))
    {
        return false;
    }
    return true;
}
bool UCoMMagicSubsystem::CastSpell(const FCoMSpellCast& CastParams)
{
    if (!CanCastSpell(CastParams.CasterWizardId, CastParams.SpellId, CastParams))
    {
        return false;
    }
    FCoMWizardMagicState& State = GetWizardMagic(CastParams.CasterWizardId);
    // Deduct mana
    State.CurrentMana -= CastParams.ManaCost;
    FCoMSpellCast Cast = CastParams;
    Cast.TurnsRemaining = CastParams.CastingTime;
    Cast.EffectivePower = CalculateSpellPower(CastParams.CasterWizardId, CastParams.SpellId);
    if (Cast.CastingTime <= 1)
    {
        // Instant spell — resolve immediately
        ResolveSpell(Cast);
    }
    else
    {
        // Multi-turn spell — queue it
        ActiveCastings.Add(CastParams.CasterWizardId, Cast);
    }
    return true;
}
void UCoMMagicSubsystem::CancelCasting(int32 WizardId)
{
    if (FCoMSpellCast* Cast = ActiveCastings.Find(WizardId))
    {
        // Refund 50% of mana
        FCoMWizardMagicState& State = GetWizardMagic(WizardId);
        State.CurrentMana += Cast->ManaCost / 2;
        ActiveCastings.Remove(WizardId);
    }
}
FCoMSpellCast UCoMMagicSubsystem::GetCurrentCasting(int32 WizardId) const
{
    if (const FCoMSpellCast* Cast = ActiveCastings.Find(WizardId))
    {
        return *Cast;
    }
    return FCoMSpellCast();
}
int32 UCoMMagicSubsystem::CalculateSpellPower(int32 WizardId, FName SpellId) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return 0;

    // Base power scales with spell tier
    FCoMSpellInfo SpellInfo = CoMSpellDatabase::GetSpellInfo(SpellId);
    int32 BasePower = SpellInfo.CastingCost; // Higher tier = more base power

    // Casting skill multiplier (1.0x at skill 10, 3.0x at skill 100)
    float SkillMult = 1.0f + (State->CastingSkill - 10) * 0.022f;

    // Spell books bonus: more books in the spell's realm = more power
    int32 BookBonus = 0;
    if (const int32* BookCount = State->SpellBooks.Find(SpellInfo.Realm))
    {
        BookBonus = (*BookCount) * 3; // 3 power per book in matching realm
    }
    // Smaller bonus from off-realm books
    for (const auto& Pair : State->SpellBooks)
    {
        if (Pair.Key != SpellInfo.Realm)
        {
            BookBonus += Pair.Value; // 1 power per off-realm book
        }
    }

    return FMath::RoundToInt(BasePower * SkillMult) + BookBonus;
}
// ─────────────────────────────────────────────────────────────────────────────
// SPELL RESEARCH
// ─────────────────────────────────────────────────────────────────────────────
TArray<FName> UCoMMagicSubsystem::GetResearchableSpells(int32 WizardId) const
{
    TArray<FName> Available;
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return Available;

    // Check all spells in the database
    TArray<FName> AllSpells = CoMSpellDatabase::GetAllSpellIDs();
    for (const FName& SpellId : AllSpells)
    {
        // Skip already-known spells
        if (State->KnownSpells.Contains(SpellId))
        {
            continue;
        }

        // Skip the spell currently being researched
        if (State->CurrentResearchSpell == SpellId)
        {
            continue;
        }

        FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(SpellId);

        // Check if the wizard has spell books in the right realm
        // Arcane spells are always available
        if (Info.Realm != ECoMSpellRealm::Arcane)
        {
            const int32* BookCount = State->SpellBooks.Find(Info.Realm);
            if (!BookCount || *BookCount <= 0)
            {
                // Also check primary/secondary realm
                if (State->PrimaryRealm != Info.Realm && State->SecondaryRealm != Info.Realm)
                {
                    continue;
                }
            }

            // Tier requirements: Common=1 book, Uncommon=2, Rare=4, VeryRare=6
            int32 BooksInRealm = 0;
            if (BookCount) BooksInRealm = *BookCount;
            if (State->PrimaryRealm == Info.Realm) BooksInRealm = FMath::Max(BooksInRealm, 1);

            int32 RequiredBooks = 1;
            switch (Info.Rarity)
            {
            case ECoMSpellRarity::Common:    RequiredBooks = 1; break;
            case ECoMSpellRarity::Uncommon:  RequiredBooks = 2; break;
            case ECoMSpellRarity::Rare:      RequiredBooks = 4; break;
            case ECoMSpellRarity::VeryRare:  RequiredBooks = 6; break;
            default: break;
            }

            if (BooksInRealm < RequiredBooks)
            {
                continue;
            }
        }

        Available.Add(SpellId);
    }

    return Available;
}
bool UCoMMagicSubsystem::StartResearch(int32 WizardId, FName SpellId)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    if (State.KnownSpells.Contains(SpellId)) return false;
    State.CurrentResearchSpell = SpellId;
    State.ResearchProgress = 0;
    return true;
}
int32 UCoMMagicSubsystem::GetResearchProgress(int32 WizardId) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State || State->CurrentResearchSpell.IsNone()) return 0;
    // Look up research cost from spell database
    FCoMSpellInfo SpellInfo = CoMSpellDatabase::GetSpellInfo(State->CurrentResearchSpell);
    int32 TotalCost = SpellInfo.ResearchCost;
    if (TotalCost <= 0) return 100;
    return FMath::Clamp(State->ResearchProgress * 100 / TotalCost, 0, 100);
}
int32 UCoMMagicSubsystem::GetResearchETATurns(int32 WizardId) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State || State->CurrentResearchSpell.IsNone()) return -1;
    if (State->ResearchAllocation <= 0) return -1;
    // Look up research cost from spell database
    FCoMSpellInfo SpellInfo = CoMSpellDatabase::GetSpellInfo(State->CurrentResearchSpell);
    int32 TotalCost = SpellInfo.ResearchCost;
    int32 Remaining = TotalCost - State->ResearchProgress;
    return FMath::CeilToInt(static_cast<float>(Remaining) / State->ResearchAllocation);
}
// ─────────────────────────────────────────────────────────────────────────────
// RITUALS
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::BeginRitual(int32 WizardId, FName RitualId, int32 ManaChannelPerTurn)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    FCoMActiveRitual Ritual;
    Ritual.RitualSpellID = RitualId;
    Ritual.WizardIndex = WizardId;
    Ritual.ManaCostTotal = 500;
    Ritual.ManaInvested = 0;
    Ritual.ManaChannelPerTurn = ManaChannelPerTurn;
    Ritual.TurnsRemaining = (ManaChannelPerTurn > 0) ? ((500 + ManaChannelPerTurn - 1) / ManaChannelPerTurn) : 1;
    ActiveRituals.Add(Ritual);
    return true;
}
void UCoMMagicSubsystem::CancelRitual(int32 WizardId, FName RitualId)
{
    ActiveRituals.RemoveAll([WizardId, RitualId](const FCoMActiveRitual& R)
    {
        return R.WizardIndex == WizardId && R.RitualSpellID == RitualId;
    });
    // Components already consumed are NOT refunded
}
TArray<FCoMActiveRitual> UCoMMagicSubsystem::GetActiveRituals(int32 WizardId) const
{
    TArray<FCoMActiveRitual> Result;
    for (const FCoMActiveRitual& R : ActiveRituals)
    {
        if (R.WizardIndex == WizardId)
        {
            Result.Add(R);
        }
    }
    return Result;
}
bool UCoMMagicSubsystem::InterruptRitual(int32 WizardId, FName RitualId)
{
    for (int32 i = ActiveRituals.Num() - 1; i >= 0; --i)
    {
        if (ActiveRituals[i].WizardIndex == WizardId &&
            ActiveRituals[i].RitualSpellID == RitualId &&
            ActiveRituals[i].Stability > FFixed64(0))
        {
            // Interrupted — all invested mana is lost
            ActiveRituals.RemoveAt(i);
            return true;
        }
    }
    return false;
}
// ─────────────────────────────────────────────────────────────────────────────
// RUNE INSCRIPTION
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::InscribeRune(int32 WizardId, FName RuneId, ECoMRuneTarget TargetType, int32 TargetId)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    FCoMActiveRune Rune;
    Rune.RuneId = RuneId;
    Rune.InscribedByWizardId = WizardId;
    Rune.TargetType = TargetType;
    Rune.TargetId = TargetId;
    Rune.Power = State.CastingSkill;
    Rune.ChargesLeft = -1; // Permanent by default
    Rune.InstanceId = NextRuneInstanceId++;
    // Check mana before inscribing
    if (State.CurrentMana < 20) return false;
    ActiveRunes.Add(Rune);
    // Mana cost for inscription
    State.CurrentMana -= 20; // Would come from rune data asset
    return true;
}
void UCoMMagicSubsystem::RemoveRune(int32 RuneInstanceId)
{
    ActiveRunes.RemoveAll([RuneInstanceId](const FCoMActiveRune& R)
    {
        return R.InstanceId == RuneInstanceId;
    });
}
TArray<FCoMActiveRune> UCoMMagicSubsystem::GetRunesOnTarget(ECoMRuneTarget TargetType, int32 TargetId) const
{
    TArray<FCoMActiveRune> Result;
    for (const FCoMActiveRune& R : ActiveRunes)
    {
        if (R.TargetType == TargetType && R.TargetId == TargetId)
        {
            Result.Add(R);
        }
    }
    return Result;
}
// ─────────────────────────────────────────────────────────────────────────────
// ENCHANTMENTS
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::MaintainEnchantment(int32 WizardId, FName SpellId)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    if (!State.KnownSpells.Contains(SpellId)) return false;
    if (State.ActiveEnchantments.Contains(SpellId)) return false;
    State.ActiveEnchantments.Add(SpellId);
    // Maintenance cost added during turn processing
    return true;
}
void UCoMMagicSubsystem::DismissEnchantment(int32 WizardId, FName SpellId)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    State.ActiveEnchantments.Remove(SpellId);
}
bool UCoMMagicSubsystem::DispelEnchantment(int32 CasterWizardId, int32 TargetWizardId, FName SpellId)
{
    FCoMWizardMagicState* TargetState = WizardMagicStates.Find(TargetWizardId);
    if (!TargetState || !TargetState->ActiveEnchantments.Contains(SpellId))
    {
        return false;
    }
    // Contested roll: caster skill vs target skill
    int32 CasterPower = CalculateSpellPower(CasterWizardId, SpellId);
    int32 DefenderPower = CalculateSpellPower(TargetWizardId, SpellId);
    int32 CasterRoll = CasterPower + RngStream.RandRange(1, 50);
    int32 DefenderRoll = DefenderPower + RngStream.RandRange(1, 50);
    if (CasterRoll > DefenderRoll)
    {
        TargetState->ActiveEnchantments.Remove(SpellId);
        return true;
    }
    return false;
}
// ─────────────────────────────────────────────────────────────────────────────
// COUNTER-MAGIC
// ─────────────────────────────────────────────────────────────────────────────
bool UCoMMagicSubsystem::CounterSpell(int32 CounterCasterWizardId, int32 TargetWizardId, int32 ManaSpent)
{
    FCoMSpellCast* TargetCast = ActiveCastings.Find(TargetWizardId);
    if (!TargetCast) return false;
    FCoMWizardMagicState& CounterState = GetWizardMagic(CounterCasterWizardId);
    if (CounterState.CurrentMana < ManaSpent) return false;
    CounterState.CurrentMana -= ManaSpent;
    // Counter success based on mana spent vs spell power
    int32 CounterPower = ManaSpent + CounterState.CastingSkill;
    int32 SpellPower = TargetCast->EffectivePower + TargetCast->ManaCost;
    if (CounterPower > SpellPower)
    {
        // Successfully countered — spell fizzles, no mana refund for target
        ActiveCastings.Remove(TargetWizardId);
        return true;
    }
    return false;
}
// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING
// ─────────────────────────────────────────────────────────────────────────────
void UCoMMagicSubsystem::ProcessTurn(int32 CurrentTurn)
{
    for (auto& Pair : WizardMagicStates)
    {
        FCoMWizardMagicState& State = Pair.Value;
        // 1. Calculate mana income
        int32 Income = CalculateManaIncome(Pair.Key);
        // 2. Subtract maintenance costs — look up actual upkeep from spell database
        int32 Maintenance = 0;
        for (const FName& EnchantID : State.ActiveEnchantments)
        {
            FCoMSpellInfo EnchInfo = CoMSpellDatabase::GetSpellInfo(EnchantID);
            Maintenance += (EnchInfo.UpkeepMana > 0) ? EnchInfo.UpkeepMana : 5;
        }
        State.MaintenanceCost = Maintenance;
        // 3. Subtract research allocation
        int32 ResearchSpend = FMath::Clamp(FMath::Min(State.ResearchAllocation, Income - Maintenance), 0, Income);
        // 4. Net mana gain
        int32 NetMana = Income - Maintenance - ResearchSpend;
        int32 ProjectedMana = State.CurrentMana + NetMana;
        State.ManaPerTurn = Income;
        // 5. Force-dismiss enchantments if mana would go negative
        while (ProjectedMana < 0 && State.ActiveEnchantments.Num() > 0)
        {
            // Dismiss the last enchantment and recoup its maintenance
            FName DismissedSpell = State.ActiveEnchantments.Last();
            FCoMSpellInfo DismissedInfo = CoMSpellDatabase::GetSpellInfo(DismissedSpell);
            int32 DismissedUpkeep = (DismissedInfo.UpkeepMana > 0) ? DismissedInfo.UpkeepMana : 5;
            State.ActiveEnchantments.RemoveAt(State.ActiveEnchantments.Num() - 1);
            Maintenance -= DismissedUpkeep;
            ProjectedMana += DismissedUpkeep;
        }
        State.CurrentMana = FMath::Clamp(ProjectedMana, 0, State.MaxMana);
        State.MaintenanceCost = Maintenance;
        // 6. Advance research
        if (ResearchSpend > 0 && !State.CurrentResearchSpell.IsNone())
        {
            State.ResearchProgress += ResearchSpend;
            FCoMSpellInfo ResearchSpellInfo = CoMSpellDatabase::GetSpellInfo(State.CurrentResearchSpell);
            int32 TotalCost = ResearchSpellInfo.ResearchCost;
            if (State.ResearchProgress >= TotalCost)
            {
                // Research complete!
                State.KnownSpells.Add(State.CurrentResearchSpell);
                State.CurrentResearchSpell = NAME_None;
                State.ResearchProgress = 0;
            }
        }
    }
    // 7. Advance multi-turn castings
    TArray<int32> CompletedCastings;
    for (auto& Pair : ActiveCastings)
    {
        Pair.Value.TurnsRemaining--;
        if (Pair.Value.TurnsRemaining <= 0)
        {
            CompletedCastings.Add(Pair.Key);
        }
    }
    for (int32 WizardId : CompletedCastings)
    {
        FCoMSpellCast& Cast = ActiveCastings[WizardId];
        ResolveSpell(Cast);
        ActiveCastings.Remove(WizardId);
    }
    // 8. Advance rituals
    for (int32 i = ActiveRituals.Num() - 1; i >= 0; --i)
    {
        FCoMActiveRitual& Ritual = ActiveRituals[i];
        FCoMWizardMagicState& State = GetWizardMagic(Ritual.WizardIndex);
        int32 ChannelAmount = FMath::Min(Ritual.ManaChannelPerTurn, State.CurrentMana);
        State.CurrentMana -= ChannelAmount;
        Ritual.ManaInvested += ChannelAmount;
        if (ChannelAmount > 0)
        {
            Ritual.TurnsRemaining--;
        }
        if (Ritual.ManaInvested >= Ritual.ManaCostTotal)
        {
            ResolveRitual(Ritual);
            ActiveRituals.RemoveAt(i);
        }
    }
    // 9. Tick rune charges
    TickRunes();
}
int32 UCoMMagicSubsystem::CalculateManaIncome(int32 WizardId) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return 0;
    int32 Income = 5; // Base mana per turn
    // Mana nodes
    for (const FIntPoint& Node : State->ControlledNodes)
    {
        Income += GetNodeManaOutput(Node, State->PrimaryRealm);
    }
    // Casting skill bonus
    Income += State->CastingSkill / 10;
    // Spell book bonus
    for (const auto& Pair : State->SpellBooks)
    {
        Income += Pair.Value; // 1 mana per spell book
    }
    return Income;
}
void UCoMMagicSubsystem::ResolveSpell(FCoMSpellCast& Cast)
{
    // In full implementation, this would:
    // - Look up spell data asset for effects
    // - Apply damage/healing/buff/debuff to target
    // - Handle area-of-effect
    // - Trigger visual effects
    // - Log to combat/event history
    // For now, just mark as resolved
    // Actual spell effects will be integrated with combat and world systems
}
void UCoMMagicSubsystem::ResolveRitual(FCoMActiveRitual& Ritual)
{
    // Rituals are powerful global effects:
    // - Summon legendary creature
    // - Plane-wide enchantment
    // - Open permanent portal
    // - Awaken ancient power
    // Full implementation deferred to ritual data asset integration
}
void UCoMMagicSubsystem::TickRunes()
{
    for (int32 i = ActiveRunes.Num() - 1; i >= 0; --i)
    {
        if (ActiveRunes[i].ChargesLeft > 0)
        {
            ActiveRunes[i].ChargesLeft--;
            if (ActiveRunes[i].ChargesLeft <= 0)
            {
                ActiveRunes.RemoveAt(i);
            }
        }
        // ChargesLeft == -1 means permanent, don't decrement
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMagicSubsystem::ExportAll(TArray<FCoMWizardMagicState>& OutMagicStates,
                                    TArray<FCoMSpellCast>& OutCastings,
                                    TArray<int32>& OutCastingWizardIDs,
                                    TArray<FCoMActiveRitual>& OutRituals,
                                    TArray<FCoMActiveRune>& OutRunes) const
{
    OutMagicStates.Empty();
    OutMagicStates.Reserve(WizardMagicStates.Num());
    for (const auto& Pair : WizardMagicStates)
    {
        OutMagicStates.Add(Pair.Value);
    }

    OutCastings.Empty();
    OutCastingWizardIDs.Empty();
    OutCastings.Reserve(ActiveCastings.Num());
    OutCastingWizardIDs.Reserve(ActiveCastings.Num());
    for (const auto& Pair : ActiveCastings)
    {
        OutCastingWizardIDs.Add(Pair.Key);
        OutCastings.Add(Pair.Value);
    }

    OutRituals = ActiveRituals;
    OutRunes   = ActiveRunes;
}

void UCoMMagicSubsystem::ImportAll(const TArray<FCoMWizardMagicState>& InMagicStates,
                                    const TArray<FCoMSpellCast>& InCastings,
                                    const TArray<int32>& InCastingWizardIDs,
                                    const TArray<FCoMActiveRitual>& InRituals,
                                    const TArray<FCoMActiveRune>& InRunes)
{
    WizardMagicStates.Empty();
    for (const FCoMWizardMagicState& State : InMagicStates)
    {
        WizardMagicStates.Add(State.WizardId, State);
    }

    ActiveCastings.Empty();
    for (int32 i = 0; i < InCastings.Num() && i < InCastingWizardIDs.Num(); ++i)
    {
        ActiveCastings.Add(InCastingWizardIDs[i], InCastings[i]);
    }

    ActiveRituals = InRituals;
    ActiveRunes   = InRunes;

    // Restore next rune instance ID
    NextRuneInstanceId = 1;
    for (const FCoMActiveRune& Rune : ActiveRunes)
    {
        if (Rune.InstanceId >= NextRuneInstanceId)
        {
            NextRuneInstanceId = Rune.InstanceId + 1;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MagicSubsystem] ImportAll: %d wizard states, %d castings, %d rituals, %d runes imported."),
           WizardMagicStates.Num(), ActiveCastings.Num(), ActiveRituals.Num(), ActiveRunes.Num());
}
