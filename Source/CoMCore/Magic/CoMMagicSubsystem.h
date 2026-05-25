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

    // ── Per-Realm Mana Pools (MoM-style) ─────────────────────────────────

    /**
     * Mana pool per spell realm. Realm-typed mana comes from controlling
     * mana nodes of that realm. Casting a spell draws from its realm pool
     * first, then from the generic pool (CurrentMana) as overflow.
     *
     * Example: Controlling 2 Nature nodes gives Nature mana income.
     * Casting a Nature spell costs from RealmMana[Nature] first.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMSpellRealm, int32> RealmMana;

    /** Per-realm mana income per turn (from nodes of that realm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMSpellRealm, int32> RealmManaPerTurn;

    /** Maximum per-realm mana storage (scales with books in that realm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMSpellRealm, int32> RealmManaMax;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSpellCastCinematicRequested,
    int32, CasterWizardIndex,
    ECoMSpellRealm, Realm,
    FString, SpellDisplayName,
    bool, bIsArtifact);

/**
 * Fired whenever a wizard gains or loses a global enchantment. Every player is
 * notified (Caster of Magic style) — the UI routes it to the notification
 * centre and the enchantments panel. bActive = newly cast (true) or
 * dispelled/lapsed (false).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGlobalEnchantmentChanged,
    int32, CasterWizardIndex,
    FName, SpellID,
    FString, SpellDisplayName,
    bool, bActive);

UCLASS()
class COMCORE_API UCoMMagicSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Re-seed this subsystem's RNG from a per-game master seed so playtests vary
     *  by seed and stay reproducible. Mixes a per-subsystem salt so each stream is
     *  independent. Called from UCoMPlaytestSubsystem::BootstrapGame. */
    void ReseedRandom(int32 MasterSeed);

    /** Fired when a "big" spell resolves (summon / global enchant / artifact). UI binds. */
    UPROPERTY(BlueprintAssignable, Category = "Magic")
    FOnSpellCastCinematicRequested OnSpellCastCinematicRequested;

    /** Fired when any wizard gains/loses a global enchantment (all-player alert). */
    UPROPERTY(BlueprintAssignable, Category = "Magic")
    FOnGlobalEnchantmentChanged OnGlobalEnchantmentChanged;

    // ── Global Enchantments (Caster of Magic style) ──────────────────────────

    /**
     * Apply all active global enchantments' per-turn effects (unrest reduction,
     * city decay, healing, mana boons, doomsday countdown, map vision, ...).
     * Called once per turn from ProcessTurn.
     */
    void ProcessGlobalEnchantments(int32 CurrentTurn);

    /** True if WizardId currently maintains the given global enchantment. */
    UFUNCTION(BlueprintCallable, Category = "Magic|Enchantments")
    bool IsGlobalEnchantmentActive(int32 WizardId, FName SpellID) const;

    /** True if any wizard other than WizardId maintains SpellID. */
    UFUNCTION(BlueprintCallable, Category = "Magic|Enchantments")
    bool IsEnchantmentActiveByOther(int32 WizardId, FName SpellID) const;

    /**
     * Multiplier applied to WizardId's spell costs from hostile global
     * enchantments (e.g. an enemy's Suppress Magic). 1.0 = no penalty.
     */
    UFUNCTION(BlueprintCallable, Category = "Magic|Enchantments")
    float GetIncomingCastCostMultiplier(int32 WizardId) const;

    /** Voluntarily cancel one of WizardId's global enchantments (stops upkeep). */
    UFUNCTION(BlueprintCallable, Category = "Magic|Enchantments")
    void CancelGlobalEnchantment(int32 WizardId, FName SpellID);

    /** Active global enchantments for one wizard. */
    UFUNCTION(BlueprintCallable, Category = "Magic|Enchantments")
    TArray<FName> GetActiveEnchantments(int32 WizardId) const;

    /** Every active global enchantment in play, paired with its owner wizard. */
    void GetAllActiveEnchantments(TArray<int32>& OutOwners, TArray<FName>& OutSpellIDs) const;

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

    // ── Spirit Melding (MoM-style node claiming) ─────────────────────────────

    /**
     * Meld a magic spirit with a mana node to claim it for a wizard.
     * The spirit type must match the node's realm (or be Arcane for any realm).
     * Node must be unguarded (guardian defeated) and unclaimed.
     * On success: node produces realm-typed mana each turn for this wizard.
     * @return true if melding succeeded.
     */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool MeldSpiritWithNode(int32 WizardId, FIntPoint NodeTile, ECoMSpellRealm SpiritRealm);

    /**
     * Dispel the spirit melded at a node, releasing it from wizard control.
     * Can be done to own nodes voluntarily, or to enemy nodes via Dispel Magic.
     */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void DispelNodeSpirit(FIntPoint NodeTile);

    /** Get the realm-typed mana income for a wizard from all their controlled nodes. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    TMap<ECoMSpellRealm, int32> GetRealmManaIncome(int32 WizardId) const;

    /**
     * Get a wizard's current realm-specific mana pool.
     * Returns 0 for realms with no pool.
     */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    int32 GetRealmMana(int32 WizardId, ECoMSpellRealm Realm) const;

    /**
     * Spend mana to cast a spell. Draws from realm pool first, then generic.
     * @return true if enough mana was available and was spent.
     */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool SpendManaForSpell(int32 WizardId, ECoMSpellRealm SpellRealm, int32 Cost);

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

    // ── Auto-Research ────────────────────────────────────────────────────────

    /** Enable or disable auto-research for a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    void SetAutoResearch(int32 WizardId, bool bEnable);

    /** Check if auto-research is enabled for a wizard. */
    UFUNCTION(BlueprintPure, Category = "Magic")
    bool IsAutoResearchEnabled(int32 WizardId) const;

    /** Auto-pick the next research spell for a wizard. Returns true if a spell was selected. */
    UFUNCTION(BlueprintCallable, Category = "Magic")
    bool AutoPickResearch(int32 WizardId);

    // ── Turn Processing ──────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Magic")
    void ProcessTurn(int32 CurrentTurn);

    // ── Save/Load Export/Import ─────────────────────────────────────────────

    /** Export all magic state for save serialization. */
    void ExportAll(TArray<FCoMWizardMagicState>& OutMagicStates,
                   TArray<FCoMSpellCast>& OutCastings,
                   TArray<int32>& OutCastingWizardIDs,
                   TArray<FCoMActiveRitual>& OutRituals,
                   TArray<FCoMActiveRune>& OutRunes) const;

    /** Import magic state from save data (clears existing state). */
    void ImportAll(const TArray<FCoMWizardMagicState>& InMagicStates,
                   const TArray<FCoMSpellCast>& InCastings,
                   const TArray<int32>& InCastingWizardIDs,
                   const TArray<FCoMActiveRitual>& InRituals,
                   const TArray<FCoMActiveRune>& InRunes);

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

    /** Per-wizard auto-research toggle. */
    UPROPERTY()
    TMap<int32, bool> AutoResearchMap;

    FRandomStream RngStream;
};
