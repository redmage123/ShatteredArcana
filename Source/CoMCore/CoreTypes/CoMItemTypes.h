// Copyright Shattered Arcana. All Rights Reserved.
// CoMItemTypes.h -- Per-instance magical item data: powers a wizard can forge
// onto a base item, and runtime instances that heroes equip.

#pragma once

#include "CoreMinimal.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMItemTypes.generated.h"

/**
 * The kind of effect a single item power produces.
 * Each forged item carries a list of powers; the subsystem turns those into
 * stat modifiers, granted skills, or spell charges when the item is equipped.
 */
UENUM(BlueprintType)
enum class ECoMItemPowerType : uint8
{
	StatBonus    UMETA(DisplayName = "Stat Bonus"),     // +N to a named stat
	GrantSkill   UMETA(DisplayName = "Grant Skill"),    // unlocks a skill while equipped
	SpellCharge  UMETA(DisplayName = "Spell Charges"),  // N uses of a specific spell
	Enchanted    UMETA(DisplayName = "Enchanted"),      // immune to Disjunction / Dispel
	MAX          UMETA(Hidden)
};

/**
 * One forged power entry on an item. Powers are picked from
 * UCoMItemSubsystem::GetPowerCatalog(); each catalog entry has fixed
 * Magnitude and ManaCost values.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMItemPower
{
	GENERATED_BODY()

	/** Catalog ID (e.g. "attack_plus_2", "flame_blade", "spell_save_minus_2"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PowerID;

	/** Effect category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMItemPowerType Type = ECoMItemPowerType::StatBonus;

	/** For StatBonus: stat key (e.g. "Attack"). For GrantSkill: skill ID. For SpellCharge: spell ID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Key;

	/** For StatBonus: bonus amount. For SpellCharge: charge count. Otherwise unused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Magnitude = 0;

	/** Mana cost contribution this power adds to the item's total forging cost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ManaCost = 0;

	/** Human-readable name shown on the hero/vault UI ("Flame Blade", "+2 Attack"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;

	/** Realm this power thematically belongs to. Drives the item's dominant
	 *  realm glow (Life=gold, Death=violet, Chaos=red, Nature=green,
	 *  Sorcery=blue, Arcane=silver, etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm Realm = ECoMSpellRealm::Arcane;
};

/**
 * Runtime instance of a forged magical item.
 * Powers are stored on the instance, NOT on the template, so two items of
 * the same base type can carry different forged powers.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMItemInstance
{
	GENERATED_BODY()

	/** Stable ID, unique within UCoMItemSubsystem. 0 = invalid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 InstanceID = 0;

	/** Optional reference to a base UCoMItemTemplateDataAsset (FName ItemID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TemplateID;

	/** Player-facing name. Defaults to template name unless renamed during forging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;

	/** Equipment slot category — enforced when equipping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMItemSlot Slot = ECoMItemSlot::Weapon;

	/** Forged powers carried by this instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCoMItemPower> Powers;

	/** Total mana paid to forge this instance (sum of Powers[i].ManaCost + base). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TotalManaCost = 0;

	/** True if this is a major forging ("Create Artifact"). Cosmetic + cost tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bArtifact = false;

	/** Wizard who owns this item in their vault. -1 = unowned / on the map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 OwnerWizardIndex = -1;

	/** HeroUnitID currently equipping this item. 0 = unequipped (in vault). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EquippedByHeroID = 0;

	/** Art variant slug (e.g. "sword_02", "ring_01") -- maps to
	 *  /Game/UI/Items/<ArtVariant>. The forge picks one based on the slot
	 *  and the dominant power; the player can re-pick during forging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ArtVariant;

	/** Realm whose colour drives the picture's aura. Set at forge time from
	 *  the most expensive power's realm, then displayed as a tinted glow
	 *  in the vault / forge / hero panels (CoM-style). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECoMSpellRealm DominantRealm = ECoMSpellRealm::Arcane;

	/** Maximum enchantments allowed on this instance. Defaults to 4
	 *  (CoM convention); higher tiers allow 6 or 8. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxEnchantments = 4;

	/** Turns remaining to fully forge this item. 0 = ready for use. While
	 *  > 0 the item lives in the wizard's forging queue, not the vault. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ForgeTurnsRemaining = 0;

	/** Total turns this item required to forge -- so the UI can show a
	 *  progress fraction (Remaining / Total). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ForgeTurnsTotal = 0;
};
