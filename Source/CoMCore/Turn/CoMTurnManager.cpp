// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMCore/Turn/CoMTurnManager.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Audio/CoMAudioSubsystem.h"
#include "Victory/CoMVictorySubsystem.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMTurnManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentTurnNumber = 0;
	CurrentWizardIndex = 0;
	bGameOver = false;
}

void UCoMTurnManager::Deinitialize()
{
	TurnOrder.Empty();
	WizardIsHuman.Empty();
	WizardEliminated.Empty();
	Super::Deinitialize();
}

// =====================================================================
// Game setup
// =====================================================================

void UCoMTurnManager::InitializeGame(int32 NumHumans, int32 NumAI)
{
	TurnOrder.Empty();
	WizardIsHuman.Empty();
	WizardEliminated.Empty();
	bGameOver = false;
	CurrentTurnNumber = 1;
	CurrentWizardIndex = 0;

	const int32 TotalWizards = NumHumans + NumAI;

	// Build turn order: humans first (indices 0..NumHumans-1),
	// then AI (indices NumHumans..TotalWizards-1).
	for (int32 i = 0; i < TotalWizards; ++i)
	{
		TurnOrder.Add(i);
		WizardIsHuman.Add(i, i < NumHumans);
		WizardEliminated.Add(i, false);
	}

	UE_LOG(LogTemp, Log,
		TEXT("CoMTurnManager: Game initialized — %d humans, %d AI, turn %d"),
		NumHumans, NumAI, CurrentTurnNumber);

	// TODO: call world generation subsystem here once wired.
	// e.g. GetGameInstance()->GetSubsystem<UCoMWorldGenSubsystem>()->GenerateWorld(...);

	OnTurnBegin.Broadcast(CurrentTurnNumber);
}

// =====================================================================
// Turn flow
// =====================================================================

void UCoMTurnManager::EndPlayerTurn()
{
	if (bGameOver)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoMTurnManager: EndPlayerTurn called but game is over."));
		return;
	}

	const int32 WizardId = TurnOrder.IsValidIndex(CurrentWizardIndex)
		? TurnOrder[CurrentWizardIndex]
		: -1;

	UE_LOG(LogTemp, Log,
		TEXT("CoMTurnManager: Wizard %d ended input phase (slot %d/%d)."),
		WizardId, CurrentWizardIndex + 1, TurnOrder.Num());

	// Advance to the next non-eliminated wizard.
	do
	{
		CurrentWizardIndex++;
	}
	while (CurrentWizardIndex < TurnOrder.Num() &&
		   WizardEliminated.FindRef(TurnOrder[CurrentWizardIndex]));

	if (CurrentWizardIndex >= TurnOrder.Num())
	{
		// All wizards have submitted; run the simultaneous resolution.
		ProcessFullTurn();

		// Reset for next round of input.
		CurrentWizardIndex = 0;

		// Skip eliminated wizards at the start of the new round.
		while (CurrentWizardIndex < TurnOrder.Num() &&
			   WizardEliminated.FindRef(TurnOrder[CurrentWizardIndex]))
		{
			CurrentWizardIndex++;
		}

		OnTurnBegin.Broadcast(CurrentTurnNumber);
	}
}

void UCoMTurnManager::ProcessFullTurn()
{
	UE_LOG(LogTemp, Log,
		TEXT("CoMTurnManager: ═══ Processing turn %d ═══"), CurrentTurnNumber);

	// Delegate to UCoMTurnSubsystem — the sole turn orchestrator.
	UGameInstance* GI = GetGameInstance();
	UCoMTurnSubsystem* TS = GI ? GI->GetSubsystem<UCoMTurnSubsystem>() : nullptr;

	if (TS)
	{
		TS->ProcessFullTurn();

		// Keep local turn counter in sync.
		CurrentTurnNumber = TS->GetCurrentTurn();
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CoMTurnManager::ProcessFullTurn — UCoMTurnSubsystem not available. "
			     "Falling back to local turn increment only."));
		CurrentTurnNumber++;
	}

	// Still run local victory / elimination checks for any TurnManager-specific listeners.
	CheckForEliminations();
	if (CheckVictory())
	{
		bGameOver = true;

		// ── Audio: victory fanfare ────────────────────────────────────────
		if (UCoMAudioSubsystem* Audio = GI ? GI->GetSubsystem<UCoMAudioSubsystem>() : nullptr)
		{
			Audio->StopAllMusic();
			Audio->PlayMusic(FName("Music_Victory"));
			Audio->PlayUISound(FName("SFX_UI_Victory"));
		}
	}

	OnTurnEnd.Broadcast(CurrentTurnNumber);

	UE_LOG(LogTemp, Log,
		TEXT("CoMTurnManager: ═══ Turn complete. Next turn: %d ═══"),
		CurrentTurnNumber);
}

// =====================================================================
// Queries
// =====================================================================

bool UCoMTurnManager::IsHumanTurn() const
{
	if (!TurnOrder.IsValidIndex(CurrentWizardIndex))
	{
		return false;
	}
	const int32 WizardId = TurnOrder[CurrentWizardIndex];
	const bool* bHuman = WizardIsHuman.Find(WizardId);
	return bHuman ? *bHuman : false;
}

int32 UCoMTurnManager::GetCurrentWizard() const
{
	if (TurnOrder.IsValidIndex(CurrentWizardIndex))
	{
		return TurnOrder[CurrentWizardIndex];
	}
	return -1;
}

int32 UCoMTurnManager::GetTurnNumber() const
{
	return CurrentTurnNumber;
}

bool UCoMTurnManager::IsGameOver() const
{
	return bGameOver;
}

int32 UCoMTurnManager::GetActiveWizardCount() const
{
	int32 Count = 0;
	for (const auto& Pair : WizardEliminated)
	{
		if (!Pair.Value)
		{
			Count++;
		}
	}
	return Count;
}

void UCoMTurnManager::EliminateWizard(int32 WizardId)
{
	if (bool* Found = WizardEliminated.Find(WizardId))
	{
		if (!*Found)
		{
			*Found = true;
			OnWizardEliminated.Broadcast(WizardId);
			UE_LOG(LogTemp, Log, TEXT("CoMTurnManager: Wizard %d eliminated!"), WizardId);
		}
	}
}

// =====================================================================
// Internal helpers
// =====================================================================

void UCoMTurnManager::RunPhase(ECoMTurnPhase Phase)
{
	CurrentPhase = Phase;
	OnPhaseBegin.Broadcast(Phase);
}

void UCoMTurnManager::CheckForEliminations()
{
	// A wizard with no cities AND no armies is eliminated.
	// For now this is a placeholder — we need city and army subsystems
	// to actually check ownership.  Log and skip gracefully.

	// TODO: For each non-eliminated wizard, query city/army subsystems.
	// If both counts are 0, call EliminateWizard(WizardId).

	// Check if only one wizard remains — that is also a game-over condition.
	if (GetActiveWizardCount() <= 1 && TurnOrder.Num() > 1)
	{
		bGameOver = true;
		UE_LOG(LogTemp, Log,
			TEXT("CoMTurnManager: Only %d wizard(s) remain — game over."),
			GetActiveWizardCount());
	}
}

bool UCoMTurnManager::CheckVictory() const
{
	// Delegate to UCoMVictorySubsystem for full victory evaluation.
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (const UCoMVictorySubsystem* VictorySub = GI->GetSubsystem<UCoMVictorySubsystem>())
		{
			return VictorySub->IsGameOver();
		}
	}

	// Fallback: victory if only one wizard remains.
	return GetActiveWizardCount() <= 1 && TurnOrder.Num() > 1;
}
