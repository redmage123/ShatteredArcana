// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMSeasonSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSeasonChanged, ECoMPlane, Plane, ECoMSeason, OldSeason, ECoMSeason, NewSeason);

/**
 * Economy modifiers applied by the current season on a given plane.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMSeasonEconomyModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FFixed64 FoodMultiplier = FFixed64(1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FFixed64 TradeMultiplier = FFixed64(1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FFixed64 ProductionMultiplier = FFixed64(1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bBuildingAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ECoMSpellRealm, FFixed64> MagicSchoolBonuses;
};

/**
 * Per-plane configuration for the seasonal cycle. Hardcoded defaults;
 * intended to be overridable via DataAsset in a future pass.
 */
USTRUCT(BlueprintType)
struct COMCORE_API FCoMPlaneSeasonConfig
{
	GENERATED_BODY()

	/** Total turns in a full year for this plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 TurnsPerYear = 48;

	/** Economy modifiers indexed by season ordinal (0 = Spring/first season). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ECoMSeason, FCoMSeasonEconomyModifiers> SeasonModifiers;
};

/**
 * Manages independent seasonal cycles for each of the 8 planes.
 *
 * Each plane has its own turn length per year, season names, and economy /
 * magic modifiers. Call AdvanceTurn() once per global turn to tick all
 * planes forward.
 */
UCLASS()
class COMCORE_API UCoMSeasonSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------

	/** Reset all planes to Spring (first season), turn 0. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Seasons")
	void InitializeSeasons();

	/** Advance every plane by one turn. Fires OnSeasonChanged when a transition occurs. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Seasons")
	void AdvanceTurn();

	/** Current season on the given plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Seasons")
	ECoMSeason GetCurrentSeason(ECoMPlane Plane) const;

	/** How many turns have elapsed within the current season on this plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Seasons")
	int32 GetTurnsIntoSeason(ECoMPlane Plane) const;

	/** Turns remaining before the next season begins on this plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Seasons")
	int32 GetTurnsUntilNextSeason(ECoMPlane Plane) const;

	/** Economy / magic modifiers for the current season on this plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Seasons")
	FCoMSeasonEconomyModifiers GetEconomyModifiers(ECoMPlane Plane) const;

	/** Broadcast when any plane transitions to a new season. */
	UPROPERTY(BlueprintAssignable, Category = "CoM|Seasons")
	FOnSeasonChanged OnSeasonChanged;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Build the hardcoded per-plane season configs. */
	void BuildDefaultConfigs();

	/** Return the number of turns per season for a given plane. */
	int32 GetTurnsPerSeason(ECoMPlane Plane) const;

	/** Compute the next season in the cycle. */
	static ECoMSeason GetNextSeason(ECoMSeason Current);

	/** Return a default (neutral) modifier set. */
	static FCoMSeasonEconomyModifiers MakeDefaultModifiers();

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	/** Seasonal state per plane (current season + turn counter). */
	UPROPERTY()
	TMap<ECoMPlane, FCoMPlaneSeasonConfig> PlaneSeasons;

	/** Per-plane configuration (turns per year, modifier tables). */
	TMap<ECoMPlane, FCoMPlaneSeasonConfig> PlaneConfigs;
};

// === CoMSeasonSubsystem.cpp ===
