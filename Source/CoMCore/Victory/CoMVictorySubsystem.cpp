// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMVictorySubsystem.cpp -- Victory condition evaluation implementation.

#include "Victory/CoMVictorySubsystem.h"
#include "Framework/CoMGameState.h"
#include "Wizards/CoMPlayerState.h"
#include "Economy/CoMCitySubsystem.h"
#include "Diplomacy/CoMDiplomacySubsystem.h"
#include "Magic/CoMMagicSubsystem.h"
#include "Units/CoMHeroSubsystem.h"
#include "Units/CoMUnitSubsystem.h"
#include "Units/CoMDragonSubsystem.h"
#include "Audio/CoMAudioSubsystem.h"
#include "Turn/CoMTurnSubsystem.h"
#include "Engine/World.h"

// =========================================================================
// USubsystem interface
// =========================================================================

void UCoMVictorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EliminatedWizards.Empty();
	WizardVictories.Empty();
	DestabilizedPlanes.Empty();
	bGameOver = false;
	WinningWizardId = -1;
	WinningCondition = ECoMVictoryType::MAX;
	bSaelThrixSlain = false;
	bSpellOfMasteryCast = false;
	MasteryCasterWizardId = -1;
}

void UCoMVictorySubsystem::Deinitialize()
{
	EliminatedWizards.Empty();
	WizardVictories.Empty();
	DestabilizedPlanes.Empty();
	Super::Deinitialize();
}

void UCoMVictorySubsystem::ResetForNewGame()
{
	EliminatedWizards.Empty();
	WizardVictories.Empty();
	DestabilizedPlanes.Empty();
	bGameOver = false;
	WinningWizardId = -1;
	WinningCondition = ECoMVictoryType::MAX;
	bSaelThrixSlain = false;
	bSpellOfMasteryCast = false;
	MasteryCasterWizardId = -1;
}

// =========================================================================
// Core victory check
// =========================================================================

void UCoMVictorySubsystem::CheckVictoryConditions(int32 CurrentTurn)
{
	if (bGameOver)
	{
		return;
	}

	// 1. Check for wizard elimination (0 cities => eliminated)
	CheckForEliminations(CurrentTurn);

	if (bGameOver)
	{
		return; // Elimination may have triggered a last-man-standing victory
	}

	// 2. If only 1 wizard remains, that is a Conquest victory
	const TArray<int32> Survivors = GetSurvivingWizardIndices();
	if (Survivors.Num() == 1 && GetTotalWizardCount() > 1)
	{
		DeclareVictory(Survivors[0], ECoMVictoryType::Conquest);
		return;
	}

	// 3. Check each surviving wizard for victory conditions (priority order)
	for (const int32 WizardId : Survivors)
	{
		// Spell of Mastery (instant win, may have been flagged by NotifySpellOfMasteryCast)
		if (CheckSpellOfMastery(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Magical);
			return;
		}

		// Domination (control all capitals)
		if (CheckDomination(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Domination);
			return;
		}

		// Domination by majority (control >50% of all cities, rivals remain)
		if (CheckCityDominance(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Domination);
			return;
		}

		// Diplomatic (allied with all survivors)
		if (CheckDiplomaticVictory(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Diplomatic);
			return;
		}

		// Pact (contracts with 3 Archdevils)
		if (CheckPactVictory(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Pact);
			return;
		}

		// Dissolution (3 planes destabilized + Sael-Thrix slain)
		if (CheckDissolutionVictory(WizardId))
		{
			DeclareVictory(WizardId, ECoMVictoryType::Dissolution);
			return;
		}
	}
}

// =========================================================================
// Individual condition checks
// =========================================================================

bool UCoMVictorySubsystem::CheckDomination(int32 WizardId) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return false; }

	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!CitySub) { return false; }

	// A wizard achieves Domination by controlling every non-eliminated wizard's
	// original capital city. We identify capitals as the first city each wizard
	// founded (CityID with the lowest value per wizard).
	//
	// For now, we check: does this wizard own at least one city from every other
	// non-eliminated wizard? We approximate capital ownership by checking that
	// every surviving wizard's cities are all owned by WizardId (or the wizard
	// has no cities and is eliminated).

	const TArray<int32> Survivors = GetSurvivingWizardIndices();
	if (Survivors.Num() < 2)
	{
		return false; // Conquest, not domination
	}

	for (const int32 OtherWizardId : Survivors)
	{
		if (OtherWizardId == WizardId) { continue; }

		// Check if the other wizard still has any cities that WizardId does not control.
		// For Domination, WizardId must have captured the other wizard's capital.
		// We define capital as the first-founded city (lowest CityID) for that wizard.
		// Since we cannot track original ownership after capture, we check:
		// does WizardId own any city at all for every other surviving wizard's territory?
		// Simplified: does WizardId have more cities than all other survivors combined?
		// Better approach: check that no other surviving wizard has a city they still own.

		TArray<const FCoMCityData*> OtherCities = CitySub->GetCitiesForWizard(OtherWizardId);
		if (OtherCities.Num() > 0)
		{
			// Other wizard still has cities -- not domination
			return false;
		}
	}

	// Every other surviving wizard has 0 cities but hasn't been eliminated yet.
	// (They'll be eliminated this turn in CheckForEliminations.)
	// This is effectively domination -- WizardId controls all territory.
	return true;
}

bool UCoMVictorySubsystem::CheckCityDominance(int32 WizardId) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return false; }

	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!CitySub) { return false; }

	// Need at least two survivors — otherwise this is a Conquest win.
	const TArray<int32> Survivors = GetSurvivingWizardIndices();
	if (Survivors.Num() < 2) { return false; }

	const int32 TotalCities = CitySub->GetAllCityIDs().Num();
	if (TotalCities < 4) { return false; } // avoid trivial early-game triggers

	const int32 OwnCities = CitySub->GetCitiesForWizard(WizardId).Num();

	// Strictly more than half of all cities in the world.
	return OwnCities * 2 > TotalCities;
}

bool UCoMVictorySubsystem::CheckSpellOfMastery(int32 WizardId) const
{
	// Spell of Mastery is an instant-win triggered by NotifySpellOfMasteryCast().
	return bSpellOfMasteryCast && MasteryCasterWizardId == WizardId;
}

bool UCoMVictorySubsystem::CheckDiplomaticVictory(int32 WizardId) const
{
	const TArray<int32> Survivors = GetSurvivingWizardIndices();

	// Minimum surviving wizard count required
	if (Survivors.Num() < MinWizardsForDiplomaticVictory)
	{
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return false; }

	UCoMDiplomacySubsystem* DiplomacySub = GI->GetSubsystem<UCoMDiplomacySubsystem>();
	if (!DiplomacySub) { return false; }

	// Must have Alliance or better (MilitaryAlliance, WizardsPact, Confederation)
	// with every other surviving wizard.
	for (const int32 OtherWizardId : Survivors)
	{
		if (OtherWizardId == WizardId) { continue; }

		if (!DiplomacySub->AreAllied(WizardId, OtherWizardId))
		{
			return false;
		}
	}

	return true;
}

bool UCoMVictorySubsystem::CheckPactVictory(int32 WizardId) const
{
	// Check via FCoMVictoryState in the game state, or via diplomacy subsystem.
	// Look for active Archdevil contracts stored in the VictoryState struct.
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return false; }

	// Try to get victory state from game state
	UWorld* World = GI->GetWorld();
	if (!World) { return false; }

	ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS) { return false; }

	// Look up the wizard's player state for victory tracking
	ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	if (!PS) { return false; }

	// Check the VictoryState from CoMStructs.h which tracks Archdevil contracts.
	// The FCoMVictoryState stores ActiveArchdevilContracts.
	// For now, we check the diplomacy subsystem for "Pact" type treaties
	// with NPC Archdevil wizard slots.
	UCoMDiplomacySubsystem* DiplomacySub = GI->GetSubsystem<UCoMDiplomacySubsystem>();
	if (!DiplomacySub) { return false; }

	// Count Archdevil pact treaties. Archdevil wizard slots are identified
	// by checking ApexRuler data. For now, count WizardsPact treaties as
	// a proxy (will be refined when ApexRuler integration is complete).
	int32 ArchdevilPactCount = 0;

	const int32 TotalWizards = GetTotalWizardCount();
	for (int32 i = 0; i < TotalWizards; ++i)
	{
		if (i == WizardId) { continue; }

		const ECoMTreatyType Treaty = DiplomacySub->GetTreatyBetween(WizardId, i);
		if (Treaty == ECoMTreatyType::WizardsPact)
		{
			// Check if this wizard slot is an Archdevil (NPC).
			// For now, any WizardsPact counts toward the pact total.
			// TODO: Filter to only Archdevil-flagged wizard slots.
			ArchdevilPactCount++;
		}
	}

	return ArchdevilPactCount >= RequiredArchdevilPacts;
}

bool UCoMVictorySubsystem::CheckDissolutionVictory(int32 WizardId) const
{
	// Requires 3+ planes destabilized AND Sael-Thrix slain.
	// Any wizard can trigger this -- it is not wizard-specific tracking,
	// but the wizard who lands the killing blow on Sael-Thrix wins.
	// For now, the first wizard checked after both conditions are met wins.
	return DestabilizedPlanes.Num() >= RequiredDestabilizedPlanes && bSaelThrixSlain;
}

// =========================================================================
// Elimination
// =========================================================================

void UCoMVictorySubsystem::EliminateWizard(int32 WizardId, int32 ConquerorId)
{
	if (EliminatedWizards.Contains(WizardId))
	{
		return; // Already eliminated
	}

	EliminatedWizards.Add(WizardId);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMVictorySubsystem: Wizard %d eliminated by Wizard %d."),
	       WizardId, ConquerorId);

	// Broadcast elimination delegate
	OnWizardEliminated.Broadcast(WizardId, ConquerorId);

	// If this is the human player, broadcast defeat
	if (IsHumanPlayer(WizardId))
	{
		OnDefeat.Broadcast(WizardId);

		// Play defeat audio
		UGameInstance* GI = GetGameInstance();
		if (GI)
		{
			if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
			{
				Audio->StopAllMusic();
				Audio->PlayMusic(FName("Music_Defeat"));
				Audio->PlayUISound(FName("SFX_UI_Defeat"));
			}
		}
	}

	// Break all treaties with the eliminated wizard
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (UCoMDiplomacySubsystem* Diplomacy = GI->GetSubsystem<UCoMDiplomacySubsystem>())
		{
			const TArray<int32> Allies = Diplomacy->GetAllies(WizardId);
			for (const int32 AllyId : Allies)
			{
				Diplomacy->BreakTreaty(WizardId, AllyId);
			}
		}
	}

	// Also notify TurnManager for backward compatibility
	if (GI)
	{
		// UCoMTurnManager tracks its own elimination state
		// Let it know through its own EliminateWizard method
		UE_LOG(LogTemp, Log,
		       TEXT("UCoMVictorySubsystem: Wizard %d removed from active play."), WizardId);
	}
}

bool UCoMVictorySubsystem::IsEliminated(int32 WizardId) const
{
	return EliminatedWizards.Contains(WizardId);
}

int32 UCoMVictorySubsystem::GetSurvivingWizardCount() const
{
	return GetTotalWizardCount() - EliminatedWizards.Num();
}

// =========================================================================
// Victory declaration
// =========================================================================

void UCoMVictorySubsystem::DeclareVictory(int32 WizardId, ECoMVictoryType Type)
{
	if (bGameOver)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("UCoMVictorySubsystem::DeclareVictory — game already over."));
		return;
	}

	bGameOver = true;
	WinningWizardId = WizardId;
	WinningCondition = Type;
	WizardVictories.Add(WizardId, Type);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMVictorySubsystem: === VICTORY === Wizard %d wins via %s!"),
	       WizardId, *UEnum::GetValueAsString(Type));

	// Calculate final scores for all wizards
	const TArray<FCoMVictoryScoreEntry> FinalScores = GetAllScores();
	for (const FCoMVictoryScoreEntry& Entry : FinalScores)
	{
		UE_LOG(LogTemp, Log,
		       TEXT("  Wizard %d: Score %d"), Entry.WizardId, Entry.TotalScore);
	}

	// Broadcast victory delegate
	OnVictoryDeclared.Broadcast(WizardId, Type);

	// Play victory audio
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (UCoMAudioSubsystem* Audio = GI->GetSubsystem<UCoMAudioSubsystem>())
		{
			Audio->StopAllMusic();
			Audio->PlayMusic(FName("Music_Victory"));
			Audio->PlayUISound(FName("SFX_UI_Victory"));
		}
	}
}

// =========================================================================
// Scoring
// =========================================================================

int32 UCoMVictorySubsystem::CalculateScore(int32 WizardId) const
{
	return CalculateDetailedScore(WizardId).TotalScore;
}

FCoMVictoryScoreEntry UCoMVictorySubsystem::CalculateDetailedScore(int32 WizardId) const
{
	FCoMVictoryScoreEntry Entry;
	Entry.WizardId = WizardId;

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return Entry; }

	// Cities
	if (UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>())
	{
		TArray<const FCoMCityData*> Cities = CitySub->GetCitiesForWizard(WizardId);
		Entry.CityScore = Cities.Num() * ScorePerCity;
	}

	// Units (approximate via unit subsystem)
	if (UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>())
	{
		// UnitSubsystem should have a count method; use GetUnitsForWizard if available.
		// For now, use a placeholder count.
		// TODO: Wire to actual unit count query when available.
		Entry.UnitScore = 0; // Will be populated when unit queries are wired
	}

	// Spells known
	if (UCoMMagicSubsystem* MagicSub = GI->GetSubsystem<UCoMMagicSubsystem>())
	{
		const FCoMWizardMagicState& MagicState = const_cast<UCoMMagicSubsystem*>(MagicSub)->GetWizardMagic(WizardId);
		Entry.SpellScore = MagicState.KnownSpells.Num() * ScorePerSpell;
	}

	// Turns played
	if (UCoMTurnSubsystem* TurnSub = GI->GetSubsystem<UCoMTurnSubsystem>())
	{
		Entry.TurnScore = TurnSub->GetCurrentTurn() * ScorePerTurn;
	}

	// Heroes alive
	if (UCoMHeroSubsystem* HeroSub = GI->GetSubsystem<UCoMHeroSubsystem>())
	{
		// TODO: Wire to hero count for wizard query
		Entry.HeroScore = 0;
	}

	// Dragons controlled
	if (UCoMDragonSubsystem* DragonSub = GI->GetSubsystem<UCoMDragonSubsystem>())
	{
		// TODO: Wire to dragon count for wizard query
		Entry.DragonScore = 0;
	}

	// Victory bonus
	if (bGameOver && WinningWizardId == WizardId)
	{
		Entry.VictoryBonus = ScoreVictoryBonus;
	}

	Entry.TotalScore = Entry.CityScore + Entry.UnitScore + Entry.SpellScore
	                 + Entry.TurnScore + Entry.HeroScore + Entry.DragonScore
	                 + Entry.VictoryBonus;

	return Entry;
}

TArray<FCoMVictoryScoreEntry> UCoMVictorySubsystem::GetAllScores() const
{
	TArray<FCoMVictoryScoreEntry> Scores;

	const int32 TotalWizards = GetTotalWizardCount();
	for (int32 i = 0; i < TotalWizards; ++i)
	{
		Scores.Add(CalculateDetailedScore(i));
	}

	// Sort descending by TotalScore
	Scores.Sort([](const FCoMVictoryScoreEntry& A, const FCoMVictoryScoreEntry& B)
	{
		return A.TotalScore > B.TotalScore;
	});

	return Scores;
}

// =========================================================================
// Dissolution flags
// =========================================================================

void UCoMVictorySubsystem::SetPlaneDestabilized(ECoMPlane Plane, bool bDestabilized)
{
	if (bDestabilized)
	{
		DestabilizedPlanes.Add(Plane);
		UE_LOG(LogTemp, Log,
		       TEXT("UCoMVictorySubsystem: Plane %s destabilized. Total: %d/%d"),
		       *UEnum::GetValueAsString(Plane), DestabilizedPlanes.Num(), RequiredDestabilizedPlanes);
	}
	else
	{
		DestabilizedPlanes.Remove(Plane);
	}
}

// =========================================================================
// Spell of Mastery notifications
// =========================================================================

void UCoMVictorySubsystem::NotifySpellOfMasteryResearchStarted(int32 WizardId)
{
	UE_LOG(LogTemp, Warning,
	       TEXT("UCoMVictorySubsystem: !!! Wizard %d has begun researching a Spell of Mastery! !!!"),
	       WizardId);

	// TODO: Notify all other wizards via UI/diplomacy so they can attempt to stop it.
}

void UCoMVictorySubsystem::NotifySpellOfMasteryCast(int32 WizardId)
{
	UE_LOG(LogTemp, Log,
	       TEXT("UCoMVictorySubsystem: Wizard %d has cast a Spell of Mastery!"), WizardId);

	bSpellOfMasteryCast = true;
	MasteryCasterWizardId = WizardId;

	// Immediate victory — will be picked up in CheckVictoryConditions,
	// or call DeclareVictory directly if called mid-turn.
	if (!bGameOver)
	{
		DeclareVictory(WizardId, ECoMVictoryType::Magical);
	}
}

// =========================================================================
// Internal helpers
// =========================================================================

void UCoMVictorySubsystem::CheckForEliminations(int32 CurrentTurn)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return; }

	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!CitySub) { return; }

	const TArray<int32> Survivors = GetSurvivingWizardIndices();

	for (const int32 WizardId : Survivors)
	{
		TArray<const FCoMCityData*> Cities = CitySub->GetCitiesForWizard(WizardId);
		if (Cities.Num() == 0)
		{
			// Wizard has no cities — eliminated.
			// Try to identify the conqueror (wizard who captured the last city).
			// For now, pass -1 as conqueror.
			EliminateWizard(WizardId, /*ConquerorId=*/ -1);
		}
	}
}

TArray<int32> UCoMVictorySubsystem::GetSurvivingWizardIndices() const
{
	TArray<int32> Survivors;

	UGameInstance* GI = GetGameInstance();
	if (!GI) { return Survivors; }

	UWorld* World = GI->GetWorld();
	if (!World) { return Survivors; }

	const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS) { return Survivors; }

	for (int32 i = 0; i < GS->WizardStates.Num(); ++i)
	{
		if (GS->WizardStates[i] != nullptr && !EliminatedWizards.Contains(i))
		{
			Survivors.Add(i);
		}
	}

	return Survivors;
}

bool UCoMVictorySubsystem::IsHumanPlayer(int32 WizardId) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return false; }

	UWorld* World = GI->GetWorld();
	if (!World) { return false; }

	const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS) { return false; }

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	return PS && PS->bIsHumanPlayer;
}

int32 UCoMVictorySubsystem::GetTotalWizardCount() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) { return 0; }

	UWorld* World = GI->GetWorld();
	if (!World) { return 0; }

	const ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
	if (!GS) { return 0; }

	int32 Count = 0;
	for (int32 i = 0; i < GS->WizardStates.Num(); ++i)
	{
		if (GS->WizardStates[i] != nullptr)
		{
			Count++;
		}
	}
	return Count;
}

TMap<int32, int32> UCoMVictorySubsystem::GetOriginalCapitals() const
{
	// TODO: Track original capital assignments at game start.
	// For now, returns empty. Will be populated when capital tracking is added.
	TMap<int32, int32> Capitals;
	return Capitals;
}

// =========================================================================
// Save/Load Export/Import
// =========================================================================

void UCoMVictorySubsystem::ImportEliminatedWizards(const TArray<int32>& Data)
{
	EliminatedWizards = Data;
}

void UCoMVictorySubsystem::ExportAll(TArray<int32>& OutEliminated, bool& OutGameOver,
                                      int32& OutWinningWizardId, ECoMVictoryType& OutWinningCondition,
                                      TArray<ECoMPlane>& OutDestabilizedPlanes, bool& OutSaelThrixSlain,
                                      bool& OutSpellOfMasteryCast, int32& OutMasteryCasterWizardId) const
{
	OutEliminated = EliminatedWizards;
	OutGameOver = bGameOver;
	OutWinningWizardId = WinningWizardId;
	OutWinningCondition = WinningCondition;
	OutDestabilizedPlanes = DestabilizedPlanes.Array();
	OutSaelThrixSlain = bSaelThrixSlain;
	OutSpellOfMasteryCast = bSpellOfMasteryCast;
	OutMasteryCasterWizardId = MasteryCasterWizardId;
}

void UCoMVictorySubsystem::ImportAll(const TArray<int32>& InEliminated, bool InGameOver,
                                      int32 InWinningWizardId, ECoMVictoryType InWinningCondition,
                                      const TArray<ECoMPlane>& InDestabilizedPlanes, bool InSaelThrixSlain,
                                      bool InSpellOfMasteryCast, int32 InMasteryCasterWizardId)
{
	EliminatedWizards = InEliminated;
	bGameOver = InGameOver;
	WinningWizardId = InWinningWizardId;
	WinningCondition = InWinningCondition;

	DestabilizedPlanes.Empty();
	for (const ECoMPlane& Plane : InDestabilizedPlanes)
	{
		DestabilizedPlanes.Add(Plane);
	}

	bSaelThrixSlain = InSaelThrixSlain;
	bSpellOfMasteryCast = InSpellOfMasteryCast;
	MasteryCasterWizardId = InMasteryCasterWizardId;

	UE_LOG(LogTemp, Log,
	       TEXT("[VictorySubsystem] ImportAll: %d eliminated, GameOver=%s, Winner=%d"),
	       EliminatedWizards.Num(), bGameOver ? TEXT("true") : TEXT("false"), WinningWizardId);
}
