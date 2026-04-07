// Copyright Shattered Arcana. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMDragonSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDragonDomainExpanded, int32, DomainID, int32, NewRadius);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDragonEggHatched, int32, EggID, int32, DragonID);

/**
 * Manages dragon instances, territorial domains, and egg incubation/hatching.
 * Split from UCoMHeroSubsystem for single responsibility.
 */
UCLASS()
class COMCORE_API UCoMDragonSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -----------------------------------------------------------------
	// Dragons
	// -----------------------------------------------------------------

	/** Spawn a dragon instance and return its DragonID (-1 on failure). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Dragons")
	int32 SpawnDragon(int32 TypeID, ECoMPlane Plane, FIntPoint LairPosition, UPARAM(ref) FRandomStream& Rng);

	/** Establish a territorial domain around an existing dragon. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Dragons")
	void CreateDragonDomain(int32 DragonID, int32 InfluenceRadius);

	/** Return dragon instance by ID, or nullptr. */
	const FCoMDragonInstance* GetDragon(int32 DragonID) const;

	/** Return domain by ID, or nullptr. */
	const FCoMDragonDomain* GetDomain(int32 DomainID) const;

	/** All domains on the specified plane. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CoM|Dragons")
	TArray<const FCoMDragonDomain*> GetDomainsOnPlane(ECoMPlane Plane) const;

	// -----------------------------------------------------------------
	// Dragon Eggs
	// -----------------------------------------------------------------

	/** Create a dragon egg laid by the given parent. Returns EggID. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Dragons")
	int32 LayDragonEgg(int32 ParentDragonID, int32 PossessorWizard);

	/** Attempt to hatch an egg. Returns the new DragonID, or -1 if not ready. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Dragons")
	int32 HatchEgg(int32 EggID);

	// -----------------------------------------------------------------
	// Dragon Turn Processing
	// -----------------------------------------------------------------

	/** Per-turn: age dragons, expand domains, tick egg incubation. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Dragons")
	void ProcessDragonTurn();

	// -----------------------------------------------------------------
	// Delegates
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "CoM|Dragons")
	FOnDragonDomainExpanded OnDragonDomainExpanded;

	UPROPERTY(BlueprintAssignable, Category = "CoM|Dragons")
	FOnDragonEggHatched OnDragonEggHatched;

private:
	// -----------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------

	/** Generate claimed tile set for a domain from lair position + radius. */
	static TArray<FIntPoint> ComputeClaimedTiles(FIntPoint Center, int32 Radius);

	/** Allocate the next sequential ID for a given counter. */
	int32 AllocateDragonID();
	int32 AllocateDomainID();
	int32 AllocateEggID();

	// -----------------------------------------------------------------
	// State
	// -----------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMDragonInstance> Dragons;

	UPROPERTY()
	TMap<int32, FCoMDragonDomain> Domains;

	UPROPERTY()
	TArray<FCoMDragonEgg> Eggs;

	int32 NextDragonID  = 1;
	int32 NextDomainID  = 1;
	int32 NextEggID     = 1;

	FRandomStream RngStream;
};
