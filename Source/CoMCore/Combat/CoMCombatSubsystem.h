// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "CoMCore/Framework/CoMGameInstance.h"
#include "CoMCombatSubsystem.generated.h"

// ═══════════════════════════════════════════════════════════════════════════
// Combat Result — output of a resolved battle
// ═══════════════════════════════════════════════════════════════════════════

USTRUCT(BlueprintType)
struct COMCORE_API FCoMCombatResult
{
	GENERATED_BODY()

	/** Wizard ID of the winning side (INDEX_NONE if draw). */
	UPROPERTY(BlueprintReadOnly) int32 WinnerWizardID = INDEX_NONE;

	/** Unit IDs killed on the attacker's side. */
	UPROPERTY(BlueprintReadOnly) TArray<int32> AttackerCasualties;

	/** Unit IDs killed on the defender's side. */
	UPROPERTY(BlueprintReadOnly) TArray<int32> DefenderCasualties;

	/** XP gained per surviving unit (keyed by unit ID). */
	UPROPERTY(BlueprintReadOnly) TMap<int32, int32> XPGained;

	/** Number of combat rounds fought. */
	UPROPERTY(BlueprintReadOnly) int32 CombatRounds = 0;

	/** True if the battle was auto-resolved (no tactical UI). */
	UPROPERTY(BlueprintReadOnly) bool bAutoResolved = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// Combat Modifiers — terrain, weather, hero, enchantment bonuses
// ═══════════════════════════════════════════════════════════════════════════

USTRUCT(BlueprintType)
struct COMCORE_API FCoMCombatModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 TerrainBonus    = FFixed64(0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 WeatherBonus    = FFixed64(0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 HeroCommandBonus = FFixed64(0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FFixed64 EnchantmentBonus = FFixed64(0);
};

// ═══════════════════════════════════════════════════════════════════════════
// Delegate
// ═══════════════════════════════════════════════════════════════════════════

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatResolved, const FCoMCombatResult&, Result);

/** Fired when a player battle is detected and needs a pre-battle choice (auto-resolve vs tactical). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreBattleChoice, int32, AttackerArmyID, int32, DefenderArmyID);

// ═══════════════════════════════════════════════════════════════════════════
// Internal: snapshot of a unit for combat simulation
// ═══════════════════════════════════════════════════════════════════════════

USTRUCT()
struct FCoMCombatUnitState
{
	GENERATED_BODY()

	int32 UnitID         = INDEX_NONE;
	int32 OwnerWizardID  = INDEX_NONE;

	// Combat stats
	FFixed64 MeleeAttack  = FFixed64(3);
	FFixed64 RangedAttack = FFixed64(0);
	FFixed64 Defense      = FFixed64(3);
	int32    HitPoints    = 1;
	int32    MaxHP        = 1;
	int32    FiguresRemaining = 1;  // multi-figure units (e.g. 8 swordsmen)
	bool     bIsRanged    = false;
	bool     bIsHero      = false;
	ECoMHeroTier HeroTier = ECoMHeroTier::Adventurer;

	/** Skill IDs granted by equipped items (flame_blade, vampiric, etc.). */
	TArray<FName> ItemSkills;
};

/**
 * Resolves auto-combat between armies using a round-based system
 * inspired by Master of Magic / Caster of Magic.
 *
 * Each combat runs in rounds (max 50 before draw). Per round every unit
 * rolls attack dice, defender rolls blocks, unblocked hits deal 1 HP each.
 * After attacker units attack, defender units counterattack the same way.
 * Combat ends when one side has no surviving units.
 */
UCLASS()
class COMCORE_API UCoMCombatSubsystem : public UGameInstanceSubsystem
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
	// Public API
	// -----------------------------------------------------------------

	/** Resolve auto-combat between two armies. Returns the full result. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	FCoMCombatResult ResolveAutoCombat(int32 AttackerArmyID, int32 DefenderArmyID, int32 CurrentTurn);

	/** Detect pairs of hostile armies sharing the same tile. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	TArray<FIntPoint> DetectEncounters() const;

	/** Resolve all detected encounters for this turn.
	 *  AI-vs-AI battles auto-resolve. Player-involved battles populate
	 *  FCoMCombatContext and trigger a level transition to the tactical map. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void ResolveAllEncounters(int32 CurrentTurn);

	/** Set the human player's wizard index (used to decide tactical vs auto-resolve). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void SetHumanPlayerWizardIndex(int32 WizardIndex) { HumanPlayerWizardIndex = WizardIndex; }

	/** Name of the tactical combat map level to open for player battles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Combat")
	FName TacticalCombatMapName = FName(TEXT("TacticalCombatMap"));

	/** Calculate total combat power of an army. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Combat")
	FFixed64 CalculateArmyPower(int32 ArmyID) const;

	/** Quick-resolve a battle using army power comparison + random roll (no tactical map). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	FCoMCombatResult QuickResolveBattle(int32 AttackerArmyId, int32 DefenderArmyId);

	/** Player's preference for auto-resolve vs tactical combat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Combat")
	bool bPlayerPrefersAutoResolve = false;

	/** Called by UI after the player picks auto-resolve or fight on the pre-battle popup. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Combat")
	void ResolvePendingBattle(bool bAutoResolve);

	/** Static terrain defense modifier lookup. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Combat")
	static FFixed64 GetTerrainModifier(ECoMTerrain TerrainType);

	/** Weather modifier affecting ranged attacks. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Combat")
	static FFixed64 GetWeatherModifier(ECoMWeatherType WeatherType);

	/** Hero command bonus for an army (based on hero tier/class). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Combat")
	FFixed64 GetHeroCommandBonus(int32 ArmyID) const;

	// -----------------------------------------------------------------
	// Delegate
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Combat")
	FOnCombatResolved OnCombatResolved;

	/** Fired when a player-involved battle needs a pre-battle choice. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Combat")
	FOnPreBattleChoice OnPreBattleChoice;

	static FFixed64 GetHeroTierAttackBonus(ECoMHeroTier Tier);
private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Build combat-unit snapshots from an army's unit roster. */
	TArray<FCoMCombatUnitState> BuildCombatUnits(int32 ArmyID) const;

	/** Run one round of attacks: each unit in Side attacks units in OtherSide. */
	void ExecuteAttackRound(
		TArray<FCoMCombatUnitState>& Attackers,
		TArray<FCoMCombatUnitState>& Defenders,
		FFixed64 AttackHeroBonus,
		FFixed64 DefenseTerrainMod,
		FFixed64 WeatherRangedMod);

	/** Remove dead units (HP <= 0) and return their IDs. */
	TArray<int32> RemoveDeadUnits(TArray<FCoMCombatUnitState>& Units);

	/** Get hero tier bonus as a multiplier (Adventurer +5%, Hero +10%, etc.). */

	/** Populate CombatContext and open the tactical combat level. */
	void StartTacticalBattle(int32 AttackerArmyID, int32 DefenderArmyID,
	                         int32 AttackerWizard, int32 DefenderWizard);

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	FRandomStream RngStream;

	/** Wizard index of the human player (-1 = not set / all AI). */
	int32 HumanPlayerWizardIndex = -1;

	/** Pending battle army IDs (set when waiting for player pre-battle choice). */
	int32 PendingAttackerArmyID = INDEX_NONE;
	int32 PendingDefenderArmyID = INDEX_NONE;
	int32 PendingAttackerWizard = INDEX_NONE;
	int32 PendingDefenderWizard = INDEX_NONE;
};
