// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMStatsSubsystem.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

#include "CoMCore/Combat/CoMCombatSubsystem.h"
#include "CoMCore/World/CoMSiteEncounterSubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"

namespace
{
	FString StatsPath()
	{
		return FPaths::ProjectSavedDir() / TEXT("com_stats.json");
	}
}

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void UCoMStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterDefaultAchievements();
	EnsureArraySizes();
	LoadFromDisk();
	BindToSubsystems();
}

void UCoMStatsSubsystem::BindToSubsystems()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	if (UCoMCombatSubsystem* Cb = GI->GetSubsystem<UCoMCombatSubsystem>())
	{
		Cb->OnCombatResolved.AddDynamic(this, &UCoMStatsSubsystem::HandleCombatResolved);
	}
	if (UCoMSiteEncounterSubsystem* Sites = GI->GetSubsystem<UCoMSiteEncounterSubsystem>())
	{
		Sites->OnSiteCleared.AddDynamic(this, &UCoMStatsSubsystem::HandleSiteCleared);
	}
	if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
	{
		Items->OnItemForged.AddDynamic(this, &UCoMStatsSubsystem::HandleItemForged);
	}
	if (UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>())
	{
		Magic->OnSpellResolved.AddDynamic(this, &UCoMStatsSubsystem::HandleSpellResolved);
	}
	if (UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>())
	{
		Cities->OnCityCaptured.AddDynamic(this, &UCoMStatsSubsystem::HandleCityCaptured);
	}
	if (UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>())
	{
		Heroes->OnHeroAccepted.AddDynamic(this, &UCoMStatsSubsystem::HandleHeroAccepted);
	}
}

void UCoMStatsSubsystem::HandleCombatResolved(const FCoMCombatResult& Result)
{
	Stats.BattlesFought++;
	Stats.UnitsKilled += Result.AttackerCasualties.Num() + Result.DefenderCasualties.Num();
}

void UCoMStatsSubsystem::HandleSiteCleared(int32 /*SiteID*/, int32 /*WizardIndex*/,
	int32 /*GoldReward*/, int32 /*ManaReward*/)
{
	Stats.SitesCleared++;
}

void UCoMStatsSubsystem::HandleItemForged(int32 /*InstanceID*/)
{
	Stats.ItemsForged++;
}

void UCoMStatsSubsystem::HandleSpellResolved(int32 /*CasterWizardId*/, FName /*SpellID*/, int32 ManaCost)
{
	Stats.SpellsCast++;
	Stats.ManaSpent += FMath::Max(0, ManaCost);
}

void UCoMStatsSubsystem::HandleCityCaptured(int32 /*CityID*/, int32 /*FormerOwner*/, int32 /*NewOwner*/)
{
	Stats.CitiesCaptured++;
}

void UCoMStatsSubsystem::HandleHeroAccepted(int32 /*HeroUnitID*/, int32 /*OwnerWizardIndex*/)
{
	Stats.HeroesRecruited++;
}

void UCoMStatsSubsystem::Deinitialize()
{
	SaveToDisk();
	Super::Deinitialize();
}

void UCoMStatsSubsystem::EnsureArraySizes()
{
	const int32 V = static_cast<int32>(ECoMVictoryType::MAX);
	if (Stats.WinsByVictoryType.Num() < V) Stats.WinsByVictoryType.SetNumZeroed(V);
	if (Stats.WinsByWizardSlot.Num() < 14) Stats.WinsByWizardSlot.SetNumZeroed(14);
}

// ─── Achievement table ───────────────────────────────────────────────────────

void UCoMStatsSubsystem::RegisterDefaultAchievements()
{
	if (Achievements.Num() > 0) return;
	auto Add = [&](const TCHAR* ID, const TCHAR* Name, const TCHAR* Desc)
	{
		FCoMAchievement A;
		A.AchievementID = FName(ID);
		A.DisplayName   = Name;
		A.Description   = Desc;
		Achievements.Add(A);
	};
	Add(TEXT("first_blood"),        TEXT("First Blood"),
		TEXT("Win your first game on any difficulty."));
	Add(TEXT("spell_of_mastery"),   TEXT("Cosmic Master"),
		TEXT("Win by casting the Spell of Mastery."));
	Add(TEXT("conqueror"),          TEXT("Conqueror"),
		TEXT("Win by Domination -- own a majority of all cities."));
	Add(TEXT("speed_demon"),        TEXT("Speed Demon"),
		TEXT("Win a game in 150 turns or fewer."));
	Add(TEXT("the_long_haul"),      TEXT("The Long Haul"),
		TEXT("Survive a 400+ turn game."));
	Add(TEXT("battle_hardened"),    TEXT("Battle-Hardened"),
		TEXT("Fight 100 battles over your career."));
	Add(TEXT("siege_master"),       TEXT("Siege Master"),
		TEXT("Capture 25 enemy cities over your career."));
	Add(TEXT("archmagi"),           TEXT("Archmagi"),
		TEXT("Cast 1000 spells over your career."));
	Add(TEXT("artificer"),          TEXT("Artificer"),
		TEXT("Forge 25 artifact items."));
	Add(TEXT("tavern_legend"),      TEXT("Tavern Legend"),
		TEXT("Recruit 25 heroes."));
	Add(TEXT("treasure_hunter"),    TEXT("Treasure Hunter"),
		TEXT("Clear 50 lairs / ruins / towers."));
	Add(TEXT("five_for_five"),      TEXT("Five for Five"),
		TEXT("Win 5 games."));
	Add(TEXT("merlin"),             TEXT("As Merlin Foretold"),
		TEXT("Win a game playing as wizard slot 0 (Merlin)."));
	Add(TEXT("hall_of_fame"),       TEXT("Hall of Fame"),
		TEXT("Win as 5 different wizard slots."));
}

// ─── Recorders ───────────────────────────────────────────────────────────────

void UCoMStatsSubsystem::RecordGameEnd(int32 WinnerWizardSlot, int32 PlayerWizardSlot,
	ECoMVictoryType VictoryType, int32 TurnsPlayed)
{
	EnsureArraySizes();
	Stats.GamesPlayed++;
	Stats.LongestGameTurns = FMath::Max(Stats.LongestGameTurns, TurnsPlayed);

	const bool bPlayerWon = (WinnerWizardSlot == PlayerWizardSlot) && (WinnerWizardSlot >= 0);
	if (bPlayerWon)
	{
		Stats.GamesWon++;
		const int32 V = static_cast<int32>(VictoryType);
		if (Stats.WinsByVictoryType.IsValidIndex(V)) Stats.WinsByVictoryType[V]++;
		if (Stats.WinsByWizardSlot.IsValidIndex(PlayerWizardSlot))
			Stats.WinsByWizardSlot[PlayerWizardSlot]++;
		if (Stats.FastestWinTurns < 0 || TurnsPlayed < Stats.FastestWinTurns)
		{
			Stats.FastestWinTurns = TurnsPlayed;
		}
	}
	else
	{
		Stats.GamesLost++;
	}

	EvaluateAchievements();
	SaveToDisk();
}

void UCoMStatsSubsystem::RecordBattleFought()   { Stats.BattlesFought++; }
void UCoMStatsSubsystem::RecordUnitsKilled(int32 N) { Stats.UnitsKilled += FMath::Max(0, N); }
void UCoMStatsSubsystem::RecordCityCaptured()   { Stats.CitiesCaptured++; }
void UCoMStatsSubsystem::RecordSpellCast(int32 ManaCost)
{
	Stats.SpellsCast++;
	Stats.ManaSpent += FMath::Max(0, ManaCost);
}
void UCoMStatsSubsystem::RecordSiteCleared()    { Stats.SitesCleared++; }
void UCoMStatsSubsystem::RecordItemForged()     { Stats.ItemsForged++; }
void UCoMStatsSubsystem::RecordHeroRecruited()  { Stats.HeroesRecruited++; }

// ─── Achievement evaluation ──────────────────────────────────────────────────

void UCoMStatsSubsystem::EvaluateAchievements()
{
	auto Unlock = [&](const TCHAR* ID, bool bCondition)
	{
		if (!bCondition) return;
		for (FCoMAchievement& A : Achievements)
		{
			if (A.AchievementID == FName(ID) && !A.bUnlocked)
			{
				A.bUnlocked = true;
				A.UnlockedAtGame = Stats.GamesPlayed;
				UE_LOG(LogTemp, Log, TEXT("Achievement unlocked: %s -- %s"),
					*A.DisplayName, *A.Description);
			}
		}
	};

	Unlock(TEXT("first_blood"),     Stats.GamesWon >= 1);
	Unlock(TEXT("five_for_five"),   Stats.GamesWon >= 5);
	Unlock(TEXT("speed_demon"),
		Stats.FastestWinTurns >= 0 && Stats.FastestWinTurns <= 150);
	Unlock(TEXT("the_long_haul"),   Stats.LongestGameTurns >= 400);
	Unlock(TEXT("battle_hardened"), Stats.BattlesFought >= 100);
	Unlock(TEXT("siege_master"),    Stats.CitiesCaptured >= 25);
	Unlock(TEXT("archmagi"),        Stats.SpellsCast >= 1000);
	Unlock(TEXT("artificer"),       Stats.ItemsForged >= 25);
	Unlock(TEXT("tavern_legend"),   Stats.HeroesRecruited >= 25);
	Unlock(TEXT("treasure_hunter"), Stats.SitesCleared >= 50);

	const int32 MagicalIdx     = static_cast<int32>(ECoMVictoryType::Magical);
	const int32 DominationIdx  = static_cast<int32>(ECoMVictoryType::Domination);
	Unlock(TEXT("spell_of_mastery"),
		Stats.WinsByVictoryType.IsValidIndex(MagicalIdx) && Stats.WinsByVictoryType[MagicalIdx] >= 1);
	Unlock(TEXT("conqueror"),
		Stats.WinsByVictoryType.IsValidIndex(DominationIdx) && Stats.WinsByVictoryType[DominationIdx] >= 1);
	Unlock(TEXT("merlin"),
		Stats.WinsByWizardSlot.IsValidIndex(0) && Stats.WinsByWizardSlot[0] >= 1);

	int32 DistinctSlotWins = 0;
	for (int32 N : Stats.WinsByWizardSlot) if (N >= 1) ++DistinctSlotWins;
	Unlock(TEXT("hall_of_fame"), DistinctSlotWins >= 5);
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void UCoMStatsSubsystem::SaveToDisk()
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("games_played"),     Stats.GamesPlayed);
	Root->SetNumberField(TEXT("games_won"),        Stats.GamesWon);
	Root->SetNumberField(TEXT("games_lost"),       Stats.GamesLost);
	Root->SetNumberField(TEXT("fastest_win"),      Stats.FastestWinTurns);
	Root->SetNumberField(TEXT("longest_game"),     Stats.LongestGameTurns);
	Root->SetNumberField(TEXT("battles_fought"),   Stats.BattlesFought);
	Root->SetNumberField(TEXT("units_killed"),     Stats.UnitsKilled);
	Root->SetNumberField(TEXT("cities_captured"),  Stats.CitiesCaptured);
	Root->SetNumberField(TEXT("spells_cast"),      Stats.SpellsCast);
	Root->SetNumberField(TEXT("mana_spent"),       Stats.ManaSpent);
	Root->SetNumberField(TEXT("sites_cleared"),    Stats.SitesCleared);
	Root->SetNumberField(TEXT("items_forged"),     Stats.ItemsForged);
	Root->SetNumberField(TEXT("heroes_recruited"), Stats.HeroesRecruited);

	TArray<TSharedPtr<FJsonValue>> Vict;
	for (int32 N : Stats.WinsByVictoryType) Vict.Add(MakeShared<FJsonValueNumber>(N));
	Root->SetArrayField(TEXT("wins_by_victory_type"), Vict);
	TArray<TSharedPtr<FJsonValue>> Slot;
	for (int32 N : Stats.WinsByWizardSlot) Slot.Add(MakeShared<FJsonValueNumber>(N));
	Root->SetArrayField(TEXT("wins_by_wizard_slot"), Slot);

	TArray<TSharedPtr<FJsonValue>> Ach;
	for (const FCoMAchievement& A : Achievements)
	{
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("id"),               A.AchievementID.ToString());
		O->SetStringField(TEXT("name"),             A.DisplayName);
		O->SetStringField(TEXT("description"),      A.Description);
		O->SetBoolField(  TEXT("unlocked"),         A.bUnlocked);
		O->SetNumberField(TEXT("unlocked_at_game"), A.UnlockedAtGame);
		Ach.Add(MakeShared<FJsonValueObject>(O));
	}
	Root->SetArrayField(TEXT("achievements"), Ach);

	FString Out;
	const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, W);
	FFileHelper::SaveStringToFile(Out, *StatsPath());
}

void UCoMStatsSubsystem::LoadFromDisk()
{
	if (bLoaded) return;
	bLoaded = true;
	FString In;
	if (!FFileHelper::LoadFileToString(In, *StatsPath())) return;
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(In);
	if (!FJsonSerializer::Deserialize(R, Root) || !Root.IsValid()) return;

	auto N = [&](const TCHAR* K, int32& Out) {
		int32 V = 0; if (Root->TryGetNumberField(K, V)) Out = V;
	};
	N(TEXT("games_played"),     Stats.GamesPlayed);
	N(TEXT("games_won"),        Stats.GamesWon);
	N(TEXT("games_lost"),       Stats.GamesLost);
	N(TEXT("fastest_win"),      Stats.FastestWinTurns);
	N(TEXT("longest_game"),     Stats.LongestGameTurns);
	N(TEXT("battles_fought"),   Stats.BattlesFought);
	N(TEXT("units_killed"),     Stats.UnitsKilled);
	N(TEXT("cities_captured"),  Stats.CitiesCaptured);
	N(TEXT("spells_cast"),      Stats.SpellsCast);
	N(TEXT("mana_spent"),       Stats.ManaSpent);
	N(TEXT("sites_cleared"),    Stats.SitesCleared);
	N(TEXT("items_forged"),     Stats.ItemsForged);
	N(TEXT("heroes_recruited"), Stats.HeroesRecruited);

	const TArray<TSharedPtr<FJsonValue>>* Vict = nullptr;
	if (Root->TryGetArrayField(TEXT("wins_by_victory_type"), Vict) && Vict)
	{
		Stats.WinsByVictoryType.Reset();
		for (const auto& V : *Vict) Stats.WinsByVictoryType.Add(static_cast<int32>(V->AsNumber()));
	}
	const TArray<TSharedPtr<FJsonValue>>* Slot = nullptr;
	if (Root->TryGetArrayField(TEXT("wins_by_wizard_slot"), Slot) && Slot)
	{
		Stats.WinsByWizardSlot.Reset();
		for (const auto& V : *Slot) Stats.WinsByWizardSlot.Add(static_cast<int32>(V->AsNumber()));
	}
	EnsureArraySizes();

	const TArray<TSharedPtr<FJsonValue>>* Ach = nullptr;
	if (Root->TryGetArrayField(TEXT("achievements"), Ach) && Ach)
	{
		for (const auto& V : *Ach)
		{
			const TSharedPtr<FJsonObject>& O = V->AsObject();
			if (!O.IsValid()) continue;
			FString IDStr; O->TryGetStringField(TEXT("id"), IDStr);
			bool   bU = false; O->TryGetBoolField(TEXT("unlocked"), bU);
			int32  At = -1;
			int32  AtTmp = 0; if (O->TryGetNumberField(TEXT("unlocked_at_game"), AtTmp)) At = AtTmp;
			for (FCoMAchievement& A : Achievements)
			{
				if (A.AchievementID == FName(*IDStr))
				{
					A.bUnlocked = bU;
					A.UnlockedAtGame = At;
					break;
				}
			}
		}
	}
}

void UCoMStatsSubsystem::ResetAll()
{
	Stats.Reset();
	for (FCoMAchievement& A : Achievements) { A.bUnlocked = false; A.UnlockedAtGame = -1; }
	EnsureArraySizes();
	SaveToDisk();
}

// ─── Console commands ────────────────────────────────────────────────────────

static FAutoConsoleCommandWithWorldAndArgs GShowStatsCmd(
	TEXT("com.show_stats"),
	TEXT("Print career stats + achievement progress to the log."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			if (!GI && GEngine)
			{
				for (const FWorldContext& C : GEngine->GetWorldContexts())
					if (C.OwningGameInstance) { GI = C.OwningGameInstance; break; }
			}
			if (!GI) return;
			UCoMStatsSubsystem* S = GI->GetSubsystem<UCoMStatsSubsystem>();
			if (!S) return;
			const FCoMCareerStats& C = S->GetStats();
			UE_LOG(LogTemp, Log, TEXT("=== Career stats ==="));
			UE_LOG(LogTemp, Log, TEXT("Games: %d played, %d won, %d lost"),
				C.GamesPlayed, C.GamesWon, C.GamesLost);
			UE_LOG(LogTemp, Log, TEXT("Fastest win: %d turns. Longest game: %d turns."),
				C.FastestWinTurns, C.LongestGameTurns);
			UE_LOG(LogTemp, Log, TEXT("Battles %d  Units killed %d  Cities captured %d"),
				C.BattlesFought, C.UnitsKilled, C.CitiesCaptured);
			UE_LOG(LogTemp, Log, TEXT("Spells %d  Mana spent %d  Sites cleared %d"),
				C.SpellsCast, C.ManaSpent, C.SitesCleared);
			UE_LOG(LogTemp, Log, TEXT("Items forged %d  Heroes recruited %d"),
				C.ItemsForged, C.HeroesRecruited);
			UE_LOG(LogTemp, Log, TEXT("=== Achievements ==="));
			for (const FCoMAchievement& A : S->GetAchievements())
			{
				UE_LOG(LogTemp, Log, TEXT("  [%s] %s -- %s"),
					A.bUnlocked ? TEXT("UNLOCKED") : TEXT("locked  "),
					*A.DisplayName, *A.Description);
			}
		}));

static FAutoConsoleCommandWithWorldAndArgs GResetStatsCmd(
	TEXT("com.reset_stats"),
	TEXT("Wipe all career stats and relock every achievement."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* World)
		{
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			if (!GI && GEngine)
			{
				for (const FWorldContext& C : GEngine->GetWorldContexts())
					if (C.OwningGameInstance) { GI = C.OwningGameInstance; break; }
			}
			if (!GI) return;
			if (UCoMStatsSubsystem* S = GI->GetSubsystem<UCoMStatsSubsystem>())
			{
				S->ResetAll();
				UE_LOG(LogTemp, Log, TEXT("com.reset_stats: career wiped."));
			}
		}));
