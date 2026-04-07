// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMTurnManager.h"

// Subsystem headers — resolved at call time, never cached.
#include "CoMSeasonSubsystem.h"
#include "CoMHeroSubsystem.h"
#include "CoMFogOfWarSubsystem.h"

// Subsystems that may not exist yet — forward-declared, null-checked.
// #include "CoMCombatSubsystem.h"
// #include "CoMSiegeSubsystem.h"
// #include "CoMNavalSubsystem.h"
// #include "CoMMagicSubsystem.h"
// #include "CoMDragonSubsystem.h"
// #include "CoMDiplomacySubsystem.h"
// #include "CoMEspionageSubsystem.h"
// #include "CoMQuestSubsystem.h"

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

	// ── Phase 0: Weather / Seasons ───────────────────────────────────
	RunPhase(ECoMTurnPhase::Movement); // Phase enum re-used; weather runs first.
	if (UCoMSeasonSubsystem* Seasons = GetGameInstance()->GetSubsystem<UCoMSeasonSubsystem>())
	{
		Seasons->AdvanceTurn();
		UE_LOG(LogTemp, Log, TEXT("  [Weather] Season subsystem ticked."));
	}

	// ── Phase 1: Movement ────────────────────────────────────────────
	// For each wizard: process queued movement commands.
	// Movement commands are stored externally (army subsystem / command queue).
	// Placeholder: iterate wizards and log.
	for (const int32 WizardId : TurnOrder)
	{
		if (WizardEliminated.FindRef(WizardId))
		{
			continue;
		}
		// TODO: process queued movement for WizardId via army/command subsystem.
	}
	UE_LOG(LogTemp, Log, TEXT("  [Movement] Queued movements processed."));

	// ── Phase 2: Combat ──────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Combat);
	// TODO: CombatSubsystem->DetectEncounters() + ResolveAllEncounters()
	UE_LOG(LogTemp, Log, TEXT("  [Combat] Placeholder — no CombatSubsystem yet."));

	// ── Phase 3: Siege ───────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Siege);
	// TODO: SiegeSubsystem->ProcessSiegeTurn()
	UE_LOG(LogTemp, Log, TEXT("  [Siege] Placeholder — no SiegeSubsystem yet."));

	// ── Phase 4: Naval ───────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Naval);
	// TODO: NavalSubsystem->ProcessNavalTurn()
	UE_LOG(LogTemp, Log, TEXT("  [Naval] Placeholder — no NavalSubsystem yet."));

	// ── Phase 5: Production ──────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Production);
	// TODO: CityProduction subsystem call
	UE_LOG(LogTemp, Log, TEXT("  [Production] Placeholder — no CityProduction yet."));

	// ── Phase 6: Magic ───────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Magic);
	// TODO: MagicSubsystem->ProcessTurn(CurrentTurnNumber)
	UE_LOG(LogTemp, Log, TEXT("  [Magic] Placeholder — no MagicSubsystem yet."));

	// ── Phase 7: Heroes ──────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Heroes);
	if (UCoMHeroSubsystem* Heroes = GetGameInstance()->GetSubsystem<UCoMHeroSubsystem>())
	{
		Heroes->ProcessHeroTurn();
		UE_LOG(LogTemp, Log, TEXT("  [Heroes] Hero subsystem ticked."));
	}

	// ── Phase 8: Dragons ─────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Dragons);
	// TODO: DragonSubsystem->ProcessDragonTurn()
	UE_LOG(LogTemp, Log, TEXT("  [Dragons] Placeholder — no DragonSubsystem yet."));

	// ── Phase 9: Diplomacy ───────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Diplomacy);
	// TODO: DiplomacySubsystem->ProcessTurn(CurrentTurnNumber)
	UE_LOG(LogTemp, Log, TEXT("  [Diplomacy] Placeholder — no DiplomacySubsystem yet."));

	// ── Phase 10: Espionage ──────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Espionage);
	// TODO: EspionageSubsystem->ProcessTurn(CurrentTurnNumber)
	UE_LOG(LogTemp, Log, TEXT("  [Espionage] Placeholder — no EspionageSubsystem yet."));

	// ── Phase 11: Quests ─────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Quests);
	// TODO: QuestSubsystem->ProcessTurn(CurrentTurnNumber)
	UE_LOG(LogTemp, Log, TEXT("  [Quests] Placeholder — no QuestSubsystem yet."));

	// ── Phase 12: Fog of War ─────────────────────────────────────────
	RunPhase(ECoMTurnPhase::FogOfWar);
	if (UCoMFogOfWarSubsystem* FoW = GetGameInstance()->GetSubsystem<UCoMFogOfWarSubsystem>())
	{
		FoW->UpdateAllVision();
		UE_LOG(LogTemp, Log, TEXT("  [FogOfWar] Vision updated for all wizards."));
	}

	// ── Phase 13: Victory ────────────────────────────────────────────
	RunPhase(ECoMTurnPhase::Victory);
	CheckForEliminations();
	if (CheckVictory())
	{
		bGameOver = true;
		UE_LOG(LogTemp, Log, TEXT("  [Victory] Game over condition met!"));
	}

	// ── Advance turn counter ─────────────────────────────────────────
	OnTurnEnd.Broadcast(CurrentTurnNumber);
	CurrentTurnNumber++;

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
	// Placeholder: victory if only one wizard remains.
	// Full implementation will check spell of mastery, conquest, etc.
	return GetActiveWizardCount() <= 1 && TurnOrder.Num() > 1;
}
