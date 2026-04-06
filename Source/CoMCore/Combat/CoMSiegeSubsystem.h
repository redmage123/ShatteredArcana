// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMTypes.h"
#include "FixedPoint/Fixed64.h"
#include "CoMSiegeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSiegeStarted, int32, SiegeID, int32, DefenderCityID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSiegeEnded, int32, SiegeID, bool, bAttackerVictory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSiegePhaseChanged, int32, SiegeID, ECoMSiegePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallBreached, int32, SiegeID, int32, BreachSegment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeSurrender, int32, SiegeID);

class UCoMCitySubsystem;
class UCoMUnitSubsystem;
class UCoMWeatherSubsystem;

/**
 * Multi-turn siege system with wall HP, breach points, siege equipment,
 * starvation mechanics, and surrender logic.
 */
UCLASS()
class COMCORE_API UCoMSiegeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------

	/** Begin a siege. Returns the new SiegeID (-1 on failure). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	int32 BeginSiege(int32 AttackerArmyID, int32 DefenderCityID);

	/** End a siege explicitly (attacker victory or retreat). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	void EndSiege(int32 SiegeID, bool bAttackerVictory);

	/** Advance all active sieges by one turn: bombardment, starvation, morale, surrender. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	void ProcessSiegeTurn();

	/** Return siege state by ID, or nullptr. */
	const FCoMSiegeState* GetSiege(int32 SiegeID) const;

	/** Return all active siege states. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Siege")
	TArray<const FCoMSiegeState*> GetActiveSieges() const;

	/** True if the given city is currently under siege. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Siege")
	bool IsCityUnderSiege(int32 CityID) const;

	/** Deploy a siege equipment unit to an active siege. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	void DeploySiegeEquipment(int32 SiegeID, int32 EquipmentUnitID);

	/** Resolve a direct assault attempt against the city. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	void AttemptAssault(int32 SiegeID);

	/** Offer surrender to defender. Returns true if defender accepts. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	bool OfferSurrender(int32 SiegeID);

	/** Cut supply lines to accelerate starvation. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Siege")
	void CutSupplyLines(int32 SiegeID);

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Siege")
	FOnSiegeStarted OnSiegeStarted;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Siege")
	FOnSiegeEnded OnSiegeEnded;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Siege")
	FOnSiegePhaseChanged OnSiegePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Siege")
	FOnWallBreached OnWallBreached;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Siege")
	FOnSiegeSurrender OnSiegeSurrender;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Apply bombardment damage from all siege equipment in this siege. */
	void ProcessBombardment(FCoMSiegeState& Siege);

	/** Decrement food reserves and apply morale loss from starvation. */
	void ProcessStarvation(FCoMSiegeState& Siege);

	/** Check if the phase should transition based on current state. */
	void UpdatePhase(FCoMSiegeState& Siege);

	/** Query weather to get bombardment slowdown factor (1.0 = normal). */
	FFixed64 GetWeatherBombardmentFactor(int32 CityID) const;

	/** Query weather to check if assault is blocked (e.g. thunderstorm). */
	bool IsAssaultBlockedByWeather(int32 CityID) const;

	/** Check if any siege equipment is a siege tower. */
	bool HasSiegeTowers(const FCoMSiegeState& Siege) const;

	/** Initialize wall and gatehouse HP from city wall level. */
	void InitializeWallHP(FCoMSiegeState& Siege, int32 CityID);

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMSiegeState> AllSieges;

	int32 NextSiegeID = 1;
};
