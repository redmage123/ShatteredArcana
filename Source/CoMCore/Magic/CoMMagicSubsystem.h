// Copyright Mythforge Studios. All Rights Reserved.
// CoMMagicSubsystem.h — Spell casting, mana, rituals, runes, spell research, wizard progression
// Phase 7 — Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMMagicSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// WIZARD MAGIC STATE
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMWizardMagicState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WizardId = -1;

    /** Current mana pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentMana = 0;

    /** Maximum mana capacity (grows with nodes, buildings, level). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxMana = 100;

    /** Mana generated per turn from all sources. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ManaPerTurn = 10;

    /** Mana allocated to spell research per turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ResearchAllocation = 0;

    /** Mana allocated to maintaining enchantments per turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaintenanceCost = 0;

    /** Current spell being researched. Empty if none. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CurrentResearchSpell;

    /** Research progress (0 to required cost). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ResearchProgress = 0;

    /** Known spells (spell IDs). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> KnownSpells;

    /** Active enchantments (global spells maintained per turn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ActiveEnchantments;

    /** Wizard's primary magic realm (determines available spells). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMSpellRealm PrimaryRealm = ECoMSpellRealm::Arcane;

    /** Secondary realm (if any — half-power). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMSpellRealm SecondaryRealm = ECoMSpellRealm::None;

    /** Wizard casting skill (affects spell power, research speed). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CastingSkill = 10;

    /** Controlled mana nodes on the map. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FIntPoint> ControlledNodes;

    /** Number of spell books in each realm (determines available tiers). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMSpellRealm, int32> SpellBooks;
};

// ─────────────────────────────────────────────────────────────────────────────
// SPELL INSTANCE (cast on the overworld or in combat)
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMSpellCast
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SpellId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CasterWizardId = -1;

    /** Target type determines how target fields are used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMSpellScope Scope = ECoMSpellScope::Combat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint TargetTile = FIntPoint(-1, -1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetUnitId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetCityId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetWizardId = -1;

    /** Mana cost (may be modified by caster skill, retorts). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ManaCost = 0;

    /** Turns to cast (overworld spells may take multiple turns). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CastingTime = 1;

    /** Turns remaining. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TurnsRemaining = 0;

    /** Effective power (base * skill * modifiers). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EffectivePower = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// RITUAL (multi-turn powerful spell)
// ─────────────────────────────────────────────────────────────────────────────

// FCoMActiveRitual is defined in CoMStructs.h

// ─────────────────────────────────────────────────────────────────────────────
// RUNE INSCRIPTION
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMActiveRune
{
    GENERATED_BODY()

    /** Unique instance ID for this rune inscription. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InstanceId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RuneId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InscribedByWizardId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMRuneTarget TargetType = ECoMRuneTarget::Item;

    /** Target item/unit/city ID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetId = -1;

    /** Power level of inscription (affected by casting skill). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Power = 10;

    /** Remaining charges (-1 = permanent). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ChargesLeft = -1;

    /** Turn inscribed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InscribedTurn = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// MAGIC SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMMagicSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── Wizard Magic State ───────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Magic")
    FCoMWizardMagicState& GetWizardMagic(int32 WizardId);

    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetCurrentMana(int32 WizardId) const;

    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetManaPerTurn(int32 WizardId) const;

    UFUNCTION(BlueprintCallable, Category = "Magic")
    void SetResearchAllocation(int32 WizardId, int32 ManaPerTurn);

    // ── Mana Nodes ───────────────────────────────────────────────────────────

    /** Claim a mana node on the map (requires controlling territory). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool ClaimManaNode(int32 WizardId, FIntPoint NodeTile);

    /** Release a mana node. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void ReleaseManaNode(int32 WizardId, FIntPoint NodeTile);

    /** Get mana output of a specific node (affected by realm, corruption). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetNodeManaOutput(FIntPoint NodeTile, ECoMSpellRealm WizardRealm) const;

    // ── Spell Casting ────────────────────────────────────────────────────────

    /** Check if a wizard can cast a spell (known, enough mana, valid target). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool CanCastSpell(int32 WizardId, FName SpellId, const FCoMSpellCast& CastParams) const;

    /** Begin casting a spell. Returns true if casting started. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool CastSpell(const FCoMSpellCast& CastParams);

    /** Cancel a spell being cast. Mana is partially refunded. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void CancelCasting(int32 WizardId);

    /** Get the spell currently being cast (nullptr if none). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    FCoMSpellCast GetCurrentCasting(int32 WizardId) const;

    /** Calculate effective power of a spell cast. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 CalculateSpellPower(int32 WizardId, FName SpellId) const;

    // ── Spell Research ───────────────────────────────────────────────────────

    /** Get available spells to research (based on spell books, realm). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    TArray<FName> GetResearchableSpells(int32 WizardId) const;

    /** Begin researching a spell. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool StartResearch(int32 WizardId, FName SpellId);

    /** Get research progress as percentage (0-100). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetResearchProgress(int32 WizardId) const;

    /** Get estimated turns to complete current research. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetResearchETATurns(int32 WizardId) const;

    // ── Rituals ──────────────────────────────────────────────────────────────

    /** Begin a ritual (multi-turn, resource-consuming powerful spell). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool BeginRitual(int32 WizardId, FName RitualId, int32 ManaChannelPerTurn);

    /** Cancel an active ritual. Components are lost. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void CancelRitual(int32 WizardId, FName RitualId);

    /** Get all active rituals for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    TArray<FCoMActiveRitual> GetActiveRituals(int32 WizardId) const;

    /** Interrupt a ritual (e.g. from enemy action). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool InterruptRitual(int32 WizardId, FName RitualId);

    // ── Rune Inscription ─────────────────────────────────────────────────────

    /** Inscribe a rune on an item, unit, or city. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool InscribeRune(int32 WizardId, FName RuneId, ECoMRuneTarget TargetType, int32 TargetId);

    /** Remove a rune inscription. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void RemoveRune(int32 RuneInstanceId);

    /** Get all active runes on a target. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    TArray<FCoMActiveRune> GetRunesOnTarget(ECoMRuneTarget TargetType, int32 TargetId) const;

    // ── Enchantments (global persistent spells) ──────────────────────────────

    /** Maintain a global enchantment (costs mana per turn). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool MaintainEnchantment(int32 WizardId, FName SpellId);

    /** Dismiss a global enchantment. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void DismissEnchantment(int32 WizardId, FName SpellId);

    /** Dispel an enemy's enchantment (contested roll). */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool DispelEnchantment(int32 CasterWizardId, int32 TargetWizardId, FName SpellId);

    // ── Counter-magic ────────────────────────────────────────────────────────

    /** Attempt to counter an enemy spell being cast. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool CounterSpell(int32 CounterCasterWizardId, int32 TargetWizardId, int32 ManaSpent);

    // ── Turn Processing ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Magic")
    void ProcessTurn(int32 CurrentTurn);

private:
    /** Calculate total mana income for a wizard. */
    int32 CalculateManaIncome(int32 WizardId) const;

    /** Resolve a completed spell casting. */
    void ResolveSpell(FCoMSpellCast& Cast);

    /** Resolve a completed ritual. */
    void ResolveRitual(FCoMActiveRitual& Ritual);

    /** Advance research for a wizard. */
    void AdvanceResearch(int32 WizardId);

    /** Consume rune charges. */
    void TickRunes();

    UPROPERTY()
    TMap<int32, FCoMWizardMagicState> WizardMagicStates;

    UPROPERTY()
    TMap<int32, FCoMSpellCast> ActiveCastings;

    UPROPERTY()
    TArray<FCoMActiveRitual> ActiveRituals;

    UPROPERTY()
    TArray<FCoMActiveRune> ActiveRunes;

    int32 NextRuneInstanceId = 1;

    FRandomStream RngStream;
};
