// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMWorldEventSubsystem.cpp — World events lifecycle: trigger, resolve, expire.
// COM-040

#include "Events/CoMWorldEventSubsystem.h"
#include "World/CoMWorldMapSubsystem.h"
#include "World/CoMWeatherSubsystem.h"
#include "Units/CoMUnitSubsystem.h"
#include "Units/CoMHeroSubsystem.h"
#include "Units/CoMDragonSubsystem.h"
#include "Economy/CoMResourceSubsystem.h"
#include "Economy/CoMCitySubsystem.h"
#include "Magic/CoMMagicSubsystem.h"
#include "Audio/CoMAudioSubsystem.h"
#include "Turn/CoMTurnSubsystem.h"
#include "Framework/CoMGameState.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ActiveEvents.Empty();
	EventHistory.Empty();
	NextEventID = 1;

	// Seed 0x4576656E = ASCII "Even", Sequence 3 = world events stream
	EventRng = FCoMDeterministicRandom(0x4576656EULL, 3);
}

void UCoMWorldEventSubsystem::Deinitialize()
{
	ActiveEvents.Empty();
	EventHistory.Empty();
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Turn Processing
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ProcessEventTurn(int32 CurrentTurn)
{
	UE_LOG(LogTemp, Log, TEXT("[WorldEvents] Processing event turn %d. Active events: %d"),
	       CurrentTurn, ActiveEvents.Num());

	// 1. Expire events whose duration has elapsed
	ExpireFinishedEvents(CurrentTurn);

	// 2. Roll for new events on each discovered plane
	RollForNewEvents(CurrentTurn);

	// 3. Apply ongoing effects of active events
	ApplyOngoingEffects(CurrentTurn);
}

// ─────────────────────────────────────────────────────────────────────────────
// Event Lifecycle — public
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMWorldEventSubsystem::TriggerEvent(ECoMWorldEventType Type, ECoMPlane Plane, FIntPoint Epicenter)
{
	FCoMWorldEvent NewEvent;
	NewEvent.EventID         = NextEventID++;
	NewEvent.Type            = Type;
	// Query current turn so events triggered via public API get correct expiry timing
	int32 CurTurn = 0;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (auto* TS = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			CurTurn = TS->GetCurrentTurn();
		}
	}
	NewEvent.TurnTriggered   = CurTurn;
	NewEvent.Duration        = GetDefaultDuration(Type);
	NewEvent.AffectedPlanes.Add(Plane);
	NewEvent.EpicenterPosition = Epicenter;
	NewEvent.Intensity       = FFixed64(1);

	// For global events, radius 0 means affects entire plane
	NewEvent.EpicenterRadius = 0;

	ActiveEvents.Add(NewEvent);

	UE_LOG(LogTemp, Log, TEXT("[WorldEvents] Triggered event %d: Type=%d on Plane=%d at (%d,%d), Duration=%d"),
	       NewEvent.EventID, static_cast<int32>(Type), static_cast<int32>(Plane),
	       Epicenter.X, Epicenter.Y, NewEvent.Duration);

	// Resolve immediate effects
	ResolveEvent(NewEvent.EventID);

	// Broadcast delegate
	OnWorldEventTriggered.Broadcast(NewEvent);

	// Audio notification
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->PlayUISound(FName("SFX_UI_WorldEvent"));
		}
	}

	return NewEvent.EventID;
}

void UCoMWorldEventSubsystem::ResolveEvent(int32 EventID)
{
	const FCoMWorldEvent* Event = GetEventByID(EventID);
	if (!Event)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldEvents] ResolveEvent: EventID %d not found."), EventID);
		return;
	}

	switch (Event->Type)
	{
		// Celestial
		case ECoMWorldEventType::SolarEclipse:
		case ECoMWorldEventType::LunarAlignment:
		case ECoMWorldEventType::CometAppearance:
		case ECoMWorldEventType::StarFall:
			ResolveCelestialEvent(*Event);
			break;

		// Planar
		case ECoMWorldEventType::PlanarConvergence:
		case ECoMWorldEventType::PlanarBleeding:
		case ECoMWorldEventType::PlanarSealWeakening:
		case ECoMWorldEventType::AetherTide:
			ResolvePlanarEvent(*Event);
			break;

		// Dragon
		case ECoMWorldEventType::DragonMatingFlight:
		case ECoMWorldEventType::DragonConvocation:
		case ECoMWorldEventType::DragonAwakening:
			ResolveDragonEvent(*Event);
			break;

		// Nature
		case ECoMWorldEventType::GlobalBloom:
		case ECoMWorldEventType::GreatDrought:
		case ECoMWorldEventType::Plague:
		case ECoMWorldEventType::MassExtinction:
			ResolveNatureEvent(*Event);
			break;

		// Chaos
		case ECoMWorldEventType::MagicStorm:
		case ECoMWorldEventType::VolcanicAge:
		case ECoMWorldEventType::DimensionalRift:
		case ECoMWorldEventType::RealityFracture:
		case ECoMWorldEventType::ChaosStorm:
			ResolveChaosEvent(*Event);
			break;

		// Death
		case ECoMWorldEventType::UndeathRising:
		case ECoMWorldEventType::SoulHarvest:
		case ECoMWorldEventType::AncientLichAwakens:
			ResolveDeathEvent(*Event);
			break;

		// Social
		case ECoMWorldEventType::OrganizationWar:
		case ECoMWorldEventType::MerchantCrisis:
		case ECoMWorldEventType::HeroicAge:
		case ECoMWorldEventType::ProphecyFulfilled:
			ResolveSocialEvent(*Event);
			break;

		// Underdark
		case ECoMWorldEventType::GreatUpheaval:
		case ECoMWorldEventType::DeepQuake:
		case ECoMWorldEventType::UnderdarkFlood:
			// Underdark events use nature-like resolution
			ResolveNatureEvent(*Event);
			break;

		default:
			UE_LOG(LogTemp, Warning, TEXT("[WorldEvents] ResolveEvent: Unhandled event type %d"),
			       static_cast<int32>(Event->Type));
			break;
	}
}

void UCoMWorldEventSubsystem::ExpireEvent(int32 EventID)
{
	const int32 Idx = ActiveEvents.IndexOfByPredicate(
		[EventID](const FCoMWorldEvent& E) { return E.EventID == EventID; });

	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WorldEvents] ExpireEvent: EventID %d not found in active list."), EventID);
		return;
	}

	FCoMWorldEvent Expired = ActiveEvents[Idx];
	ActiveEvents.RemoveAt(Idx);
	EventHistory.Add(Expired);

	UE_LOG(LogTemp, Log, TEXT("[WorldEvents] Expired event %d (Type=%d). Moved to history. Active: %d, History: %d"),
	       EventID, static_cast<int32>(Expired.Type), ActiveEvents.Num(), EventHistory.Num());

	OnWorldEventExpired.Broadcast(Expired);
}

void UCoMWorldEventSubsystem::ForceEvent(ECoMWorldEventType Type)
{
	UE_LOG(LogTemp, Log, TEXT("[WorldEvents] ForceEvent: forcing type %d on Aurelith at (0,0)."),
	       static_cast<int32>(Type));
	TriggerEvent(Type, ECoMPlane::Aurelith, FIntPoint(0, 0));
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

TArray<FCoMWorldEvent> UCoMWorldEventSubsystem::GetActiveEvents() const
{
	return ActiveEvents;
}

TArray<FCoMWorldEvent> UCoMWorldEventSubsystem::GetEventsOnPlane(ECoMPlane Plane) const
{
	TArray<FCoMWorldEvent> Result;
	for (const FCoMWorldEvent& Event : ActiveEvents)
	{
		if (Event.AffectedPlanes.Contains(Plane))
		{
			Result.Add(Event);
		}
	}
	return Result;
}

bool UCoMWorldEventSubsystem::IsEventActive(ECoMWorldEventType Type) const
{
	for (const FCoMWorldEvent& Event : ActiveEvents)
	{
		if (Event.Type == Type)
		{
			return true;
		}
	}
	return false;
}

const FCoMWorldEvent* UCoMWorldEventSubsystem::GetEventByID(int32 EventID) const
{
	for (const FCoMWorldEvent& Event : ActiveEvents)
	{
		if (Event.EventID == EventID)
		{
			return &Event;
		}
	}
	return nullptr;
}

const TArray<FCoMWorldEvent>& UCoMWorldEventSubsystem::GetEventHistory() const
{
	return EventHistory;
}

int32 UCoMWorldEventSubsystem::GetNextEventID() const
{
	return NextEventID;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ExportAll(TArray<FCoMWorldEvent>& OutActiveEvents,
                                         TArray<FCoMWorldEvent>& OutEventHistory,
                                         int32& OutNextEventID,
                                         FCoMDeterministicRandom& OutEventRng) const
{
	OutActiveEvents = ActiveEvents;
	OutEventHistory = EventHistory;
	OutNextEventID  = NextEventID;
	OutEventRng     = EventRng;
}

void UCoMWorldEventSubsystem::ImportAll(const TArray<FCoMWorldEvent>& InActiveEvents,
                                         const TArray<FCoMWorldEvent>& InEventHistory,
                                         int32 InNextEventID,
                                         const FCoMDeterministicRandom& InEventRng)
{
	ActiveEvents = InActiveEvents;
	EventHistory = InEventHistory;
	NextEventID  = InNextEventID;
	EventRng     = InEventRng;

	UE_LOG(LogTemp, Log, TEXT("[WorldEvents] ImportAll: %d active, %d history, NextID=%d"),
	       ActiveEvents.Num(), EventHistory.Num(), NextEventID);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal — Expire finished events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ExpireFinishedEvents(int32 CurrentTurn)
{
	// Iterate in reverse so removal doesn't invalidate indices
	for (int32 i = ActiveEvents.Num() - 1; i >= 0; --i)
	{
		const FCoMWorldEvent& Event = ActiveEvents[i];
		if (Event.TurnTriggered + Event.Duration <= CurrentTurn)
		{
			ExpireEvent(Event.EventID);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal — Roll for new events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::RollForNewEvents(int32 CurrentTurn)
{
	// Determine discovered planes from game state
	TSet<ECoMPlane> DiscoveredPlanes;
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World)
	{
		if (const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
		{
			DiscoveredPlanes = GS->DiscoveredPlanes;
		}
	}

	// Fallback: at minimum, Aurelith is always discovered
	if (DiscoveredPlanes.Num() == 0)
	{
		DiscoveredPlanes.Add(ECoMPlane::Aurelith);
	}

	// Base chance: 5% per turn per plane, +1% per 10 turns (capped at 15%)
	const int32 BonusPercent = FMath::Min(CurrentTurn / 10, 10);
	const int32 BaseChance   = 5 + BonusPercent; // 5-15%

	for (ECoMPlane Plane : DiscoveredPlanes)
	{
		if (EventRng.RollPercent(BaseChance))
		{
			ECoMWorldEventType SelectedType = SelectEventTypeForPlane(Plane, CurrentTurn);
			if (SelectedType == ECoMWorldEventType::None)
			{
				continue;
			}

			// Don't stack duplicate event types on the same plane
			bool bAlreadyActive = false;
			for (const FCoMWorldEvent& Existing : ActiveEvents)
			{
				if (Existing.Type == SelectedType && Existing.AffectedPlanes.Contains(Plane))
				{
					bAlreadyActive = true;
					break;
				}
			}
			if (bAlreadyActive)
			{
				continue;
			}

			FIntPoint Epicenter = GetRandomPosition();

			// Set TurnTriggered before TriggerEvent
			// TriggerEvent creates the event; we patch TurnTriggered on the last added event
			const int32 NewID = TriggerEvent(SelectedType, Plane, Epicenter);

			// Patch TurnTriggered on the event we just added
			for (FCoMWorldEvent& Event : ActiveEvents)
			{
				if (Event.EventID == NewID)
				{
					Event.TurnTriggered = CurrentTurn;
					break;
				}
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal — Select event type weighted by game phase and plane
// ─────────────────────────────────────────────────────────────────────────────

ECoMWorldEventType UCoMWorldEventSubsystem::SelectEventTypeForPlane(ECoMPlane Plane, int32 CurrentTurn)
{
	// Build a weighted candidate list based on game phase
	struct FEventCandidate
	{
		ECoMWorldEventType Type;
		int32 Weight;
	};

	TArray<FEventCandidate> Candidates;

	// ── Early game (always available): mild events ──────────────────────
	Candidates.Add({ECoMWorldEventType::SolarEclipse,     10});
	Candidates.Add({ECoMWorldEventType::LunarAlignment,   10});
	Candidates.Add({ECoMWorldEventType::CometAppearance,   8});
	Candidates.Add({ECoMWorldEventType::StarFall,           6});
	Candidates.Add({ECoMWorldEventType::GlobalBloom,       10});
	Candidates.Add({ECoMWorldEventType::AetherTide,         5});

	// ── Mid game (turn 31+): medium events ──────────────────────────────
	if (CurrentTurn > 30)
	{
		Candidates.Add({ECoMWorldEventType::Plague,                8});
		Candidates.Add({ECoMWorldEventType::MagicStorm,            7});
		Candidates.Add({ECoMWorldEventType::DragonMatingFlight,    6});
		Candidates.Add({ECoMWorldEventType::OrganizationWar,       5});
		Candidates.Add({ECoMWorldEventType::MerchantCrisis,        7});
		Candidates.Add({ECoMWorldEventType::HeroicAge,             6});
		Candidates.Add({ECoMWorldEventType::GreatDrought,          6});
		Candidates.Add({ECoMWorldEventType::PlanarConvergence,     5});
		Candidates.Add({ECoMWorldEventType::PlanarBleeding,        4});
		Candidates.Add({ECoMWorldEventType::PlanarSealWeakening,   4});
		Candidates.Add({ECoMWorldEventType::VolcanicAge,           3});
		Candidates.Add({ECoMWorldEventType::SoulHarvest,           3});
		Candidates.Add({ECoMWorldEventType::GreatUpheaval,         4});
		Candidates.Add({ECoMWorldEventType::DeepQuake,             4});
		Candidates.Add({ECoMWorldEventType::DragonAwakening,       3});
	}

	// ── Late game (turn 100+): apocalyptic events ───────────────────────
	if (CurrentTurn > 100)
	{
		Candidates.Add({ECoMWorldEventType::UndeathRising,        5});
		Candidates.Add({ECoMWorldEventType::DimensionalRift,      4});
		Candidates.Add({ECoMWorldEventType::RealityFracture,      3});
		Candidates.Add({ECoMWorldEventType::DragonConvocation,    3});
		Candidates.Add({ECoMWorldEventType::AncientLichAwakens,   2});
		Candidates.Add({ECoMWorldEventType::MassExtinction,       3});
		Candidates.Add({ECoMWorldEventType::ProphecyFulfilled,    2});
		Candidates.Add({ECoMWorldEventType::UnderdarkFlood,       3});
	}

	// ── Plane-specific weighting ────────────────────────────────────────
	// Boost or restrict certain events based on the plane
	for (FEventCandidate& Candidate : Candidates)
	{
		switch (Candidate.Type)
		{
			// UndeathRising more likely on Noctharion
			case ECoMWorldEventType::UndeathRising:
			case ECoMWorldEventType::SoulHarvest:
			case ECoMWorldEventType::AncientLichAwakens:
				if (Plane == ECoMPlane::Noctharion) { Candidate.Weight *= 3; }
				else if (Plane == ECoMPlane::Aurelith) { Candidate.Weight /= 2; }
				break;

			// ChaosStorm only on Abyssal
			case ECoMWorldEventType::ChaosStorm:
				if (Plane != ECoMPlane::Abyssal) { Candidate.Weight = 0; }
				else { Candidate.Weight = 8; }
				break;

			// Nature events boosted on Verdantis
			case ECoMWorldEventType::GlobalBloom:
			case ECoMWorldEventType::GreatDrought:
			case ECoMWorldEventType::MassExtinction:
				if (Plane == ECoMPlane::Verdantis) { Candidate.Weight *= 2; }
				break;

			// Planar events boosted on Ethereal
			case ECoMWorldEventType::PlanarConvergence:
			case ECoMWorldEventType::PlanarBleeding:
			case ECoMWorldEventType::PlanarSealWeakening:
				if (Plane == ECoMPlane::Ethereal) { Candidate.Weight *= 2; }
				break;

			// Volcanic/fire events boosted on Infernyx
			case ECoMWorldEventType::VolcanicAge:
				if (Plane == ECoMPlane::Infernyx) { Candidate.Weight *= 3; }
				break;

			// Dragon events boosted on Aurelith/Verdantis (where dragons roam)
			case ECoMWorldEventType::DragonMatingFlight:
			case ECoMWorldEventType::DragonConvocation:
			case ECoMWorldEventType::DragonAwakening:
				if (Plane == ECoMPlane::Aurelith || Plane == ECoMPlane::Verdantis)
				{
					Candidate.Weight = static_cast<int32>(Candidate.Weight * 1.5f);
				}
				break;

			// Magic events boosted on Aethermist
			case ECoMWorldEventType::MagicStorm:
			case ECoMWorldEventType::AetherTide:
			case ECoMWorldEventType::LunarAlignment:
				if (Plane == ECoMPlane::Aethermist) { Candidate.Weight *= 2; }
				break;

			// Underdark events only on planes with underdark layers (all planes have them)
			case ECoMWorldEventType::GreatUpheaval:
			case ECoMWorldEventType::DeepQuake:
			case ECoMWorldEventType::UnderdarkFlood:
				// Slightly less likely on Aethermist (celestial plane)
				if (Plane == ECoMPlane::Aethermist) { Candidate.Weight /= 2; }
				break;

			// Chaos events boosted on Abyssal
			case ECoMWorldEventType::DimensionalRift:
			case ECoMWorldEventType::RealityFracture:
				if (Plane == ECoMPlane::Abyssal) { Candidate.Weight *= 2; }
				break;

			// Feywild-specific boosts for social/celestial
			case ECoMWorldEventType::CometAppearance:
			case ECoMWorldEventType::HeroicAge:
				if (Plane == ECoMPlane::Feywild) { Candidate.Weight *= 2; }
				break;

			default:
				break;
		}
	}

	// Also add ChaosStorm as a candidate if on Abyssal and mid-game+
	if (Plane == ECoMPlane::Abyssal && CurrentTurn > 30)
	{
		// Check if ChaosStorm is already in the list (it was handled above but may
		// have been added with weight 0 by the default early-game list — it wasn't).
		bool bHasChaosStorm = false;
		for (const FEventCandidate& C : Candidates)
		{
			if (C.Type == ECoMWorldEventType::ChaosStorm)
			{
				bHasChaosStorm = true;
				break;
			}
		}
		if (!bHasChaosStorm)
		{
			Candidates.Add({ECoMWorldEventType::ChaosStorm, 8});
		}
	}

	// Remove zero-weight candidates
	Candidates.RemoveAll([](const FEventCandidate& C) { return C.Weight <= 0; });

	if (Candidates.Num() == 0)
	{
		return ECoMWorldEventType::None;
	}

	// Sum weights and roll
	int32 TotalWeight = 0;
	for (const FEventCandidate& C : Candidates)
	{
		TotalWeight += C.Weight;
	}

	int32 Roll = EventRng.NextIntRange(0, TotalWeight);
	for (const FEventCandidate& C : Candidates)
	{
		Roll -= C.Weight;
		if (Roll < 0)
		{
			return C.Type;
		}
	}

	// Fallback (should not reach here)
	return Candidates.Last().Type;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal — Apply ongoing effects
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ApplyOngoingEffects(int32 CurrentTurn)
{
	for (const FCoMWorldEvent& Event : ActiveEvents)
	{
		ApplyOngoingEffect(Event, CurrentTurn);
	}
}

void UCoMWorldEventSubsystem::ApplyOngoingEffect(const FCoMWorldEvent& Event, int32 CurrentTurn)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::Plague:
		{
			// Cities lose 1 population per turn (handled via CitySubsystem)
			// Plague spreads along trade routes each turn
			if (UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>())
			{
				// TODO: Cities->ApplyPlagueEffect(Event.AffectedPlanes, Event.EpicenterPosition, Event.EpicenterRadius);
				UE_LOG(LogTemp, Log, TEXT("[WorldEvents] Plague ongoing: would reduce city population on affected planes."));
			}
			break;
		}

		case ECoMWorldEventType::MagicStorm:
		{
			// Random spells cast at random locations each turn
			if (UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>())
			{
				// Pick a random location on the affected plane
				FIntPoint Target = GetRandomPosition();
				UE_LOG(LogTemp, Log, TEXT("[WorldEvents] MagicStorm ongoing: random spell effect at (%d,%d)."),
				       Target.X, Target.Y);
				// TODO: Magic->CastRandomSpellAtLocation(Event.AffectedPlanes[0], Target);
			}
			break;
		}

		case ECoMWorldEventType::PlanarBleeding:
		{
			// Terrain on one plane slowly converts to another plane's type
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] PlanarBleeding ongoing: terrain corruption spreading."));
			// TODO: WorldMap->ConvertTerrain(...) — corruption spread per turn
			break;
		}

		case ECoMWorldEventType::RealityFracture:
		{
			// Terrain randomly swaps between planes each turn
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] RealityFracture ongoing: terrain swapping."));
			// TODO: Swap random tiles between planes
			break;
		}

		case ECoMWorldEventType::UndeathRising:
		{
			// Undead units spawn from graveyards/battlefields each turn
			if (UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>())
			{
				for (ECoMPlane Plane : Event.AffectedPlanes)
				{
					FIntPoint SpawnPos = GetRandomPosition();
					// Spawn a basic undead unit as neutral hostile (wizard index -1)
					// TODO: Use proper undead SpecID from data assets
					UE_LOG(LogTemp, Log, TEXT("[WorldEvents] UndeathRising: spawning undead at (%d,%d) on plane %d."),
					       SpawnPos.X, SpawnPos.Y, static_cast<int32>(Plane));
					// Units->SpawnUnit(UndeadSpecID, Plane, ECoMMapLayer::Surface, SpawnPos, CoM::WIZARD_INDEX_NONE);
				}
			}
			break;
		}

		default:
			// Most events apply their effect at resolve time and don't tick per-turn
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Celestial events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveCelestialEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::SolarEclipse:
		{
			// +20% undead unit power on affected plane for duration, -10% ranged accuracy
			// Applied via GlobalModifiers on the event struct — combat subsystem reads these
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] SolarEclipse: +20%% undead power, -10%% ranged accuracy for %d turns."),
			       Event.Duration);

			// Modifiers are stored on the event and queried by combat/unit subsystems
			// The event struct's GlobalModifiers array is the canonical source
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier UndeadBoost;
				UndeadBoost.StatName   = FName("UndeadPower");
				UndeadBoost.Multiplier = FFixed64(1.2f); // +20%
				MutableEvent->GlobalModifiers.Add(UndeadBoost);

				FCoMStatModifier RangedPenalty;
				RangedPenalty.StatName   = FName("RangedAccuracy");
				RangedPenalty.Multiplier = FFixed64(0.9f); // -10%
				MutableEvent->GlobalModifiers.Add(RangedPenalty);
			}
			break;
		}

		case ECoMWorldEventType::LunarAlignment:
		{
			// +30% mana income for all wizards for duration
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] LunarAlignment: +30%% mana income for %d turns."),
			       Event.Duration);

			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier ManaBoost;
				ManaBoost.StatName   = FName("ManaIncome");
				ManaBoost.Multiplier = FFixed64(1.3f); // +30%
				MutableEvent->GlobalModifiers.Add(ManaBoost);
			}
			break;
		}

		case ECoMWorldEventType::CometAppearance:
		{
			// Random hero appears at a random location, claimable by first wizard
			if (UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>())
			{
				FIntPoint HeroPos = GetRandomPosition();
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
				UE_LOG(LogTemp, Log,
				       TEXT("[WorldEvents] CometAppearance: hero spawning at (%d,%d) on plane %d."),
				       HeroPos.X, HeroPos.Y, static_cast<int32>(Plane));
				// TODO: Heroes->SpawnWanderingHero(Plane, HeroPos);
			}
			break;
		}

		case ECoMWorldEventType::StarFall:
		{
			// Spawns rare resource node at random location
			if (UCoMWorldMapSubsystem* WorldMap = GI->GetSubsystem<UCoMWorldMapSubsystem>())
			{
				FIntPoint ResourcePos = GetRandomPosition();
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
				UE_LOG(LogTemp, Log,
				       TEXT("[WorldEvents] StarFall: rare resource node at (%d,%d) on plane %d."),
				       ResourcePos.X, ResourcePos.Y, static_cast<int32>(Plane));
				// TODO: WorldMap->PlaceResourceNode(Plane, ResourcePos, ECoMResource::StarMetal);
			}
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Planar events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolvePlanarEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::PlanarConvergence:
		{
			// Two random planes get portal connections for 10 turns
			// Select a second plane different from the affected one
			ECoMPlane SourcePlane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
			const int32 MaxPlane = static_cast<int32>(ECoMPlane::MAX);
			int32 TargetIdx = EventRng.NextIntRange(0, MaxPlane);
			ECoMPlane TargetPlane = static_cast<ECoMPlane>(TargetIdx);
			if (TargetPlane == SourcePlane)
			{
				TargetPlane = static_cast<ECoMPlane>((TargetIdx + 1) % MaxPlane);
			}

			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] PlanarConvergence: portal between plane %d and plane %d for %d turns."),
			       static_cast<int32>(SourcePlane), static_cast<int32>(TargetPlane), Event.Duration);
			// TODO: WorldMap->CreateTemporaryPortal(SourcePlane, TargetPlane, Event.Duration);
			break;
		}

		case ECoMWorldEventType::PlanarBleeding:
		{
			// Terrain on one plane slowly converts to another plane's type (per-turn in ApplyOngoingEffect)
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] PlanarBleeding: corruption spread initiated on plane %d."),
			       Event.AffectedPlanes.Num() > 0 ? static_cast<int32>(Event.AffectedPlanes[0]) : -1);
			break;
		}

		case ECoMWorldEventType::PlanarSealWeakening:
		{
			// Planar barriers weaken — summoning costs reduced
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier SummonDiscount;
				SummonDiscount.StatName   = FName("SummoningCost");
				SummonDiscount.Multiplier = FFixed64(0.7f); // -30% summoning cost
				MutableEvent->GlobalModifiers.Add(SummonDiscount);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] PlanarSealWeakening: -30%% summoning costs for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::AetherTide:
		{
			// +50% spell casting cost for 5 turns, mana nodes produce double
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier CastCostIncrease;
				CastCostIncrease.StatName   = FName("SpellCastingCost");
				CastCostIncrease.Multiplier = FFixed64(1.5f); // +50%
				MutableEvent->GlobalModifiers.Add(CastCostIncrease);

				FCoMStatModifier ManaNodeBoost;
				ManaNodeBoost.StatName   = FName("ManaNodeProduction");
				ManaNodeBoost.Multiplier = FFixed64(2.0f); // double
				MutableEvent->GlobalModifiers.Add(ManaNodeBoost);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] AetherTide: +50%% spell cost, 2x mana node production for %d turns."),
			       Event.Duration);
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Dragon events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveDragonEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::DragonMatingFlight:
		{
			// Spawn dragon eggs at random lair locations
			if (UCoMDragonSubsystem* Dragons = GI->GetSubsystem<UCoMDragonSubsystem>())
			{
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
				const int32 EggCount = EventRng.NextIntRange(1, 4); // 1-3 eggs
				for (int32 i = 0; i < EggCount; ++i)
				{
					FIntPoint LairPos = GetRandomPosition();
					UE_LOG(LogTemp, Log,
					       TEXT("[WorldEvents] DragonMatingFlight: egg spawned at (%d,%d) on plane %d."),
					       LairPos.X, LairPos.Y, static_cast<int32>(Plane));
					// TODO: Dragons->SpawnDragonEgg(Plane, LairPos);
				}
			}
			break;
		}

		case ECoMWorldEventType::DragonConvocation:
		{
			// All wild dragons converge to one location — dangerous but lucrative
			FIntPoint ConvergePoint = GetRandomPosition();
			ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] DragonConvocation: all dragons converge to (%d,%d) on plane %d."),
			       ConvergePoint.X, ConvergePoint.Y, static_cast<int32>(Plane));
			// TODO: Dragons->ConvocateAllWildDragons(Plane, ConvergePoint);
			break;
		}

		case ECoMWorldEventType::DragonAwakening:
		{
			// Ancient dragon spawns as neutral hostile
			if (UCoMDragonSubsystem* Dragons = GI->GetSubsystem<UCoMDragonSubsystem>())
			{
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
				FIntPoint LairPos = GetRandomPosition();
				FRandomStream TempRng(static_cast<int32>(EventRng.NextUInt32()));
				int32 DragonID = Dragons->SpawnDragon(/*TypeID=*/0, Plane, LairPos, TempRng);
				UE_LOG(LogTemp, Log,
				       TEXT("[WorldEvents] DragonAwakening: ancient dragon %d spawned at (%d,%d) on plane %d."),
				       DragonID, LairPos.X, LairPos.Y, static_cast<int32>(Plane));
			}
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Nature events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveNatureEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::GlobalBloom:
		{
			// +50% food production for duration
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier FoodBoost;
				FoodBoost.StatName   = FName("FoodProduction");
				FoodBoost.Multiplier = FFixed64(1.5f); // +50%
				MutableEvent->GlobalModifiers.Add(FoodBoost);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] GlobalBloom: +50%% food production for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::GreatDrought:
		{
			// -50% food, increased fire damage
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier FoodPenalty;
				FoodPenalty.StatName   = FName("FoodProduction");
				FoodPenalty.Multiplier = FFixed64(0.5f); // -50%
				MutableEvent->GlobalModifiers.Add(FoodPenalty);

				FCoMStatModifier FireBoost;
				FireBoost.StatName   = FName("FireDamage");
				FireBoost.Multiplier = FFixed64(1.25f); // +25%
				MutableEvent->GlobalModifiers.Add(FireBoost);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] GreatDrought: -50%% food, +25%% fire damage for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::Plague:
		{
			// Cities lose 1 population per turn for duration (ongoing effect handles this)
			// Initial spread: mark trade route cities as at-risk
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] Plague: cities will lose population each turn for %d turns. Spreads via trade routes."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::MassExtinction:
		{
			// Neutral wildlife units die, forest terrain loses yield
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier ForestYield;
				ForestYield.StatName   = FName("ForestYield");
				ForestYield.Multiplier = FFixed64(0.5f); // -50%
				MutableEvent->GlobalModifiers.Add(ForestYield);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] MassExtinction: neutral wildlife removed, forest yield -50%% for %d turns."),
			       Event.Duration);
			// TODO: Units->DespawnNeutralWildlife(Event.AffectedPlanes);
			break;
		}

		// Underdark events handled via nature resolution
		case ECoMWorldEventType::GreatUpheaval:
		{
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] GreatUpheaval: underdark passages shifting."));
			// TODO: WorldMap->ReshuffleUnderdarkConnections(Event.AffectedPlanes);
			break;
		}

		case ECoMWorldEventType::DeepQuake:
		{
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier MiningPenalty;
				MiningPenalty.StatName   = FName("MiningYield");
				MiningPenalty.Multiplier = FFixed64(0.7f); // -30%
				MutableEvent->GlobalModifiers.Add(MiningPenalty);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] DeepQuake: -30%% mining yield for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::UnderdarkFlood:
		{
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] UnderdarkFlood: underdark layers partially impassable for %d turns."),
			       Event.Duration);
			// TODO: WorldMap->FloodUnderdarkTiles(Event.AffectedPlanes, Event.EpicenterPosition, Event.EpicenterRadius);
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Chaos events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveChaosEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::MagicStorm:
		{
			// Random spells cast at random locations each turn (ongoing effect)
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier SpellResistPenalty;
				SpellResistPenalty.StatName   = FName("SpellResistance");
				SpellResistPenalty.Multiplier = FFixed64(0.8f); // -20%
				MutableEvent->GlobalModifiers.Add(SpellResistPenalty);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] MagicStorm: random spells + -20%% spell resistance for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::VolcanicAge:
		{
			// Volcanic terrain spawns, fire damage boosted
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier FireBoost;
				FireBoost.StatName   = FName("FireDamage");
				FireBoost.Multiplier = FFixed64(1.5f); // +50%
				MutableEvent->GlobalModifiers.Add(FireBoost);

				FCoMStatModifier MovePenalty;
				MovePenalty.StatName   = FName("MovementSpeed");
				MovePenalty.Multiplier = FFixed64(0.8f); // -20%
				MutableEvent->GlobalModifiers.Add(MovePenalty);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] VolcanicAge: +50%% fire damage, -20%% movement for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::DimensionalRift:
		{
			// Spawns hostile void creatures
			if (UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>())
			{
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Aurelith;
				const int32 CreatureCount = EventRng.NextIntRange(2, 6); // 2-5 creatures
				for (int32 i = 0; i < CreatureCount; ++i)
				{
					FIntPoint SpawnPos = GetRandomPosition();
					UE_LOG(LogTemp, Log,
					       TEXT("[WorldEvents] DimensionalRift: void creature at (%d,%d) on plane %d."),
					       SpawnPos.X, SpawnPos.Y, static_cast<int32>(Plane));
					// TODO: Units->SpawnUnit(VoidCreatureSpecID, Plane, ECoMMapLayer::Surface, SpawnPos, CoM::WIZARD_INDEX_NONE);
				}
			}
			break;
		}

		case ECoMWorldEventType::RealityFracture:
		{
			// Terrain randomly swaps between planes (ongoing effect)
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] RealityFracture: terrain swapping each turn for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::ChaosStorm:
		{
			// Abyssal-specific: random devastation, corruption spread
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier ChaosDamage;
				ChaosDamage.StatName   = FName("ChaosDamage");
				ChaosDamage.Multiplier = FFixed64(1.5f); // +50%
				MutableEvent->GlobalModifiers.Add(ChaosDamage);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] ChaosStorm: +50%% chaos damage, corruption spreading for %d turns."),
			       Event.Duration);
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Death events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveDeathEvent(const FCoMWorldEvent& Event)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	switch (Event.Type)
	{
		case ECoMWorldEventType::UndeathRising:
		{
			// Undead units spawn from graveyards/battlefields (ongoing effect handles per-turn spawns)
			// Initial burst: spawn a small wave
			if (UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>())
			{
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Noctharion;
				const int32 InitialWave = EventRng.NextIntRange(3, 8); // 3-7 undead
				for (int32 i = 0; i < InitialWave; ++i)
				{
					FIntPoint SpawnPos = GetRandomPosition();
					UE_LOG(LogTemp, Log,
					       TEXT("[WorldEvents] UndeathRising: initial undead at (%d,%d) on plane %d."),
					       SpawnPos.X, SpawnPos.Y, static_cast<int32>(Plane));
					// TODO: Units->SpawnUnit(UndeadSpecID, Plane, ECoMMapLayer::Surface, SpawnPos, CoM::WIZARD_INDEX_NONE);
				}
			}
			break;
		}

		case ECoMWorldEventType::SoulHarvest:
		{
			// Death magic strengthened, life magic weakened
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier DeathMagicBoost;
				DeathMagicBoost.StatName   = FName("DeathMagicPower");
				DeathMagicBoost.Multiplier = FFixed64(1.3f); // +30%
				MutableEvent->GlobalModifiers.Add(DeathMagicBoost);

				FCoMStatModifier LifeMagicPenalty;
				LifeMagicPenalty.StatName   = FName("LifeMagicPower");
				LifeMagicPenalty.Multiplier = FFixed64(0.7f); // -30%
				MutableEvent->GlobalModifiers.Add(LifeMagicPenalty);
			}
			UE_LOG(LogTemp, Log,
			       TEXT("[WorldEvents] SoulHarvest: +30%% death magic, -30%% life magic for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::AncientLichAwakens:
		{
			// Powerful neutral lich with army appears
			if (UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>())
			{
				ECoMPlane Plane = Event.AffectedPlanes.Num() > 0 ? Event.AffectedPlanes[0] : ECoMPlane::Noctharion;
				FIntPoint LichPos = GetRandomPosition();

				// Spawn lich + army
				UE_LOG(LogTemp, Log,
				       TEXT("[WorldEvents] AncientLichAwakens: lich + army at (%d,%d) on plane %d."),
				       LichPos.X, LichPos.Y, static_cast<int32>(Plane));

				// Create army for the lich
				int32 ArmyID = Units->CreateArmy(CoM::WIZARD_INDEX_NONE, Plane, ECoMMapLayer::Surface, LichPos);
				// TODO: Spawn lich hero unit + undead minion units into the army
				// Units->SpawnUnit(LichSpecID, Plane, ECoMMapLayer::Surface, LichPos, CoM::WIZARD_INDEX_NONE);
				(void)ArmyID; // suppress unused warning until units are spawned
			}
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Effect resolution — Social events
// ─────────────────────────────────────────────────────────────────────────────

void UCoMWorldEventSubsystem::ResolveSocialEvent(const FCoMWorldEvent& Event)
{
	switch (Event.Type)
	{
		case ECoMWorldEventType::OrganizationWar:
		{
			// Internal strife — city unrest increases
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier UnrestIncrease;
				UnrestIncrease.StatName = FName("CityUnrest");
				UnrestIncrease.FlatBonus = FFixed64(2); // +2 unrest per city
				MutableEvent->GlobalModifiers.Add(UnrestIncrease);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] OrganizationWar: +2 city unrest for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::MerchantCrisis:
		{
			// Trade routes disrupted, gold income -25%
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier GoldPenalty;
				GoldPenalty.StatName   = FName("GoldIncome");
				GoldPenalty.Multiplier = FFixed64(0.75f); // -25%
				MutableEvent->GlobalModifiers.Add(GoldPenalty);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] MerchantCrisis: -25%% gold income for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::HeroicAge:
		{
			// +2 hero recruitment slots for all wizards
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier HeroSlots;
				HeroSlots.StatName  = FName("HeroRecruitSlots");
				HeroSlots.FlatBonus = FFixed64(2); // +2 slots
				MutableEvent->GlobalModifiers.Add(HeroSlots);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] HeroicAge: +2 hero recruitment slots for %d turns."),
			       Event.Duration);
			break;
		}

		case ECoMWorldEventType::ProphecyFulfilled:
		{
			// Major narrative event — boosts all stats for the triggering plane
			FCoMWorldEvent* MutableEvent = const_cast<FCoMWorldEvent*>(GetEventByID(Event.EventID));
			if (MutableEvent)
			{
				FCoMStatModifier ProphecyBoost;
				ProphecyBoost.StatName   = FName("AllStats");
				ProphecyBoost.Multiplier = FFixed64(1.1f); // +10% to everything
				MutableEvent->GlobalModifiers.Add(ProphecyBoost);
			}
			UE_LOG(LogTemp, Log, TEXT("[WorldEvents] ProphecyFulfilled: +10%% all stats for %d turns."),
			       Event.Duration);
			break;
		}

		default:
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMWorldEventSubsystem::GetDefaultDuration(ECoMWorldEventType Type)
{
	switch (Type)
	{
		// Short duration (3 turns)
		case ECoMWorldEventType::LunarAlignment:
		case ECoMWorldEventType::Plague:
			return 3;

		// Standard duration (5 turns)
		case ECoMWorldEventType::SolarEclipse:
		case ECoMWorldEventType::GlobalBloom:
		case ECoMWorldEventType::GreatDrought:
		case ECoMWorldEventType::MagicStorm:
		case ECoMWorldEventType::MerchantCrisis:
		case ECoMWorldEventType::AetherTide:
		case ECoMWorldEventType::SoulHarvest:
		case ECoMWorldEventType::ChaosStorm:
		case ECoMWorldEventType::DeepQuake:
			return 5;

		// Long duration (10 turns)
		case ECoMWorldEventType::PlanarConvergence:
		case ECoMWorldEventType::HeroicAge:
		case ECoMWorldEventType::VolcanicAge:
		case ECoMWorldEventType::UndeathRising:
		case ECoMWorldEventType::RealityFracture:
		case ECoMWorldEventType::PlanarBleeding:
		case ECoMWorldEventType::UnderdarkFlood:
			return 10;

		// Very long (15 turns)
		case ECoMWorldEventType::MassExtinction:
		case ECoMWorldEventType::DimensionalRift:
		case ECoMWorldEventType::PlanarSealWeakening:
		case ECoMWorldEventType::GreatUpheaval:
			return 15;

		// Instant/permanent events (1 turn — effect applied and done)
		case ECoMWorldEventType::CometAppearance:
		case ECoMWorldEventType::StarFall:
		case ECoMWorldEventType::DragonMatingFlight:
		case ECoMWorldEventType::DragonAwakening:
		case ECoMWorldEventType::AncientLichAwakens:
		case ECoMWorldEventType::ProphecyFulfilled:
			return 1;

		// Medium (8 turns)
		case ECoMWorldEventType::DragonConvocation:
		case ECoMWorldEventType::OrganizationWar:
			return 8;

		default:
			return 5;
	}
}

FIntPoint UCoMWorldEventSubsystem::GetRandomPosition()
{
	const int32 X = EventRng.NextIntRange(0, MAP_WIDTH);
	const int32 Y = EventRng.NextIntRange(0, MAP_HEIGHT);
	return FIntPoint(X, Y);
}
