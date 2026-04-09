// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMWorldEventSubsystem.h — World events subsystem: triggers, resolves, and expires
// global events that reshape gameplay across planes.
// COM-040

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMCore/Turn/CoMDeterministicRandom.h"
#include "CoMWorldEventSubsystem.generated.h"

// Forward declarations — resolved at call time via GetGameInstance()->GetSubsystem<>()
class UCoMWorldMapSubsystem;
class UCoMWeatherSubsystem;
class UCoMUnitSubsystem;
class UCoMResourceSubsystem;
class UCoMCitySubsystem;
class UCoMAudioSubsystem;
class UCoMHeroSubsystem;
class UCoMDragonSubsystem;
class UCoMMagicSubsystem;

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventTriggered, const FCoMWorldEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEventExpired, const FCoMWorldEvent&, Event);

// ─────────────────────────────────────────────────────────────────────────────

/**
 * UCoMWorldEventSubsystem
 *
 * GameInstance subsystem that manages the lifecycle of world events:
 * random celestial phenomena, planar disturbances, dragon activity,
 * plagues, chaos storms, undead risings, and social upheavals.
 *
 * Each turn:
 *   1. Expire events whose duration has elapsed
 *   2. Roll for new events (probability scales with game age)
 *   3. Apply ongoing effects of active events
 *
 * Event selection is weighted by game phase (early/mid/late) and by
 * plane type (e.g., UndeathRising favours Noctharion, ChaosStorm
 * only fires on Abyssal).
 *
 * All randomness goes through FCoMDeterministicRandom for replay
 * and network determinism.
 */
UCLASS()
class COMCORE_API UCoMWorldEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ── USubsystem interface ──────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Turn Processing ──────────────────────────────────────────────────

	/**
	 * Main per-turn entry point. Called by UCoMTurnSubsystem::ProcessAllSubsystemTurns()
	 * after weather but before combat.
	 *
	 * Steps: expire -> roll new -> apply ongoing effects.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|WorldEvents")
	void ProcessEventTurn(int32 CurrentTurn);

	// ── Event Lifecycle ──────────────────────────────────────────────────

	/**
	 * Trigger a specific event type on a plane at a given epicenter.
	 * Returns the assigned EventID.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|WorldEvents")
	int32 TriggerEvent(ECoMWorldEventType Type, ECoMPlane Plane, FIntPoint Epicenter);

	/** Apply immediate effects when an event fires. Called internally by TriggerEvent. */
	void ResolveEvent(int32 EventID);

	/** Remove an expired event and undo any reversible effects. */
	void ExpireEvent(int32 EventID);

	/** Debug/console command: force-trigger an event on Aurelith at (0,0). */
	UFUNCTION(BlueprintCallable, Category = "CoM|WorldEvents")
	void ForceEvent(ECoMWorldEventType Type);

	// ── Queries ──────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "CoM|WorldEvents")
	TArray<FCoMWorldEvent> GetActiveEvents() const;

	UFUNCTION(BlueprintPure, Category = "CoM|WorldEvents")
	TArray<FCoMWorldEvent> GetEventsOnPlane(ECoMPlane Plane) const;

	UFUNCTION(BlueprintPure, Category = "CoM|WorldEvents")
	bool IsEventActive(ECoMWorldEventType Type) const;

	/** Get an event by ID, or nullptr if not found. */
	const FCoMWorldEvent* GetEventByID(int32 EventID) const;

	// ── Delegates ────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "CoM|WorldEvents")
	FOnWorldEventTriggered OnWorldEventTriggered;

	UPROPERTY(BlueprintAssignable, Category = "CoM|WorldEvents")
	FOnWorldEventExpired OnWorldEventExpired;

	// ── Save/Load Export/Import ──────────────────────────────────────────

	/** Export all event state for save serialization. */
	void ExportAll(TArray<FCoMWorldEvent>& OutActiveEvents,
	               TArray<FCoMWorldEvent>& OutEventHistory,
	               int32& OutNextEventID,
	               FCoMDeterministicRandom& OutEventRng) const;

	/** Import event state from save data (clears existing state). */
	void ImportAll(const TArray<FCoMWorldEvent>& InActiveEvents,
	               const TArray<FCoMWorldEvent>& InEventHistory,
	               int32 InNextEventID,
	               const FCoMDeterministicRandom& InEventRng);

private:
	// ── State ────────────────────────────────────────────────────────────

	/** Currently active world events. */
	UPROPERTY()
	TArray<FCoMWorldEvent> ActiveEvents;

	/** Completed events kept for scoring, narrative, and event-chain logic. */
	UPROPERTY()
	TArray<FCoMWorldEvent> EventHistory;

	/** Monotonically increasing event ID counter. */
	int32 NextEventID = 1;

	/** Deterministic PRNG for event rolls. Seed 0x4576656E = ASCII "Even". Sequence 3 = world events stream. */
	FCoMDeterministicRandom EventRng;

	// ── Internal helpers ─────────────────────────────────────────────────

	/** Expire events whose duration has elapsed. */
	void ExpireFinishedEvents(int32 CurrentTurn);

	/** Roll for new random events on each discovered plane. */
	void RollForNewEvents(int32 CurrentTurn);

	/** Apply per-turn ongoing effects of all active events. */
	void ApplyOngoingEffects(int32 CurrentTurn);

	/** Select an event type weighted by game phase and plane. */
	ECoMWorldEventType SelectEventTypeForPlane(ECoMPlane Plane, int32 CurrentTurn);

	/** Get the default duration (in turns) for an event type. */
	static int32 GetDefaultDuration(ECoMWorldEventType Type);

	/** Get a random position on the map for the given plane. */
	FIntPoint GetRandomPosition();

	// ── Effect application per category ──────────────────────────────────

	void ResolveCelestialEvent(const FCoMWorldEvent& Event);
	void ResolvePlanarEvent(const FCoMWorldEvent& Event);
	void ResolveDragonEvent(const FCoMWorldEvent& Event);
	void ResolveNatureEvent(const FCoMWorldEvent& Event);
	void ResolveChaosEvent(const FCoMWorldEvent& Event);
	void ResolveDeathEvent(const FCoMWorldEvent& Event);
	void ResolveSocialEvent(const FCoMWorldEvent& Event);

	/** Apply per-turn effects for a single active event. */
	void ApplyOngoingEffect(const FCoMWorldEvent& Event, int32 CurrentTurn);

	// ── Map constants ────────────────────────────────────────────────────

	static constexpr int32 MAP_WIDTH  = 160;
	static constexpr int32 MAP_HEIGHT = 100;
};
