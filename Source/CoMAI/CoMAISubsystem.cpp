// Copyright Mythforge Studios. All Rights Reserved.
// CoMAISubsystem.cpp -- AI orchestration: planner -> executor for each AI wizard.

#include "CoMAISubsystem.h"

#include "Strategic/CoMAIStrategicPlanner.h"
#include "Tactical/CoMAITacticalExecutor.h"
#include "Difficulty/CoMAIDifficultyModifier.h"

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

	CurrentDifficulty = ECoMAIDifficulty::Normal;
	bDelegateBound = false;

	// Bind to UCoMTurnSubsystem::OnGamePhaseChanged so we auto-process AI turns.
	// The turn subsystem may not be initialized yet, so we try here and also
	// lazily bind on the first ProcessAITurns call.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			TurnSub->OnGamePhaseChanged.AddDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
			bDelegateBound = true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem initialized. Difficulty=Normal, DelegateBound=%s"),
	       bDelegateBound ? TEXT("Yes") : TEXT("Pending"));
}

void UCoMAISubsystem::Deinitialize()
{
	// Unbind delegate
	if (bDelegateBound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
			{
				TurnSub->OnGamePhaseChanged.RemoveDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
			}
		}
		bDelegateBound = false;
	}

	Planner            = nullptr;
	Executor           = nullptr;
	DifficultyModifier = nullptr;
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

// ---------------------------------------------------------------------------
// Main AI turn processing
// ---------------------------------------------------------------------------

void UCoMAISubsystem::ProcessAITurns()
{
	// Lazy delegate binding in case TurnSubsystem was not ready at Initialize time
	if (!bDelegateBound)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
			{
				TurnSub->OnGamePhaseChanged.AddDynamic(this, &UCoMAISubsystem::OnGamePhaseChanged);
				bDelegateBound = true;
				UE_LOG(LogTemp, Log, TEXT("UCoMAISubsystem: Late-bound to TurnSubsystem delegate."));
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

		// Step 2: Tactical execution (pass difficulty so executor can apply multipliers)
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
