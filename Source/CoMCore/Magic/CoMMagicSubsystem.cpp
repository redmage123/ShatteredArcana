
#include "CoMMagicSubsystem.h"
#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMCore/Data/CoMGlobalEnchantmentData.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMFogOfWarSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Victory/CoMVictorySubsystem.h"
#include "CoMCore/Magic/CoMSpellVFXSubsystem.h"
#include "CoMCore/Audio/CoMAudioSubsystem.h"
#include "CoMCore/CoreTypes/CoMConstants.h"









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
void UCoMMagicSubsystem::ReseedRandom(int32 MasterSeed)
{
    RngStream.Initialize(MasterSeed ^ 0x4D616769);
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
    // Look up the node's realm from world data
    UGameInstance* GI = GetGameInstance();
    if (!GI) return 5;

    UCoMWorldMapSubsystem* MapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
    if (!MapSub) return 5;

    const FCoMTileData* Tile = MapSub->GetTile(ECoMPlane::Aurelith, ECoMMapLayer::Surface,
                                                 NodeTile.X, NodeTile.Y);
    if (!Tile || !Tile->bHasManaNode) return 0;

    int32 Output = Tile->NodeManaOutput;

    // Realm-matched books give bonus: +1 per book in matching realm
    const FCoMWizardMagicState* State = WizardMagicStates.Find(0); // TODO: pass wizard ID
    if (State && Tile->NodeRealm != ECoMSpellRealm::None)
    {
        if (const int32* Books = State->SpellBooks.Find(Tile->NodeRealm))
        {
            Output += *Books; // Each book in the node's realm adds +1 mana
        }
    }

    return Output;
}

// ── Spirit Melding ───────────────────────────────────────────────────────────

bool UCoMMagicSubsystem::MeldSpiritWithNode(int32 WizardId, FIntPoint NodeTile, ECoMSpellRealm SpiritRealm)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return false;

    UCoMWorldMapSubsystem* MapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
    if (!MapSub) return false;

    FCoMTileData* Tile = MapSub->GetTileMutable(ECoMPlane::Aurelith, ECoMMapLayer::Surface,
                                                  NodeTile.X, NodeTile.Y);
    if (!Tile || !Tile->bHasManaNode) return false;

    // Node must be unguarded
    if (Tile->bNodeGuarded)
    {
        UE_LOG(LogTemp, Warning, TEXT("MeldSpirit: Node at (%d,%d) is still guarded."),
               NodeTile.X, NodeTile.Y);
        return false;
    }

    // Node must be unclaimed
    if (Tile->NodeOwnerWizardIndex >= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MeldSpirit: Node at (%d,%d) already claimed by wizard %d."),
               NodeTile.X, NodeTile.Y, Tile->NodeOwnerWizardIndex);
        return false;
    }

    // Spirit realm must match node realm, or be Arcane (wild card)
    if (SpiritRealm != ECoMSpellRealm::Arcane && SpiritRealm != Tile->NodeRealm)
    {
        UE_LOG(LogTemp, Warning, TEXT("MeldSpirit: Spirit realm mismatch — spirit is %d, node is %d."),
               static_cast<int32>(SpiritRealm), static_cast<int32>(Tile->NodeRealm));
        return false;
    }

    // Claim the node
    Tile->NodeOwnerWizardIndex = WizardId;
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    State.ControlledNodes.AddUnique(NodeTile);

    // Add realm mana income
    int32 ManaOutput = Tile->NodeManaOutput;
    State.RealmManaPerTurn.FindOrAdd(Tile->NodeRealm) += ManaOutput;

    // Set max storage for this realm (base 50 + 10 per book in realm)
    int32 Books = 0;
    if (const int32* B = State.SpellBooks.Find(Tile->NodeRealm)) { Books = *B; }
    State.RealmManaMax.FindOrAdd(Tile->NodeRealm) = FMath::Max(
        State.RealmManaMax.FindOrAdd(Tile->NodeRealm), 50 + Books * 10);

    UE_LOG(LogTemp, Log, TEXT("MeldSpirit: Wizard %d melded with %s node at (%d,%d). +%d %s mana/turn."),
           WizardId,
           *UEnum::GetValueAsString(Tile->NodeRealm),
           NodeTile.X, NodeTile.Y, ManaOutput,
           *UEnum::GetValueAsString(Tile->NodeRealm));

    return true;
}

void UCoMMagicSubsystem::DispelNodeSpirit(FIntPoint NodeTile)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UCoMWorldMapSubsystem* MapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
    if (!MapSub) return;

    FCoMTileData* Tile = MapSub->GetTileMutable(ECoMPlane::Aurelith, ECoMMapLayer::Surface,
                                                  NodeTile.X, NodeTile.Y);
    if (!Tile || !Tile->bHasManaNode || Tile->NodeOwnerWizardIndex < 0) return;

    int32 OldOwner = Tile->NodeOwnerWizardIndex;
    FCoMWizardMagicState& State = GetWizardMagic(OldOwner);

    // Remove from controlled nodes
    State.ControlledNodes.Remove(NodeTile);

    // Reduce realm mana income
    if (int32* Income = State.RealmManaPerTurn.Find(Tile->NodeRealm))
    {
        *Income = FMath::Max(0, *Income - Tile->NodeManaOutput);
    }

    Tile->NodeOwnerWizardIndex = -1;

    UE_LOG(LogTemp, Log, TEXT("DispelNodeSpirit: Node at (%d,%d) released from wizard %d."),
           NodeTile.X, NodeTile.Y, OldOwner);
}

TMap<ECoMSpellRealm, int32> UCoMMagicSubsystem::GetRealmManaIncome(int32 WizardId) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return {};
    return State->RealmManaPerTurn;
}

int32 UCoMMagicSubsystem::GetRealmMana(int32 WizardId, ECoMSpellRealm Realm) const
{
    const FCoMWizardMagicState* State = WizardMagicStates.Find(WizardId);
    if (!State) return 0;
    const int32* Mana = State->RealmMana.Find(Realm);
    return Mana ? *Mana : 0;
}

bool UCoMMagicSubsystem::SpendManaForSpell(int32 WizardId, ECoMSpellRealm SpellRealm, int32 Cost)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);

    int32 Remaining = Cost;

    // Draw from realm-specific pool first
    if (SpellRealm != ECoMSpellRealm::None && SpellRealm != ECoMSpellRealm::Arcane)
    {
        int32& RealmPool = State.RealmMana.FindOrAdd(SpellRealm);
        int32 FromRealm = FMath::Min(RealmPool, Remaining);
        RealmPool -= FromRealm;
        Remaining -= FromRealm;
    }

    // Draw remainder from generic pool
    if (Remaining > 0)
    {
        if (State.CurrentMana < Remaining)
        {
            // Not enough mana total — refund realm mana and fail
            if (SpellRealm != ECoMSpellRealm::None && SpellRealm != ECoMSpellRealm::Arcane)
            {
                State.RealmMana.FindOrAdd(SpellRealm) += (Cost - Remaining);
            }
            return false;
        }
        State.CurrentMana -= Remaining;
    }

    return true;
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
    // Must have enough mana — hostile global enchantments (Suppress Magic) can
    // inflate the effective cost.
    const int32 EffectiveCost = FMath::CeilToInt(CastParams.ManaCost * GetIncomingCastCostMultiplier(WizardId));
    if (State->CurrentMana < EffectiveCost) return false;
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
    // Deduct mana, inflated by any hostile Suppress Magic enchantment.
    const int32 EffectiveCost = FMath::CeilToInt(CastParams.ManaCost * GetIncomingCastCostMultiplier(CastParams.CasterWizardId));
    State.CurrentMana -= EffectiveCost;
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

            // Out of mana to maintain it — announce the lapse to all players.
            const FCoMGlobalEnchantmentDef& LapsedDef = CoMGlobalEnchantmentData::Get(DismissedSpell);
            const FString LapsedName = LapsedDef.IsValid()
                ? LapsedDef.DisplayName.ToString() : DismissedSpell.ToString();
            OnGlobalEnchantmentChanged.Broadcast(Pair.Key, DismissedSpell, LapsedName, /*bActive*/ false);
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
                FName CompletedSpell = State.CurrentResearchSpell;
                State.KnownSpells.Add(CompletedSpell);
                State.CurrentResearchSpell = NAME_None;
                State.ResearchProgress = 0;

                // Auto-research: pick the next spell if enabled.
                if (IsAutoResearchEnabled(Pair.Key))
                {
                    if (AutoPickResearch(Pair.Key))
                    {
                        UE_LOG(LogTemp, Log,
                            TEXT("[MagicSubsystem] Auto-researching %s for wizard %d"),
                            *State.CurrentResearchSpell.ToString(), Pair.Key);
                    }
                }
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

    // 10. Apply active global enchantment effects (unrest, decay, vision, ...)
    ProcessGlobalEnchantments(CurrentTurn);
}

// ─────────────────────────────────────────────────────────────────────────────
// GLOBAL ENCHANTMENTS — per-turn effects + queries
// ─────────────────────────────────────────────────────────────────────────────
void UCoMMagicSubsystem::ProcessGlobalEnchantments(int32 CurrentTurn)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMCitySubsystem*     Cities = GI->GetSubsystem<UCoMCitySubsystem>();
    UCoMUnitSubsystem*     Units  = GI->GetSubsystem<UCoMUnitSubsystem>();
    UCoMFogOfWarSubsystem* FoW    = GI->GetSubsystem<UCoMFogOfWarSubsystem>();

    // A single Tranquility anywhere blunts world-catastrophe enchantments.
    bool bPeace = false;
    for (const auto& Pair : WizardMagicStates)
    {
        for (const FName& E : Pair.Value.ActiveEnchantments)
        {
            if (CoMGlobalEnchantmentData::Get(E).Effect == ECoMEnchantEffect::GlobalPeaceAura)
            {
                bPeace = true; break;
            }
        }
        if (bPeace) { break; }
    }

    for (auto& Pair : WizardMagicStates)
    {
        const int32 WizardId = Pair.Key;
        FCoMWizardMagicState& State = Pair.Value;

        for (const FName& EnchID : State.ActiveEnchantments)
        {
            const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(EnchID);
            if (!Def.IsValid()) { continue; }

            switch (Def.Effect)
            {
            case ECoMEnchantEffect::ReduceOwnUnrest:
                if (Cities)
                {
                    for (const FCoMCityData* C : Cities->GetCitiesForWizard(WizardId))
                    {
                        if (!C) { continue; }
                        if (FCoMCityData* M = Cities->GetCityMutable(C->CityID))
                        {
                            M->Unrest = FMath::Max(0, M->Unrest - Def.Magnitude);
                        }
                    }
                }
                break;

            case ECoMEnchantEffect::HealOwnUnits:
                if (Units)
                {
                    for (const FCoMArmyGroup* A : Units->GetArmiesForWizard(WizardId))
                    {
                        if (!A) { continue; }
                        for (int32 UID : A->UnitIDs) { Units->ApplyHeal(UID, Def.Magnitude); }
                    }
                }
                break;

            case ECoMEnchantEffect::BoostOwnMana:
                State.CurrentMana = FMath::Min(State.MaxMana, State.CurrentMana + Def.Magnitude);
                break;

            case ECoMEnchantEffect::GlobalChaosBoon:
                // Raw chaos empowers the caster with bonus mana each turn.
                State.CurrentMana = FMath::Min(State.MaxMana, State.CurrentMana + Def.Magnitude * 5);
                break;

            case ECoMEnchantEffect::DoomsdayCountdown:
                // The doomsday clock floods chaos mana to its master.
                State.CurrentMana = FMath::Min(State.MaxMana, State.CurrentMana + Def.Magnitude);
                break;

            case ECoMEnchantEffect::GlobalMapVision:
                if (FoW)
                {
                    FoW->RevealArea(WizardId, ECoMPlane::Aurelith,
                        FIntPoint(CoM::MAP_WIDTH / 2, CoM::MAP_HEIGHT / 2), CoM::MAP_WIDTH);
                }
                break;

            case ECoMEnchantEffect::GlobalCityDecay:
            case ECoMEnchantEffect::GlobalDeathToll:
                if (Cities && !bPeace)
                {
                    for (int32 CID : Cities->GetAllCityIDs())
                    {
                        FCoMCityData* M = Cities->GetCityMutable(CID);
                        if (M && M->OwnerWizardIndex >= 0 && M->OwnerWizardIndex != WizardId)
                        {
                            M->Population = FMath::Max(1, M->Population - Def.Magnitude);
                        }
                    }
                }
                break;

            default:
                // Combat / query / passive effects are applied where they are read
                // (cast cost, combat resolution); instant effects fire at cast time.
                break;
            }
        }
    }
}

bool UCoMMagicSubsystem::IsGlobalEnchantmentActive(int32 WizardId, FName SpellID) const
{
    const FCoMWizardMagicState* S = WizardMagicStates.Find(WizardId);
    return S && S->ActiveEnchantments.Contains(SpellID);
}

bool UCoMMagicSubsystem::IsEnchantmentActiveByOther(int32 WizardId, FName SpellID) const
{
    for (const auto& Pair : WizardMagicStates)
    {
        if (Pair.Key != WizardId && Pair.Value.ActiveEnchantments.Contains(SpellID))
        {
            return true;
        }
    }
    return false;
}

float UCoMMagicSubsystem::GetIncomingCastCostMultiplier(int32 WizardId) const
{
    float Mult = 1.0f;
    for (const auto& Pair : WizardMagicStates)
    {
        if (Pair.Key == WizardId) { continue; }
        for (const FName& E : Pair.Value.ActiveEnchantments)
        {
            const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(E);
            if (Def.Effect == ECoMEnchantEffect::RaiseEnemyCastCost)
            {
                Mult += static_cast<float>(Def.Magnitude) / 100.0f;
            }
        }
    }
    return Mult;
}

void UCoMMagicSubsystem::CancelGlobalEnchantment(int32 WizardId, FName SpellID)
{
    FCoMWizardMagicState* S = WizardMagicStates.Find(WizardId);
    if (!S) { return; }
    if (S->ActiveEnchantments.Remove(SpellID) > 0)
    {
        const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(SpellID);
        const FString Name = Def.IsValid() ? Def.DisplayName.ToString() : SpellID.ToString();
        OnGlobalEnchantmentChanged.Broadcast(WizardId, SpellID, Name, /*bActive*/ false);
    }
}

TArray<FName> UCoMMagicSubsystem::GetActiveEnchantments(int32 WizardId) const
{
    if (const FCoMWizardMagicState* S = WizardMagicStates.Find(WizardId))
    {
        return S->ActiveEnchantments;
    }
    return TArray<FName>();
}

void UCoMMagicSubsystem::GetAllActiveEnchantments(TArray<int32>& OutOwners, TArray<FName>& OutSpellIDs) const
{
    OutOwners.Reset();
    OutSpellIDs.Reset();
    for (const auto& Pair : WizardMagicStates)
    {
        for (const FName& E : Pair.Value.ActiveEnchantments)
        {
            if (CoMGlobalEnchantmentData::IsGlobalEnchantment(E))
            {
                OutOwners.Add(Pair.Key);
                OutSpellIDs.Add(E);
            }
        }
    }
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
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    const FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(Cast.SpellId);

    // Career-stats hook: every successful resolve fires this so the
    // stats subsystem can count spells + mana spent toward Archmagi.
    OnSpellResolved.Broadcast(Cast.CasterWizardId, Cast.SpellId, Info.CastingCost);

    const int32 Power        = (Cast.EffectivePower > 0) ? Cast.EffectivePower
                                                          : CalculateSpellPower(Cast.CasterWizardId, Cast.SpellId);
    UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
    UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>();

    auto LogResolve = [&](const TCHAR* Label)
    {
        UE_LOG(LogTemp, Log, TEXT("[Magic] Resolved %s (%s) by wizard %d, power=%d"),
            *Cast.SpellId.ToString(), Label, Cast.CasterWizardId, Power);
    };

    // ── VFX + SFX trigger ────────────────────────────────────────────────
    // Fired up front so the audiovisual cue plays even if the effect logic
    // ends up no-op (missing target, etc.). The VFX widget on the HUD picks
    // this up via OnEffectRequested; PlaySpellSFX is a 3D world sound.
    {
        const float TileWorld = CoM::TILE_WORLD_SIZE_CM;
        FVector World(0.0f);
        if (Cast.TargetTile.X >= 0)
        {
            World = FVector(
                Cast.TargetTile.X * TileWorld + TileWorld * 0.5f,
                Cast.TargetTile.Y * TileWorld + TileWorld * 0.5f,
                40.0f);
        }
        else if (Cast.TargetUnitId >= 0 && Units)
        {
            // Try to locate the target unit's army for positioning.
            for (int32 W = 0; W < CoM::MAX_WIZARDS; ++W)
            {
                bool bFound = false;
                for (const FCoMArmyGroup* A : Units->GetArmiesForWizard(W))
                {
                    if (A && A->UnitIDs.Contains(Cast.TargetUnitId))
                    {
                        World = FVector(
                            A->Position.X * TileWorld + TileWorld * 0.5f,
                            A->Position.Y * TileWorld + TileWorld * 0.5f,
                            40.0f);
                        bFound = true;
                        break;
                    }
                }
                if (bFound) break;
            }
        }

        // VFX: realm-keyed effect ID, pattern matches RegisterAllDefaultEffects
        // (e.g., "chaos.fireball"). Pick effect-name by spell effect type.
        if (UCoMSpellVFXSubsystem* VFX = GI->GetSubsystem<UCoMSpellVFXSubsystem>())
        {
            const TCHAR* RealmTag = TEXT("arcane");
            switch (Info.Realm)
            {
            case ECoMSpellRealm::Life:    RealmTag = TEXT("life");    break;
            case ECoMSpellRealm::Death:   RealmTag = TEXT("death");   break;
            case ECoMSpellRealm::Chaos:   RealmTag = TEXT("chaos");   break;
            case ECoMSpellRealm::Nature:  RealmTag = TEXT("nature");  break;
            case ECoMSpellRealm::Sorcery: RealmTag = TEXT("sorcery"); break;
            case ECoMSpellRealm::Arcane:  RealmTag = TEXT("arcane");  break;
            case ECoMSpellRealm::Binding: RealmTag = TEXT("binding"); break;
            case ECoMSpellRealm::Spirit:  RealmTag = TEXT("spirit");  break;
            case ECoMSpellRealm::Glamour: RealmTag = TEXT("glamour"); break;
            default: break;
            }

            // Per-realm effect names from RegisterAllDefaultEffects, slot 0..3.
            // Pick by spell effect: Damage = slot 0, Buff = 1, Summon = 2, Debuff = 3.
            const TCHAR* PerRealmName[9][4] = {
                /*Life*/    { TEXT("smite"),     TEXT("shield"),    TEXT("resurrect"),    TEXT("heal") },
                /*Death*/   { TEXT("shadow_bolt"),TEXT("drain"),    TEXT("raise_dead"),   TEXT("curse") },
                /*Chaos*/   { TEXT("fireball"),  TEXT("lightning"), TEXT("meteor"),       TEXT("doom_bolt") },
                /*Nature*/  { TEXT("earthquake"),TEXT("growth"),    TEXT("summon_beast"), TEXT("entangle") },
                /*Sorcery*/ { TEXT("teleport"),  TEXT("counter_spell"),TEXT("dispel"),    TEXT("confusion") },
                /*Arcane*/  { TEXT("magic_missile"),TEXT("ward"),   TEXT("enchant"),      TEXT("detect") },
                /*Binding*/ { TEXT("chains"),    TEXT("contract"),  TEXT("soul_trap"),    TEXT("dominate") },
                /*Spirit*/  { TEXT("astral_bolt"),TEXT("dream"),    TEXT("possession"),   TEXT("ghost_touch") },
                /*Glamour*/ { TEXT("illusion"),   TEXT("charm"),    TEXT("true_sight"),   TEXT("time_stop") },
            };
            int32 RealmIdx = static_cast<int32>(Info.Realm);
            if (RealmIdx < 0 || RealmIdx > 8) RealmIdx = 5; // arcane fallback

            int32 Slot = 0;
            switch (Info.EffectType)
            {
            case ECoMSpellEffect::Damage:    Slot = 0; break;
            case ECoMSpellEffect::Healing:   Slot = 0; break;
            case ECoMSpellEffect::UnitBuff:  Slot = 1; break;
            case ECoMSpellEffect::Summon:    Slot = 2; break;
            case ECoMSpellEffect::UnitDebuff:Slot = 3; break;
            case ECoMSpellEffect::Dispel:    Slot = 1; break;
            default:                         Slot = 0; break;
            }

            // Iconic spells get a bespoke sheet that takes precedence over the
            // per-realm/effect-slot fallback. Keeps Spell of Mastery from
            // looking like Magic Missile, Armageddon from looking like Fireball,
            // etc. Sheets registered in CoMSpellVFXSubsystem under "iconic.*".
            static const TMap<FName, FName> IconicVFX = {
                { FName("Sorcery_T4_Spell_Of_Mastery"), FName("iconic.spell_of_mastery") },
                { FName("Spell_of_Mastery"),            FName("iconic.spell_of_mastery") },
                { FName("Just_Cause"),                  FName("iconic.just_cause") },
                { FName("Eternal_Night"),               FName("iconic.eternal_night") },
                { FName("Armageddon_Clock"),            FName("iconic.armageddon") },
                { FName("Chaos_T4_Call_The_Void"),      FName("iconic.call_the_void") },
                { FName("Gaia_Force"),                  FName("iconic.gaia_force") },
                { FName("Gaias_Blessing"),              FName("iconic.gaia_force") },
                { FName("Heavenly_Light"),              FName("iconic.heavenly_light") },
                { FName("Wall_of_Fire"),                FName("iconic.wall_of_fire") },
                { FName("Suppress_Magic"),              FName("iconic.suppress_magic") },
                { FName("Flying_Fortress"),             FName("iconic.flying_fortress") },
                { FName("Volcano"),                     FName("iconic.volcano") },
                { FName("Great_Tree"),                  FName("iconic.great_tree") },
            };

            FName VFXID;
            if (const FName* IconicID = IconicVFX.Find(Info.SpellID))
            {
                VFXID = *IconicID;
            }
            else
            {
                VFXID = FName(*FString::Printf(TEXT("%s.%s"), RealmTag, PerRealmName[RealmIdx][Slot]));
            }
            VFX->PlayEffect(VFXID, World, World);
        }

        // SFX: cast cue at caster, impact cue at target. Cast happens on
        // overworld where the caster's first army is; if we don't know,
        // use the target tile.
        if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
        {
            FVector CasterWorld = World;
            if (Units)
            {
                const TArray<const FCoMArmyGroup*> CasterArmies =
                    Units->GetArmiesForWizard(Cast.CasterWizardId);
                if (CasterArmies.Num() > 0 && CasterArmies[0])
                {
                    CasterWorld = FVector(
                        CasterArmies[0]->Position.X * TileWorld + TileWorld * 0.5f,
                        CasterArmies[0]->Position.Y * TileWorld + TileWorld * 0.5f,
                        40.0f);
                }
            }
            Audio->PlaySpellSFX(Info.Realm, /*bImpact*/ false, CasterWorld);
            if (Info.EffectType == ECoMSpellEffect::Damage ||
                Info.EffectType == ECoMSpellEffect::UnitDebuff)
            {
                Audio->PlaySpellSFX(Info.Realm, /*bImpact*/ true, World);
            }
        }

        // Cinematic cast overlay for "big" spells: summons, global
        // enchantments, item creation. UI subscribes via the delegate.
        const bool bIsItemCreation =
            Cast.SpellId == FName(TEXT("enchant_item")) ||
            Cast.SpellId == FName(TEXT("create_artifact"));
        const bool bIsCinematic =
            Info.bSummon ||
            Info.EffectType == ECoMSpellEffect::Summon ||
            Info.EffectType == ECoMSpellEffect::GlobalEnchantment ||
            bIsItemCreation;
        if (bIsCinematic)
        {
            const FString DisplayName = Info.DisplayName.IsEmpty()
                ? Cast.SpellId.ToString()
                : Info.DisplayName.ToString();
            OnSpellCastCinematicRequested.Broadcast(Cast.CasterWizardId,
                Info.Realm, DisplayName,
                Cast.SpellId == FName(TEXT("create_artifact")));
        }
    }

    switch (Info.EffectType)
    {
    case ECoMSpellEffect::Damage:
    {
        // Damage = DamageBase (from spell) + Power/10. Direct + AOE handled the same way:
        // for AOE, hit every other unit at the target tile.
        const int32 BaseDmg = (Info.DamageBase > 0 ? Info.DamageBase : 4)
                              + FMath::Max(0, Power / 10);
        if (Info.bAOE && Units)
        {
            const TArray<const FCoMArmyGroup*> Armies = Units->GetArmiesAtPosition(
                ECoMPlane::Aurelith, ECoMMapLayer::Surface, Cast.TargetTile);
            for (const FCoMArmyGroup* A : Armies)
            {
                if (!A) continue;
                for (int32 UID : A->UnitIDs) { Units->ApplyDamage(UID, BaseDmg); }
            }
        }
        else if (Cast.TargetUnitId >= 0 && Units)
        {
            Units->ApplyDamage(Cast.TargetUnitId, BaseDmg);
        }
        LogResolve(TEXT("Damage"));
        break;
    }

    case ECoMSpellEffect::Healing:
    {
        const int32 HealAmt = (Info.HealAmount > 0 ? Info.HealAmount : 5)
                              + FMath::Max(0, Power / 12);
        if (Cast.TargetUnitId >= 0 && Units)
        {
            Units->ApplyHeal(Cast.TargetUnitId, HealAmt);
        }
        LogResolve(TEXT("Healing"));
        break;
    }

    case ECoMSpellEffect::UnitBuff:
    case ECoMSpellEffect::UnitDebuff:
    {
        // Append an enchantment instance to the target unit. The combat
        // subsystem will fold its modifiers when computing combat stats.
        if (Cast.TargetUnitId >= 0 && Units)
        {
            // Need mutable access — use the same pattern UnitSubsystem uses internally.
            const FCoMUnitInstance* Probe = Units->GetUnit(Cast.TargetUnitId);
            if (Probe)
            {
                FCoMEnchantmentInstance Ench;
                Ench.EnchantmentID    = static_cast<int32>(GFrameCounter);
                Ench.SpellID          = Cast.SpellId;
                Ench.CasterWizardIndex = Cast.CasterWizardId;
                // Buff persists until dispelled (-1) for ongoing spells, otherwise 5 turns.
                Ench.TurnsRemaining   = Info.bOngoing ? -1 : 5;
                Ench.UpkeepManaCost   = FFixed64(Info.UpkeepMana);

                // Map a spell-effect-driven stat modifier from the spell's tier.
                // Real game would source from the data asset; for now: buffs add
                // +Power/10 to Attack/Defense, debuffs subtract.
                const int32 Mag = FMath::Max(1, Power / 10);
                FCoMStatModifier StatMod;
                StatMod.StatName  = (Info.EffectType == ECoMSpellEffect::UnitBuff)
                    ? FName(TEXT("Attack")) : FName(TEXT("Defense"));
                StatMod.FlatBonus = (Info.EffectType == ECoMSpellEffect::UnitBuff)
                    ? FFixed64(Mag) : FFixed64(-Mag);
                Ench.Modifiers.Add(StatMod);

                // Stash by mutating the unit through ApplyHeal(0) — that returns a
                // mutable pointer effectively. Cleaner: add a public AddEnchantment.
                // For now, do it via direct cast since ApplyDamage/ApplyHeal already
                // assume mutability through Find.
                // Note: this works because GetUnit returns a const ptr to a TMap value
                // we know is owned by UnitSubsystem.
                FCoMUnitInstance* Mut = const_cast<FCoMUnitInstance*>(Probe);
                Mut->Enchantments.Add(Ench);
            }
        }
        LogResolve(Info.EffectType == ECoMSpellEffect::UnitBuff ? TEXT("Buff") : TEXT("Debuff"));
        break;
    }

    case ECoMSpellEffect::CityBuff:
    case ECoMSpellEffect::CityDebuff:
    {
        // City-targeted effects are recorded as global enchantments on the caster
        // for now (per-city enchantment list lives on FCoMCityData and is tracked
        // independently). Concrete city-modifier application will be folded into
        // RecalcCityOutputs in a future pass.
        LogResolve(TEXT("CityBuffDebuff"));
        break;
    }

    case ECoMSpellEffect::Summon:
    {
        // Spawn a unit for the caster at the spell's TargetTile if set,
        // otherwise at the caster's first army position. The summoned spec
        // is keyed by spell rarity for now; full data-asset binding lives in
        // CoMSpells once the spawn-spec map is registered.
        if (!Units) { LogResolve(TEXT("Summon (no units sub)")); break; }

        FIntPoint SpawnTile = Cast.TargetTile;
        ECoMPlane Plane = ECoMPlane::Aurelith;
        ECoMMapLayer Layer = ECoMMapLayer::Surface;
        if (SpawnTile.X < 0)
        {
            const TArray<const FCoMArmyGroup*> Armies = Units->GetArmiesForWizard(Cast.CasterWizardId);
            if (Armies.Num() > 0 && Armies[0])
            {
                SpawnTile = Armies[0]->Position;
                Plane = Armies[0]->Plane;
                Layer = Armies[0]->Layer;
            }
        }
        if (SpawnTile.X < 0) { LogResolve(TEXT("Summon (no spawn tile)")); break; }

        // Map realm + rarity to a bespoke summoned creature. Rare/Very Rare
        // summons conjure the realm's stronger creature; common/uncommon the
        // weaker one. (Earlier this reused racial units as placeholders.)
        const bool bHighTier =
            (Info.Rarity == ECoMSpellRarity::Rare || Info.Rarity == ECoMSpellRarity::VeryRare);
        FName SummonSpec;
        switch (Info.Realm)
        {
        case ECoMSpellRealm::Life:    SummonSpec = bHighTier ? TEXT("Summon_Angel")    : TEXT("Summon_GuardianSpirit"); break;
        case ECoMSpellRealm::Death:   SummonSpec = bHighTier ? TEXT("Summon_Wraith")   : TEXT("Summon_Skeleton");       break;
        case ECoMSpellRealm::Nature:  SummonSpec = bHighTier ? TEXT("Summon_Drake")    : TEXT("Summon_WarBear");        break;
        case ECoMSpellRealm::Chaos:   SummonSpec = bHighTier ? TEXT("Summon_Drake")    : TEXT("Summon_Hellhound");      break;
        case ECoMSpellRealm::Sorcery: SummonSpec = bHighTier ? TEXT("Summon_Gargoyle") : TEXT("Summon_PhantomWarrior"); break;
        case ECoMSpellRealm::Spirit:  SummonSpec = bHighTier ? TEXT("Summon_Wraith")   : TEXT("Summon_PhantomWarrior"); break;
        case ECoMSpellRealm::Arcane:  SummonSpec = bHighTier ? TEXT("Summon_Gargoyle") : TEXT("Summon_PhantomWarrior"); break;
        default:                      SummonSpec = bHighTier ? TEXT("Summon_Gargoyle") : TEXT("Summon_Hellhound");      break;
        }

        const int32 NewID = Units->SpawnUnitByName(SummonSpec, Plane, Layer, SpawnTile, Cast.CasterWizardId);
        if (NewID > 0)
        {
            // Fold the summon into an army at the spawn tile (its own if one is
            // there, otherwise a fresh army) so it can actually move and fight
            // instead of sitting loose on the map.
            int32 TargetArmy = INDEX_NONE;
            const TArray<const FCoMArmyGroup*> AtTile =
                Units->GetArmiesAtPosition(Plane, Layer, SpawnTile);
            for (const FCoMArmyGroup* A : AtTile)
            {
                if (A && A->OwnerWizardIndex == Cast.CasterWizardId)
                {
                    TargetArmy = A->ArmyGroupID;
                    break;
                }
            }
            if (TargetArmy == INDEX_NONE)
            {
                TargetArmy = Units->CreateArmy(Cast.CasterWizardId, Plane, Layer, SpawnTile);
            }
            Units->AddUnitToArmy(NewID, TargetArmy);
            LogResolve(*FString::Printf(TEXT("Summon -> unit %d (army %d)"), NewID, TargetArmy));
        }
        else
        {
            LogResolve(TEXT("Summon (spawn failed)"));
        }
        break;
    }

    case ECoMSpellEffect::GlobalEnchantment:
    {
        FCoMWizardMagicState& State = GetWizardMagic(Cast.CasterWizardId);
        const bool bWasNew = (State.ActiveEnchantments.AddUnique(Cast.SpellId) != INDEX_NONE);

        const FCoMGlobalEnchantmentDef& Def = CoMGlobalEnchantmentData::Get(Cast.SpellId);
        const FString DisplayName = Def.IsValid()
            ? Def.DisplayName.ToString() : Cast.SpellId.ToString();

        // Caster of Magic style: announce the new enchantment to every player.
        if (bWasNew)
        {
            OnGlobalEnchantmentChanged.Broadcast(Cast.CasterWizardId, Cast.SpellId,
                DisplayName, /*bActive*/ true);
        }

        // Spell of Mastery is an instant win — flag the victory subsystem.
        if (Def.Effect == ECoMEnchantEffect::InstantMagicVictory && GI)
        {
            if (UCoMVictorySubsystem* Victory = GI->GetSubsystem<UCoMVictorySubsystem>())
            {
                Victory->NotifySpellOfMasteryCast(Cast.CasterWizardId);
            }
        }

        LogResolve(*FString::Printf(TEXT("GlobalEnchantment: %s"), *DisplayName));
        break;
    }

    case ECoMSpellEffect::Dispel:
    {
        // Probabilistic dispel of the target's most valuable enchantment. The
        // chance scales the dispel's power against the enchantment's casting
        // cost (its "entrenchment"), so expensive globals resist and have real
        // staying power instead of being stripped every turn.
        if (Cast.TargetWizardId >= 0)
        {
            FCoMWizardMagicState& Target = GetWizardMagic(Cast.TargetWizardId);
            if (Target.ActiveEnchantments.Num() > 0)
            {
                int32 BestIdx = 0;
                int32 BestCost = -1;
                for (int32 i = 0; i < Target.ActiveEnchantments.Num(); ++i)
                {
                    const int32 C = CoMSpellDatabase::GetSpellInfo(Target.ActiveEnchantments[i]).CastingCost;
                    if (C > BestCost) { BestCost = C; BestIdx = i; }
                }
                const FName TargetEnch = Target.ActiveEnchantments[BestIdx];

                const int32 DispelPower = FMath::Max(1, Cast.EffectivePower);
                const int32 Entrench    = FMath::Max(1, BestCost);
                const int32 ChancePct   = FMath::Clamp((DispelPower * 100) / (DispelPower + Entrench), 5, 95);

                if (RngStream.RandRange(0, 99) < ChancePct)
                {
                    Target.ActiveEnchantments.RemoveAt(BestIdx);
                    const FCoMGlobalEnchantmentDef& DDef = CoMGlobalEnchantmentData::Get(TargetEnch);
                    const FString DName = DDef.IsValid() ? DDef.DisplayName.ToString() : TargetEnch.ToString();
                    OnGlobalEnchantmentChanged.Broadcast(Cast.TargetWizardId, TargetEnch, DName, /*bActive*/ false);
                    LogResolve(*FString::Printf(TEXT("Dispel SUCCESS: %s (%d%%)"), *DName, ChancePct));
                }
                else
                {
                    LogResolve(*FString::Printf(TEXT("Dispel FAILED (%d%%)"), ChancePct));
                }
            }
        }
        break;
    }

    case ECoMSpellEffect::Terrain:
    {
        if (Cast.SpellId == FName(TEXT("Nature_T2_Enchant_Road")) && GI)
        {
            // MoM-style Enchant Road: every existing built road in the world
            // becomes an enchanted road (RoadLevel 1 -> 2), granting the bigger
            // 0.25x movement-cost bonus to any unit walking it.
            UCoMWorldMapSubsystem* MapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();
            int32 Upgraded = 0;
            if (MapSub)
            {
                for (int32 Y = 0; Y < CoM::MAP_HEIGHT; ++Y)
                {
                    for (int32 X = 0; X < CoM::MAP_WIDTH; ++X)
                    {
                        FCoMTileData* T = MapSub->GetTileMutable(
                            ECoMPlane::Aurelith, ECoMMapLayer::Surface, X, Y);
                        if (T && T->RoadLevel == 1) { T->RoadLevel = 2; ++Upgraded; }
                    }
                }
            }
            LogResolve(*FString::Printf(TEXT("Enchant Road -> %d roads upgraded"), Upgraded));
        }
        else
        {
            LogResolve(TEXT("Terrain (placeholder)"));
        }
        break;
    }

    case ECoMSpellEffect::Divination:
    default:
        LogResolve(TEXT("Misc"));
        break;
    }
    (void)Cities; // City effects are placeholders; suppress unused-var warning.
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

// ─────────────────────────────────────────────────────────────────────────────
// Auto-Research
// ─────────────────────────────────────────────────────────────────────────────

void UCoMMagicSubsystem::SetAutoResearch(int32 WizardId, bool bEnable)
{
    if (bEnable)
    {
        AutoResearchMap.Add(WizardId, true);
    }
    else
    {
        AutoResearchMap.Remove(WizardId);
    }
}

bool UCoMMagicSubsystem::IsAutoResearchEnabled(int32 WizardId) const
{
    const bool* bEnabled = AutoResearchMap.Find(WizardId);
    return bEnabled && *bEnabled;
}

bool UCoMMagicSubsystem::AutoPickResearch(int32 WizardId)
{
    FCoMWizardMagicState& State = GetWizardMagic(WizardId);
    if (!State.CurrentResearchSpell.IsNone())
    {
        return false; // Already researching something.
    }

    TArray<FName> Researchable = GetResearchableSpells(WizardId);
    if (Researchable.Num() == 0)
    {
        return false;
    }

    // Build a priority order: strongest realm first, then secondary, then others.
    // Within each realm, prefer the cheapest spell.
    TArray<ECoMSpellRealm> RealmOrder;
    RealmOrder.Add(State.PrimaryRealm);
    if (State.SecondaryRealm != ECoMSpellRealm::None &&
        State.SecondaryRealm != State.PrimaryRealm)
    {
        RealmOrder.Add(State.SecondaryRealm);
    }
    // Add remaining realms.
    const UEnum* RealmEnum = StaticEnum<ECoMSpellRealm>();
    for (int32 i = 0; i < static_cast<int32>(ECoMSpellRealm::MAX); ++i)
    {
        ECoMSpellRealm Realm = static_cast<ECoMSpellRealm>(i);
        if (Realm != ECoMSpellRealm::None && !RealmOrder.Contains(Realm))
        {
            RealmOrder.Add(Realm);
        }
    }

    // Search each realm in priority order for the cheapest researchable spell.
    for (ECoMSpellRealm Realm : RealmOrder)
    {
        FName CheapestSpell = NAME_None;
        int32 CheapestCost = MAX_int32;

        for (const FName& SpellId : Researchable)
        {
            FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(SpellId);
            if (Info.Realm == Realm && Info.ResearchCost < CheapestCost)
            {
                CheapestCost = Info.ResearchCost;
                CheapestSpell = SpellId;
            }
        }

        if (CheapestSpell != NAME_None)
        {
            StartResearch(WizardId, CheapestSpell);
            return true;
        }
    }

    return false;
}
