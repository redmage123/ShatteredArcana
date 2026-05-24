// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMHeroSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroDeserted, int32, HeroUnitID, FFixed64, FinalLoyalty);

/**
 * A pending tavern offer — a hero who is available for hire to a specific wizard
 * until ExpiresOnTurn. The player accepts or declines; AI calls AcceptOffer
 * via its tactical pass.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMHeroOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32   OfferID         = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32   WizardIndex     = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString HeroName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8   Tier            = 0; // ECoMHeroTier as uint8
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8   HeroClass       = 0; // ECoMHeroClass as uint8
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32   GoldCost        = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32   ExpiresOnTurn   = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroOffered, const FCoMHeroOffer&, Offer);


// ═══════════════════════════════════════════════════════════════════════════
// Hero Tiers — based on Master of Magic / Caster of Magic hero/champion system
// expanded to four tiers for Shattered Arcana
// ═══════════════════════════════════════════════════════════════════════════

UENUM(BlueprintType)
enum class ECoMHeroTier : uint8
{
	/** Common sellswords, scouts, hedge wizards. Appear in taverns. */
	Adventurer  UMETA(DisplayName = "Adventurer"),   // Tier 1 — CoM basic Heroes

	/** Proven warriors and mages. Attracted by Fame >= 5. */
	Hero        UMETA(DisplayName = "Hero"),          // Tier 2 — CoM mid-tier Heroes

	/** Legendary named figures. Require Fame >= 15. */
	Champion    UMETA(DisplayName = "Champion"),      // Tier 3 — CoM Champions (Roland, Mortu, Torin)

	/** Planar entities, ascended mortals. Quest-only recruitment. One per plane. */
	Demigod     UMETA(DisplayName = "Demigod"),       // Tier 4 — New for Shattered Arcana

	MAX UMETA(Hidden)
};

/** Per-tier configuration: stat ranges, ability slots, hiring mechanics. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMHeroTierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMHeroTier Tier = ECoMHeroTier::Adventurer;

	// Stat multiplier applied to base hero stats (1.0 = normal)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 StatMultiplier = FFixed64(1);

	// Number of ability/skill slots this tier gets
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AbilitySlots = 1;

	// Maximum level cap for this tier
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxLevel = 6;

	// XP multiplier (higher tiers need more XP per level)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 XPMultiplier = FFixed64(1);

	// Hiring cost in gold (0 = cannot be hired with gold)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HireCostMin = 25;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HireCostMax = 50;

	// Minimum wizard Fame required to attract this tier
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FameRequired = 0;

	// Can this tier be hired from taverns?
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHirableFromTavern = true;

	// Must this tier be earned through a quest?
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bQuestRequired = false;

	// Maximum number of this tier a single wizard can have simultaneously
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxPerWizard = 4;

	// Base loyalty at recruitment
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 BaseLoyalty = FFixed64(100);
};


// ═══════════════════════════════════════════════════════════════════════════
// Hero Classes — every hero has a class that determines their ability set
// Based on CoM hero/champion types + Psyker/Warlock expansion
// ═══════════════════════════════════════════════════════════════════════════

UENUM(BlueprintType)
enum class ECoMHeroClass : uint8
{
	// ── Wizard-aligned classes (traditional spellcasting) ──────────────
	Fighter      UMETA(DisplayName = "Fighter"),       // CoM: melee combat specialist
	Bowman       UMETA(DisplayName = "Bowman"),         // CoM: ranged combat
	Cleric       UMETA(DisplayName = "Cleric"),          // CoM Priestess: Life magic + healing
	Magician     UMETA(DisplayName = "Magician"),       // CoM: arcane spellcasting
	Paladin      UMETA(DisplayName = "Paladin"),        // CoM Champion: holy warrior
	Assassin     UMETA(DisplayName = "Assassin"),       // CoM: stealth + poison
	Bard         UMETA(DisplayName = "Bard"),           // CoM: morale + buff specialist
	Beastmaster  UMETA(DisplayName = "Beastmaster"),    // CoM: summon + beast control
	Necromancer  UMETA(DisplayName = "Necromancer"),    // Death magic specialist
	Druid        UMETA(DisplayName = "Druid"),          // Nature magic + terrain

	// ── Psyker-aligned classes (willpower-based, no spellbook) ────────
	Psyker       UMETA(DisplayName = "Psyker"),         // Willpower + telepathy + mind control
	Telepath     UMETA(DisplayName = "Telepath"),        // Psyker specialization: intel + command reading
	Dominator    UMETA(DisplayName = "Dominator"),       // Psyker specialization: mass mind control
	Empath       UMETA(DisplayName = "Empath"),          // Psyker specialization: morale + diplomacy

	// ── Warlock-aligned classes (pact-based, soul economy) ────────────
	Warlock      UMETA(DisplayName = "Warlock"),         // Pact Debt + entity bargaining
	WitchHunter  UMETA(DisplayName = "Witch Hunter"),    // Anti-magic specialist, pact-breaker
	Demonologist UMETA(DisplayName = "Demonologist"),    // Warlock spec: Abyssal/Infernal pacts
	Feysworn     UMETA(DisplayName = "Feysworn"),        // Warlock spec: Feywild/glamour pacts
	SoulReaper   UMETA(DisplayName = "Soul Reaper"),     // Warlock spec: soul fragment harvester

	// ── Hybrid / Demigod-tier classes ─────────────────────────────────
	DragonKnight UMETA(DisplayName = "Dragon Knight"),   // Bonded to a dragon, tier 3-4 only
	PlanarWalker UMETA(DisplayName = "Planar Walker"),   // Cross-plane specialist, tier 3-4
	Warlord      UMETA(DisplayName = "Warlord"),         // CoM Warlord: army command buffs
	Ascendant    UMETA(DisplayName = "Ascendant"),       // Demigod tier only: planar entity

	MAX UMETA(Hidden)
};

/** Maps which hero classes are available at each tier. */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMHeroClassTierEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMHeroClass HeroClass = ECoMHeroClass::Fighter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMHeroTier MinTier = ECoMHeroTier::Adventurer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMHeroTier MaxTier = ECoMHeroTier::Demigod;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ECoMWizardClass RequiredWizardClass = ECoMWizardClass::MAX; // MAX = any
};

/**
 * Manages hero personalities, inter-hero relationships, loyalty mechanics,
 * tiers, and classes. Dragon responsibility has moved to UCoMDragonSubsystem.
 */
UCLASS()
class COMCORE_API UCoMHeroSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Re-seed this subsystem's RNG from a per-game master seed so playtests vary
	 *  by seed and stay reproducible. Mixes a per-subsystem salt so each stream is
	 *  independent. Called from UCoMPlaytestSubsystem::BootstrapGame. */
	void ReseedRandom(int32 MasterSeed);


	// -----------------------------------------------------------------
	// Hero Tiers (CoM-based four-tier system)
	// -----------------------------------------------------------------

	/** Get the static tier configuration for a hero tier. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	static FCoMHeroTierConfig GetTierConfig(ECoMHeroTier Tier);

	/** Get the tier of a specific hero. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	ECoMHeroTier GetHeroTier(int32 HeroUnitID) const;

	/** Set the tier for a hero (called during spawn/recruitment). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void SetHeroTier(int32 HeroUnitID, ECoMHeroTier Tier);

	/** Register hero ownership (called during recruitment). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void SetHeroOwner(int32 HeroUnitID, int32 WizardIndex);

	/** Returns the wizard index that owns the given hero, or -1 if unowned. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	int32 GetHeroOwner(int32 HeroUnitID) const;

	/** Check if a wizard qualifies to recruit a hero of the given tier. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	bool CanRecruitTier(int32 WizardIndex, ECoMHeroTier Tier, int32 WizardFame) const;


	/** Get the class of a hero. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	ECoMHeroClass GetHeroClass(int32 HeroUnitID) const;

	/** Set the class for a hero. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void SetHeroClass(int32 HeroUnitID, ECoMHeroClass HeroClass);

	/** Get all hero classes available at a given tier, optionally filtered by wizard class. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	static TArray<ECoMHeroClass> GetAvailableClasses(ECoMHeroTier Tier, ECoMWizardClass WizardClass);

	/** Count how many heroes of a given tier a wizard currently has. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	int32 CountHeroesOfTier(int32 WizardIndex, ECoMHeroTier Tier) const;

	// -----------------------------------------------------------------
	// Hero Personality & Loyalty
	// -----------------------------------------------------------------

	/** Assign random personality traits to a hero unit. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void InitializeHeroPersonality(int32 HeroUnitID, UPARAM(ref) FRandomStream& Rng);

	/** Return the personality data for a hero, or a default if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	FCoMHeroPersonality GetPersonality(int32 HeroUnitID) const;

	/** Current loyalty value for a hero (default 100 if not initialized). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	FFixed64 GetLoyalty(int32 HeroUnitID) const;

	/** Add (or subtract) loyalty. Clamped to [0, 100]. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void ModifyLoyalty(int32 HeroUnitID, FFixed64 Delta);

	/**
	 * Roll against the hero's loyalty to determine desertion.
	 * Below 30: 5% chance. Below 10: 25% chance.
	 * Returns true if the hero deserts.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	bool CheckDesertion(int32 HeroUnitID);

	// -----------------------------------------------------------------
	// Hero Relationships
	// -----------------------------------------------------------------

	/** Modify (or create) a relationship between two heroes. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void UpdateRelationship(int32 HeroA, int32 HeroB, ECoMHeroRelationType Type, FFixed64 StrengthDelta);

	/** All relationships involving the given hero. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	TArray<FCoMHeroRelationship> GetRelationshipsForHero(int32 HeroUnitID) const;

	// -----------------------------------------------------------------
	// Hero Turn Processing
	// -----------------------------------------------------------------

	/** Per-turn: age relationships, decay loyalty, check desertion. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void ProcessHeroTurn();

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Heroes")
	FOnHeroDeserted OnHeroDeserted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Heroes")
	FOnHeroOffered OnHeroOffered;

	// -- Tavern offers ------------------------------------------------------

	/**
	 * Roll per-wizard hero offers for the turn. Higher Fame raises both the
	 * chance and the tier of generated offers. Called from ProcessHeroTurn.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void RollHeroOffers(int32 CurrentTurn);

	/** Active (un-expired) offers for a wizard. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Heroes")
	TArray<FCoMHeroOffer> GetOffersForWizard(int32 WizardIndex) const;

	/**
	 * Accept an offer: deducts gold from the wizard's player state, spawns a
	 * hero unit at the wizard's capital, registers tier/class/owner.
	 * Returns the new HeroUnitID, or -1 on failure (insufficient gold,
	 * already removed offer, no capital).
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	int32 AcceptHeroOffer(int32 OfferID);

	/** Decline an offer — removes it from the queue. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Heroes")
	void DeclineHeroOffer(int32 OfferID);

	// ── Save/Load Export/Import ───────────────────────────────────────────

	/** Export all hero state for save serialization. */
	void ExportAll(TArray<FCoMHeroPersonality>& OutPersonalities,
	               TArray<int32>& OutPersonalityUnitIDs,
	               TArray<FCoMHeroRelationship>& OutRelationships,
	               TArray<int32>& OutTierUnitIDs, TArray<uint8>& OutTierValues,
	               TArray<int32>& OutClassUnitIDs, TArray<uint8>& OutClassValues,
	               TArray<int32>& OutOwnerUnitIDs, TArray<int32>& OutOwnerWizardIDs) const;

	/** Import hero state from save data (clears existing state). */
	void ImportAll(const TArray<FCoMHeroPersonality>& InPersonalities,
	               const TArray<int32>& InPersonalityUnitIDs,
	               const TArray<FCoMHeroRelationship>& InRelationships,
	               const TArray<int32>& InTierUnitIDs, const TArray<uint8>& InTierValues,
	               const TArray<int32>& InClassUnitIDs, const TArray<uint8>& InClassValues,
	               const TArray<int32>& InOwnerUnitIDs, const TArray<int32>& InOwnerWizardIDs);

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Compute per-turn loyalty decay for a hero based on personality. */
	FFixed64 ComputeLoyaltyDecay(const FCoMHeroPersonality& Personality) const;

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMHeroPersonality> Personalities;

	/** Hero tier assignments. */
	UPROPERTY()
	TMap<int32, ECoMHeroTier> HeroTiers;

	/** Hero class assignments. */
	UPROPERTY()
	TMap<int32, ECoMHeroClass> HeroClasses;

	/** Maps HeroUnitID -> OwnerWizardIndex for per-wizard hero counting. */
	UPROPERTY()
	TMap<int32, int32> HeroOwners;

	UPROPERTY()
	TArray<FCoMHeroRelationship> Relationships;

	/** Outstanding tavern offers, keyed by OfferID. */
	UPROPERTY()
	TArray<FCoMHeroOffer> PendingOffers;

	int32 NextOfferID = 1;

	FRandomStream RngStream;
};
