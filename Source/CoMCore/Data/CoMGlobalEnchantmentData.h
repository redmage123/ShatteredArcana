// Copyright Mythforge Studios. All Rights Reserved.
// CoMGlobalEnchantmentData.h -- Static descriptor table for Caster-of-Magic
// style global enchantments. Each entry binds a spell ID to its mechanical
// effect, who it affects, an effect magnitude, its tarot-card art, and flavor
// text. The effect engine (UCoMMagicSubsystem::ProcessGlobalEnchantments and
// the query helpers) reads these descriptors; the UI panel reads the art and
// flavor.

#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMEnums.h"

/** What a global enchantment mechanically does. One per distinct effect. */
UENUM(BlueprintType)
enum class ECoMEnchantEffect : uint8
{
	None              UMETA(DisplayName = "None"),

	// ── Caster-beneficial (push) ─────────────────────────────────────────
	ReduceOwnUnrest    UMETA(DisplayName = "Reduce Own Unrest"),    // Just Cause
	HealOwnUnits       UMETA(DisplayName = "Heal Own Units"),       // Herb Mastery
	BoostOwnMana       UMETA(DisplayName = "Boost Own Mana Income"), // Gaia Force
	GlobalMapVision    UMETA(DisplayName = "Global Map Vision"),     // Nature Awareness
	RaiseSlainAsZombies UMETA(DisplayName = "Raise Slain As Zombies"), // Zombie Mastery
	BuffOwnUnitsCombat UMETA(DisplayName = "Buff Own Units In Combat"), // Crusade / Eternal Night

	// ── Caster-beneficial (query / passive) ──────────────────────────────
	RevealEnemyMagic   UMETA(DisplayName = "Reveal Enemy Magic"),    // Detect Magic
	RevealEnemyPlans   UMETA(DisplayName = "Reveal Enemy Plans"),    // Diplomatic Scrying / Dream Vision
	ScryEnchantments   UMETA(DisplayName = "Scry Enchantments"),     // Aether Binding

	// ── Enemy-detrimental / world catastrophe (push) ─────────────────────
	RaiseEnemyCastCost UMETA(DisplayName = "Raise Enemy Cast Cost"), // Suppress Magic
	GlobalCityDecay    UMETA(DisplayName = "Global City Decay"),     // Great Wasting / Call the Void / Nature's Wrath
	GlobalChaosBoon    UMETA(DisplayName = "Global Chaos Boon"),     // Chaos Surge / Doom Mastery
	DoomsdayCountdown  UMETA(DisplayName = "Doomsday Countdown"),     // Armageddon Clock

	// ── Pacifying / protective ───────────────────────────────────────────
	GlobalPeaceAura    UMETA(DisplayName = "Global Peace Aura"),     // Tranquility
	ConsecrateRealm    UMETA(DisplayName = "Consecrate Realm"),      // Consecration

	// ── Instant / special ────────────────────────────────────────────────
	InstantMagicVictory UMETA(DisplayName = "Instant Magic Victory"), // Spell of Mastery
	BanishReturn        UMETA(DisplayName = "Banish / Return"),       // Spell of Return
	GlobalDeathToll      UMETA(DisplayName = "Global Death Toll"),    // Death Wish
};

/** Who a global enchantment's effect applies to. */
UENUM(BlueprintType)
enum class ECoMEnchantScope : uint8
{
	Caster      UMETA(DisplayName = "Caster Only"),
	AllWizards  UMETA(DisplayName = "All Wizards"),
	Enemies     UMETA(DisplayName = "Enemies Of Caster"),
};

/** Full descriptor for one global enchantment. */
struct COMCORE_API FCoMGlobalEnchantmentDef
{
	FName              SpellID;
	FText              DisplayName;
	ECoMSpellRealm     Realm    = ECoMSpellRealm::Arcane;
	ECoMEnchantEffect  Effect   = ECoMEnchantEffect::None;
	ECoMEnchantScope   Scope    = ECoMEnchantScope::Caster;

	/** Effect strength — meaning depends on Effect (unrest pts, mana, %, etc.). */
	int32              Magnitude = 0;

	/** Card art slug -> /Game/UI/Enchantments/T_Enchant_<Slug>. */
	FString            CardImageSlug;

	/** One-line description shown in the enchantment panel / notification. */
	FText              FlavorText;

	bool IsValid() const { return !SpellID.IsNone(); }
};

/**
 * Static registry of every global enchantment. Lazily built on first access.
 * Several spell IDs (e.g. the tier-generated "Sorcery_T4_Spell_Of_Mastery" and
 * the canonical "Spell_of_Mastery") map to the same descriptor.
 */
class COMCORE_API CoMGlobalEnchantmentData
{
public:
	/** Returns the descriptor for SpellID, or an invalid def if not an enchantment. */
	static const FCoMGlobalEnchantmentDef& Get(FName SpellID);

	/** True if SpellID is a registered global enchantment. */
	static bool IsGlobalEnchantment(FName SpellID);

	/** All registered descriptors (one per distinct SpellID, includes aliases). */
	static const TArray<FCoMGlobalEnchantmentDef>& GetAll();
};
