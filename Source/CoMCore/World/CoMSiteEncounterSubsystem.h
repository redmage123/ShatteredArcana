// Copyright Mythforge Studios. All Rights Reserved.
// CoMSiteEncounterSubsystem.h -- Detects army arrivals at sites and guarded
// mana nodes; autoresolves the encounter; pays rewards; marks the site cleared.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMSiteEncounterSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSiteCleared,
	int32, SiteID, int32, WizardIndex, int32, GoldReward, int32, ManaReward);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeGuardDefeated,
	FIntPoint, NodePosition, int32, WizardIndex);

/**
 * UCoMSiteEncounterSubsystem
 *
 * Per-turn:
 *   For every army:
 *     If the tile has an uncleared SiteID -> roll combat army vs. GuardPower.
 *       Win: pay gold/mana/spell, mark cleared, broadcast OnSiteCleared.
 *       Lose: damage every unit in the army (HP -= 1 per figure of guard power / 30).
 *
 *     If the tile has a guarded mana node -> same flow against a stock guard.
 *       Win: clear the node guard so it can be melded later.
 *
 * Encounter strength is a stat-based comparison (no tactical map). For the
 * playable-loop milestone this keeps things deterministic and short.
 */
UCLASS()
class COMCORE_API UCoMSiteEncounterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Walk every army; resolve any site/node encounters at the army's current tile.
	 * Called from UCoMUnitSubsystem::ProcessMovementTurn after the inter-wizard
	 * encounter pass.
	 */
	UFUNCTION(BlueprintCallable, Category = "World|Sites")
	void ProcessArmyArrivals();

	/** Manually trigger an encounter for a specific army at its current tile. */
	UFUNCTION(BlueprintCallable, Category = "World|Sites")
	bool TryResolveEncounterForArmy(int32 ArmyID);

	UPROPERTY(BlueprintAssignable, Category = "World|Sites") FOnSiteCleared        OnSiteCleared;
	UPROPERTY(BlueprintAssignable, Category = "World|Sites") FOnNodeGuardDefeated  OnNodeGuardDefeated;

private:
	/** Returns total combat power of an army (sum of CurrentHP × Level, hero ×2). */
	float ComputeArmyPower(int32 ArmyID) const;

	/** Returns true if the army wins. Damages the loser as a side-effect. */
	bool ResolveAutoCombat(int32 ArmyID, int32 GuardPower);
};
