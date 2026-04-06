// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerState.h — Per-wizard runtime state for Caster of Magic.
// COM-027
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "CoreTypes/CoMEnums.h"
#include "CoreTypes/CoMConstants.h"
#include "CoMPlayerState.generated.h"

/**
 * ACoMPlayerState
 *
 * Holds all per-wizard data that must survive between turns and be readable by
 * other systems (GameState, AI, UI, diplomacy).  This is live runtime state
 * replicated across clients — not a save-game record.
 *
 * Spellbook allocation is fixed at game creation (SpellbookAllocation).
 * ResearchedSpells grows as research completes.  ActiveOverworldEnchantments
 * tracks maintained overworld spells that cost mana each turn.
 *
 * Retorts are stored as a FGameplayTagContainer using tags from the
 * CoMTags::Retort namespace (e.g. CoMTags::Retort::Archmage).
 */
UCLASS()
class COMCORE_API ACoMPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACoMPlayerState();

	// ─── Identity ─────────────────────────────────────────────────────────────

	/** Wizard display name chosen at game creation (distinct from the UE player name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Identity")
	FString WizardName;

	/**
	 * Stable roster index in [0, MAX_WIZARDS).
	 * Used for O(1) lookups in ACoMGameState::WizardStates and DiplomacyMatrix.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Identity")
	int32 WizardIndex = CoM::WIZARD_INDEX_NONE;

	/** Home race — determines starting city units and race-specific bonuses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Identity")
	ECoMRace WizardRace = ECoMRace::Human;

	/** Banner colour shown on units, cities, and the diplomacy screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Identity")
	FLinearColor WizardColor = FLinearColor::White;

	/**
	 * Moral alignment — shifts as Life/Death/Chaos spells are cast and as
	 * diplomatic/military actions accumulate.  Affects hero recruitment and
	 * certain spell interactions.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Identity")
	ECoMAlignment WizardAlignment = ECoMAlignment::Neutral;

	/** Starting plane.  Aurelith by default; Noctharion if the Myrran retort is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Identity")
	ECoMPlane HomePlane = ECoMPlane::Aurelith;

	/** True if this wizard slot is controlled by a human player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Identity")
	bool bIsHumanPlayer = false;

	/** True once this wizard has been defeated and eliminated from the game. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Identity")
	bool bIsEliminated = false;

	// ─── Economy ──────────────────────────────────────────────────────────────

	/** Gold coins held.  Clamped to >= 0; unit upkeep fails if the pool is empty. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Economy")
	int32 Gold = 0;

	/** Net gold income per turn (city production minus upkeep costs). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Economy")
	int32 GoldIncome = 0;

	// ─── Mana & Power ─────────────────────────────────────────────────────────

	/** Current mana reserve. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	int32 Mana = 0;

	/** Gross mana generated per turn (nodes + books) before enchantment upkeep. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	int32 ManaIncome = 0;

	/**
	 * Mana allocated to the casting pool each turn — the fraction of total power
	 * the wizard commits to immediate spellcasting (vs. research or mana reserve).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Magic")
	int32 CastingSkill = 0;

	/** Fame/notoriety score.  Raised by victories, spells, and world events. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	int32 Fame = 0;

	// ─── Research ─────────────────────────────────────────────────────────────

	/** Research points accumulated toward the current spell. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Research")
	int32 ResearchPoints = 0;

	/** Research points generated per turn (allocated portion of total power). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Research")
	int32 ResearchIncome = 0;

	/** DataAsset row name of the spell being researched.  NAME_None when idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Research")
	FName CurrentResearchSpellID = NAME_None;

	// ─── Spellbook ────────────────────────────────────────────────────────────

	/**
	 * Books allocated per magic realm, chosen once at game creation.
	 * Indexed by (int32)ECoMSpellRealm (Life=0 … Spirit=7; None=8 is not a valid index).
	 * Array length == CoM::NUM_SPELL_REALMS; total across all entries <= CoM::MAX_SPELL_BOOKS.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Spellbook")
	TArray<int32> SpellbookAllocation;

	/** Row names of all spells fully researched and available to cast. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Spellbook")
	TArray<FName> ResearchedSpells;

	// ─── Active Spells ────────────────────────────────────────────────────────

	/**
	 * Row names of overworld enchantments this wizard is currently maintaining.
	 * Each costs ManaIncome per turn; cancelling a spell removes it from this list.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Magic")
	TArray<FName> ActiveOverworldEnchantments;

	// ─── Retorts ──────────────────────────────────────────────────────────────

	/**
	 * Wizard perks selected at game start (up to CoM::MAX_RETORTS entries).
	 * Stored as a GameplayTagContainer for efficient HasTag queries at runtime.
	 * Tags live in the CoMTags::Retort namespace.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Retorts")
	FGameplayTagContainer Retorts;

	// ─── Victory ──────────────────────────────────────────────────────────────

	/** Aggregate victory-condition progress (interpretation depends on active win type). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Victory")
	int32 VictoryPoints = 0;

	/** True once this wizard has satisfied their primary win condition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wizard|Victory")
	bool bHasActivatedWinCondition = false;

	// ─── Accessors ────────────────────────────────────────────────────────────

	/** Returns true if the wizard has the given retort tag active. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Retorts")
	bool HasRetort(FGameplayTag RetortTag) const;

	/**
	 * Returns books allocated in the given realm.
	 * Returns 0 for ECoMSpellRealm::None / MAX (out-of-bounds indices).
	 */
	UFUNCTION(BlueprintPure, Category = "Wizard|Spellbook")
	int32 GetSpellbookCountForRealm(ECoMSpellRealm Realm) const;

	/** Returns true if the named spell has been fully researched. */
	UFUNCTION(BlueprintPure, Category = "Wizard|Spellbook")
	bool HasResearchedSpell(FName SpellID) const;

	/**
	 * Adjust the gold pool.  Positive = earn; negative = spend.
	 * Clamped to >= 0.  Returns the new balance.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Economy")
	int32 ModifyGold(int32 Delta);

	/**
	 * Adjust the mana pool.  Positive = gain; negative = spend.
	 * Clamped to >= 0.  Returns the new balance.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wizard|Magic")
	int32 ModifyMana(int32 Delta);

protected:
	virtual void BeginPlay() override;

private:
	/** Zero-initialise SpellbookAllocation to NUM_SPELL_REALMS entries on construction. */
	void InitSpellbookAllocation();
};
