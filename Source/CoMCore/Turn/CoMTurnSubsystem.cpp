// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMTurnSubsystem.cpp — Game-phase FSM implementation.
// COM-028

#include "Turn/CoMTurnSubsystem.h"
#include "Framework/CoMGameState.h"
#include "Wizards/CoMPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

// ─────────────────────────────────────────────────────────────────────────────
// USubsystem interface
// ─────────────────────────────────────────────────────────────────────────────

void UCoMTurnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentTurn          = 0;
	CurrentPhase         = ECoMGamePhase::WorldProcessing;
	ActiveWizardIndex    = CoM::WIZARD_INDEX_NONE;
	CurrentWizardPhase   = ECoMWizardPhase::Planning;
	TurnOrderPosition    = 0;
	bGameStarted         = false;
}

void UCoMTurnSubsystem::Deinitialize()
{
	WizardTurnOrder.Empty();
	CachedGameState.Reset();
	bGameStarted = false;
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Turn Lifecycle — public API
// ─────────────────────────────────────────────────────────────────────────────

void UCoMTurnSubsystem::StartGame()
{
	if (bGameStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCoMTurnSubsystem::StartGame called more than once — ignored."));
		return;
	}
	bGameStarted = true;
	BeginNextTurn();
}

void UCoMTurnSubsystem::AdvanceWizardPhase()
{
	if (CurrentPhase != ECoMGamePhase::PlayerTurn
	    && CurrentPhase != ECoMGamePhase::AITurn)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("UCoMTurnSubsystem::AdvanceWizardPhase called outside a wizard turn — ignored."));
		return;
	}

	const ECoMWizardPhase Next = static_cast<ECoMWizardPhase>(
		static_cast<uint8>(CurrentWizardPhase) + 1);

	if (Next >= ECoMWizardPhase::MAX)
	{
		// Sub-phase wrapped — shouldn't happen; treat as EndTurn
		EndWizardTurn();
		return;
	}

	SetWizardPhase(Next);

	if (CurrentWizardPhase == ECoMWizardPhase::EndTurn)
	{
		EndWizardTurn();
	}
}

void UCoMTurnSubsystem::SkipToNextWizard()
{
	if (CurrentPhase != ECoMGamePhase::PlayerTurn
	    && CurrentPhase != ECoMGamePhase::AITurn)
	{
		return;
	}
	EndWizardTurn();
}

void UCoMTurnSubsystem::AdvanceGlobalPhase()
{
	switch (CurrentPhase)
	{
		case ECoMGamePhase::WorldProcessing:
			// WorldProcessing is self-contained — this call is valid as a manual override
			// but normally ProcessWorldPhase() calls the next stage directly.
			if (WizardTurnOrder.Num() > 0)
			{
				TurnOrderPosition = 0;
				BeginWizardTurn(WizardTurnOrder[0]);
			}
			else
			{
				// No active wizards — skip straight to CombatResolution
				ProcessCombatResolution();
			}
			break;

		case ECoMGamePhase::CombatResolution:
			ProcessEndOfTurn();
			break;

		case ECoMGamePhase::EndOfTurn:
			BeginNextTurn();
			break;

		default:
			UE_LOG(LogTemp, Warning,
			       TEXT("UCoMTurnSubsystem::AdvanceGlobalPhase called during wizard turn — use AdvanceWizardPhase / SkipToNextWizard."));
			break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMTurnSubsystem::IsPlayerTurn() const
{
	if (CurrentPhase != ECoMGamePhase::PlayerTurn) { return false; }
	// PlayerTurn means it's a human-controlled wizard; AITurn means it's AI
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase Processors — private
// ─────────────────────────────────────────────────────────────────────────────

void UCoMTurnSubsystem::ProcessWorldPhase()
{
	SetPhase(ECoMGamePhase::WorldProcessing);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: Turn %d — WorldProcessing phase."), CurrentTurn);

	// Subsystems that need per-turn ticks hook into OnGamePhaseChanged (WorldProcessing):
	//   - UCoMWeatherSubsystem  (phase advance + tile effects)
	//   - UCoMMigrationSubsystem (population migration)
	//   - UCoMDiseaseSubsystem   (disease spread checks)
	//   - Ritual stability ticks (ACoMGameState::ActiveRituals)
	//   - Ley Line power fluctuation
	//   - Corruption zone expansion
	// Those subsystems listen to OnGamePhaseChanged and do their work asynchronously
	// (within the same frame tick), then the game mode calls AdvanceGlobalPhase().

	// Build wizard turn order after world processing so any new/dead wizards are captured.
	BuildWizardTurnOrder();
	OnTurnStarted.Broadcast(CurrentTurn);
}

void UCoMTurnSubsystem::BeginWizardTurn(int32 WizardIndex)
{
	const ECoMGamePhase WizardPhaseType = GetWizardPhaseType(WizardIndex);
	SetPhase(WizardPhaseType);
	SetActiveWizard(WizardIndex);
	SetWizardPhase(ECoMWizardPhase::Planning);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: Turn %d — Wizard %d begins (%s)."),
	       CurrentTurn, WizardIndex,
	       WizardPhaseType == ECoMGamePhase::PlayerTurn ? TEXT("Player") : TEXT("AI"));
}

void UCoMTurnSubsystem::EndWizardTurn()
{
	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: Turn %d — Wizard %d ends turn."),
	       CurrentTurn, ActiveWizardIndex);

	// Advance to next wizard in order
	++TurnOrderPosition;
	if (TurnOrderPosition < WizardTurnOrder.Num())
	{
		BeginWizardTurn(WizardTurnOrder[TurnOrderPosition]);
	}
	else
	{
		// All wizards done — global combat resolution
		SetActiveWizard(CoM::WIZARD_INDEX_NONE);
		ProcessCombatResolution();
	}
}

void UCoMTurnSubsystem::ProcessCombatResolution()
{
	SetPhase(ECoMGamePhase::CombatResolution);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: Turn %d — CombatResolution phase."), CurrentTurn);

	// UCoMCombatSubsystem (Phase 4) listens for ECoMGamePhase::CombatResolution
	// and resolves queued army-vs-army engagements, then calls AdvanceGlobalPhase().
}

void UCoMTurnSubsystem::ProcessEndOfTurn()
{
	SetPhase(ECoMGamePhase::EndOfTurn);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: Turn %d — EndOfTurn phase."), CurrentTurn);

	OnTurnEnded.Broadcast(CurrentTurn);

	// Diplomacy ticks, treaty expiry, tribute collection, victory condition check
	// all happen in subsystems listening to ECoMGamePhase::EndOfTurn.
	// After those complete, the game mode calls AdvanceGlobalPhase() → BeginNextTurn().
}

void UCoMTurnSubsystem::BeginNextTurn()
{
	++CurrentTurn;

	// Write to game state before broadcasting
	if (ACoMGameState* GS = CachedGameState.Get())
	{
		GS->CurrentTurn = CurrentTurn;
	}

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMTurnSubsystem: ── Beginning Turn %d ──"), CurrentTurn);

	ProcessWorldPhase();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void UCoMTurnSubsystem::BuildWizardTurnOrder()
{
	WizardTurnOrder.Empty();

	const ACoMGameState* GS = CachedGameState.Get();
	if (!GS)
	{
		// Cache miss — try to resolve via world
		UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
		if (World)
		{
			CachedGameState = Cast<ACoMGameState>(World->GetGameState());
			GS = CachedGameState.Get();
		}
	}

	if (!GS)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("UCoMTurnSubsystem::BuildWizardTurnOrder — ACoMGameState not found. "
		            "Using empty wizard order (WorldProcessing will fire OnTurnStarted)."));
		return;
	}

	// Simple ascending index order; initiative / speed bonuses applied by game designer later.
	for (int32 i = 0; i < GS->WizardStates.Num(); ++i)
	{
		if (GS->WizardStates[i] != nullptr)
		{
			WizardTurnOrder.Add(i);
		}
	}
}

void UCoMTurnSubsystem::SyncToGameState()
{
	ACoMGameState* GS = CachedGameState.Get();
	if (!GS) { return; }
	GS->CurrentPhase      = CurrentPhase;
	GS->ActiveWizardIndex = ActiveWizardIndex;
}

void UCoMTurnSubsystem::SetPhase(ECoMGamePhase NewPhase)
{
	if (CurrentPhase == NewPhase) { return; }
	const ECoMGamePhase Old = CurrentPhase;
	CurrentPhase = NewPhase;
	SyncToGameState();
	OnGamePhaseChanged.Broadcast(Old, NewPhase);
}

void UCoMTurnSubsystem::SetActiveWizard(int32 NewIndex)
{
	if (ActiveWizardIndex == NewIndex) { return; }
	const int32 Old = ActiveWizardIndex;
	ActiveWizardIndex = NewIndex;
	SyncToGameState();
	OnActiveWizardChanged.Broadcast(Old, NewIndex);
}

void UCoMTurnSubsystem::SetWizardPhase(ECoMWizardPhase NewPhase)
{
	if (CurrentWizardPhase == NewPhase) { return; }
	const ECoMWizardPhase Old = CurrentWizardPhase;
	CurrentWizardPhase = NewPhase;
	OnWizardPhaseChanged.Broadcast(Old, NewPhase);
}

ECoMGamePhase UCoMTurnSubsystem::GetWizardPhaseType(int32 WizardIndex) const
{
	const ACoMGameState* GS = CachedGameState.Get();
	if (!GS) { return ECoMGamePhase::AITurn; }

	// If the wizard index maps to a connected human player → PlayerTurn, else AITurn.
	// Checking AController presence via ACoMPlayerState is the cleanest signal.
	// For now we key off whether the corresponding APlayerController exists in the world.
	// UCoMAISubsystem (Phase 6) will override this with proper isAI flags.
	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardIndex);
	if (PS && PS->GetOwner() != nullptr)
	{
		// Has an owning controller — treat as human player
		return ECoMGamePhase::PlayerTurn;
	}
	return ECoMGamePhase::AITurn;
}
