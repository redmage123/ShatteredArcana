// Copyright Mythforge Studios. All Rights Reserved.
// CoMAISubsystem.cpp -- AI orchestration: planner -> executor for each AI wizard.

#include "CoMAISubsystem.h"

#include "Strategic/CoMAIStrategicPlanner.h"
#include "Tactical/CoMAITacticalExecutor.h"
#include "Difficulty/CoMAIDifficultyModifier.h"
#include "Diplomacy/CoMAIWizardDiplomacy.h"

#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Wizards/CoMPlayerState.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

// ---------------------------------------------------------------------------
// USubsystem interface
// ---------------------------------------------------------------------------

void UCoMAISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Planner            = NewObject<UCoMAIStrategicPlanner>(this);
	Executor           = NewObject<UCoMAITacticalExecutor>(this);
	DifficultyModifier = NewObject<UCoMAIDifficultyModifier>(this);
	WizardDiplomacy    = NewObject<UCoMAIWizardDiplomacy>(this);
	WizardDiplomacy->InitializeDefaultPersonalities();

	CurrentDifficulty = ECoMAIDifficulty::Normal;
	bDelegateBound = false;
	bPostDiplomacyBound = false;

	// Bind to UCoMTurnSubsystem::OnGamePhaseChanged so we auto-process AI turns.
	// The turn subsystem may not be initialized yet, so we try here and also
	// lazily bind on the first ProcessAITurns call.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			TurnSub->OnGamePhaseChanged.AddDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
			bDelegateBound = true;

			// Bind to the post-diplomacy tick for ProcessFullTurn batch path.
			// This fires after the diplomacy subsystem ticks but before combat,
			// so AI diplomatic changes (war declarations, alliances) affect combat.
			TurnSub->OnPostDiplomacyAITick.AddDynamic(this, &UCoMAISubsystem::OnPostDiplomacyAITick);
			bPostDiplomacyBound = true;

			// Bind to the per-turn AI tick — fires at the end of ProcessAllSubsystemTurns
			// and drives full strategic + tactical wizard execution.
			TurnSub->OnAITurnTick.AddDynamic(this, &UCoMAISubsystem::OnAITurnTick);
			bAITickBound = true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem initialized. Difficulty=Normal, DelegateBound=%s"),
	       bDelegateBound ? TEXT("Yes") : TEXT("Pending"));
}

void UCoMAISubsystem::Deinitialize()
{
	// Unbind delegates
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			if (bDelegateBound)
			{
				TurnSub->OnGamePhaseChanged.RemoveDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
			}
			if (bPostDiplomacyBound)
			{
				TurnSub->OnPostDiplomacyAITick.RemoveDynamic(this, &UCoMAISubsystem::OnPostDiplomacyAITick);
			}
			if (bAITickBound)
			{
				TurnSub->OnAITurnTick.RemoveDynamic(this, &UCoMAISubsystem::OnAITurnTick);
			}
		}
	}
	bDelegateBound = false;
	bPostDiplomacyBound = false;
	bAITickBound = false;

	Planner            = nullptr;
	Executor           = nullptr;
	DifficultyModifier = nullptr;
	WizardDiplomacy    = nullptr;
	LastStrategies.Empty();

	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Delegate callback -- auto-trigger AI turns when the game enters AITurn phase
// ---------------------------------------------------------------------------

void UCoMAISubsystem::OnGamePhaseChanged(ECoMGamePhase OldPhase, ECoMGamePhase NewPhase)
{
	if (NewPhase == ECoMGamePhase::AITurn)
	{
		ProcessAITurns();
	}
}

void UCoMAISubsystem::OnAITurnTick(int32 CurrentTurn)
{
	// Single per-turn entry point in the End-Turn flow. Run full strategic+tactical
	// AI for every AI wizard. Diplomacy already ran via OnPostDiplomacyAITick earlier
	// in the same ProcessAllSubsystemTurns sweep, so we don't double-tick it here —
	// instead we go straight to strategy + execution.
	if (!Planner || !Executor) return;

	int32 AIWizardsProcessed = 0;
	for (int32 WizardId = 0; WizardId < CoM::MAX_WIZARDS; ++WizardId)
	{
		if (!IsAIWizard(WizardId)) continue;

		FCoMAIStrategy Strategy = Planner->EvaluateStrategy(WizardId, CurrentDifficulty);
		LastStrategies.Add(WizardId, Strategy);
		Executor->ExecuteTurn(WizardId, Strategy, CurrentDifficulty);
		++AIWizardsProcessed;
	}
	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: OnAITurnTick processed %d AI wizards (turn %d)."),
		AIWizardsProcessed, CurrentTurn);
}

void UCoMAISubsystem::OnPostDiplomacyAITick(int32 CurrentTurn)
{
	// Called during ProcessFullTurn batch path — run wizard-to-wizard diplomacy
	// for all AI wizards. The FSM path handles this inside ProcessAITurns instead.
	if (!WizardDiplomacy)
	{
		return;
	}

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAISubsystem: OnPostDiplomacyAITick — running AI diplomacy for turn %d."),
	       CurrentTurn);

	for (int32 WizardId = 0; WizardId < CoM::MAX_WIZARDS; ++WizardId)
	{
		if (!IsAIWizard(WizardId))
		{
			continue;
		}

		WizardDiplomacy->ProcessDiplomacy(WizardId, CurrentTurn);
	}
}

// ---------------------------------------------------------------------------
// Main AI turn processing
// ---------------------------------------------------------------------------

void UCoMAISubsystem::ProcessAITurns()
{
	// Lazy delegate binding in case TurnSubsystem was not ready at Initialize time
	if (!bDelegateBound || !bPostDiplomacyBound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
			{
				if (!bDelegateBound)
				{
					TurnSub->OnGamePhaseChanged.AddDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
					bDelegateBound = true;
				}
				if (!bPostDiplomacyBound)
				{
					TurnSub->OnPostDiplomacyAITick.AddDynamic(this, &UCoMAISubsystem::OnPostDiplomacyAITick);
					bPostDiplomacyBound = true;
				}
				if (!bAITickBound)
				{
					TurnSub->OnAITurnTick.AddDynamic(this, &UCoMAISubsystem::OnAITurnTick);
					bAITickBound = true;
				}
				UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: Late-bound to TurnSubsystem delegates."));
			}
		}
	}

	if (!Planner || !Executor)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("UCoMAISubsystem::ProcessAITurns -- Planner or Executor is null."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: Processing AI turns..."));

	// Resolve the current turn number for diplomacy processing
	int32 CurrentTurn = 0;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			CurrentTurn = TurnSub->GetCurrentTurn();
		}
	}

	int32 AIWizardsProcessed = 0;

	for (int32 WizardId = 0; WizardId < CoM::MAX_WIZARDS; ++WizardId)
	{
		if (!IsAIWizard(WizardId))
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: === AI Wizard %d turn ==="), WizardId);

		// Step 1: Strategic evaluation (pass difficulty so planner can adjust thresholds)
		FCoMAIStrategy Strategy = Planner->EvaluateStrategy(WizardId, CurrentDifficulty);

		// Cache for debug display
		LastStrategies.Add(WizardId, Strategy);

		// Step 2: Wizard-to-wizard diplomacy (declare wars, propose alliances, trade spells)
		// Runs after strategy so decisions are informed by threat assessment, but before
		// tactical execution so new wars/alliances affect the current turn's military actions.
		if (WizardDiplomacy)
		{
			WizardDiplomacy->ProcessDiplomacy(WizardId, CurrentTurn);
		}

		// Step 3: Tactical execution (pass difficulty so executor can apply multipliers)
		Executor->ExecuteTurn(WizardId, Strategy, CurrentDifficulty);

		++AIWizardsProcessed;
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: Processed %d AI wizard turns."), AIWizardsProcessed);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void UCoMAISubsystem::SetDifficulty(ECoMAIDifficulty NewDifficulty)
{
	CurrentDifficulty = NewDifficulty;
	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: Difficulty set to %d"), (int32)NewDifficulty);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UCoMAISubsystem::IsAIWizard(int32 WizardId) const
{
	if (WizardId < 0 || WizardId >= CoM::MAX_WIZARDS)
	{
		return false;
	}

	// Resolve ACoMGameState to check wizard slot
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS)
	{
		return false;
	}

	// A wizard slot is AI-controlled if:
	//   1. The slot is occupied (WizardStates[WizardId] != nullptr)
	//   2. The player state has no owning controller (no human player)
	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	if (!PS)
	{
		return false; // Empty slot -- no wizard at all
	}

	// If the PlayerState has an owning actor (APlayerController), it is human.
	return (PS->GetOwner() == nullptr);
}

FCoMAIStrategy UCoMAISubsystem::GetLastStrategy(int32 WizardId) const
{
	const FCoMAIStrategy* Found = LastStrategies.Find(WizardId);
	return Found ? *Found : FCoMAIStrategy();
}
