// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMPlaytestSubsystem.h"

#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Framework/CoMGameInstance.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "CoMCore/Turn/CoMTurnTimings.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "CoMCore/Victory/CoMVictorySubsystem.h"
#include "CoMCore/Wizards/CoMPlayerState.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/World/CoMSiteEncounterSubsystem.h"
#include "CoMCore/World/CoMWorldGenerator.h"
#include "CoMCore/World/CoMWeatherSubsystem.h"
#include "CoMCore/Events/CoMWorldEventSubsystem.h"
#include "CoMCore/Economy/CoMResourceSubsystem.h"
#include "CoMCore/Espionage/CoMEspionageSubsystem.h"
#include "CoMCore/Quests/CoMQuestSubsystem.h"
#include "CoMCore/Combat/CoMCombatSubsystem.h"
#include "CoMCore/Combat/CoMSiegeSubsystem.h"
#include "CoMCore/Units/CoMDragonSubsystem.h"
#include "CoMCore/CoreTypes/CoMGameplayTags.h"

#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// =============================================================================
// Error / warning capture
// =============================================================================

namespace
{
	class FPlaytestOutputDevice : public FOutputDevice
	{
	public:
		int32 ErrorCount = 0;
		int32 WarningCount = 0;
		TArray<FString> Captured;
		static constexpr int32 MaxCapturedErrors = 20;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Fatal)
			{
				++ErrorCount;
				if (Captured.Num() < MaxCapturedErrors)
				{
					Captured.Add(FString::Printf(TEXT("[%s] %s"), *Category.ToString(), V));
				}
			}
			else if (Verbosity == ELogVerbosity::Warning)
			{
				++WarningCount;
			}
		}
	};
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMPlaytestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMPlaytestSubsystem::Deinitialize()
{
	LastResults.Empty();
	Super::Deinitialize();
}

// =============================================================================
// Public API
// =============================================================================

void UCoMPlaytestSubsystem::RunPlaytest(int32 NumGames, int32 MaxTurnsPerGame,
                                          int32 BaseSeed, const FString& OutFilePath, int32 NumWizards)
{
	NumGames        = FMath::Clamp(NumGames, 1, 1000);
	MaxTurnsPerGame = FMath::Clamp(MaxTurnsPerGame, 1, 5000);
	ActiveWizardCount = (NumWizards <= 0)
		? CoM::MAX_WIZARDS
		: FMath::Clamp(NumWizards, 2, CoM::MAX_WIZARDS);

	UE_LOG(LogTemp, Display, TEXT("[Playtest] === Beginning batch: %d games x %d turns, %d wizards, base seed %d ==="),
		NumGames, MaxTurnsPerGame, ActiveWizardCount, BaseSeed);
	const double StartAll = FPlatformTime::Seconds();

	LastResults.Reset();
	for (int32 G = 0; G < NumGames; ++G)
	{
		FCoMPlaytestGameResult Result;
		const int32 Seed = BaseSeed + G;

		FPlaytestOutputDevice CaptureDev;
		GLog->AddOutputDevice(&CaptureDev);

		const double StartGame = FPlatformTime::Seconds();
		RunOneGame(G, Seed, MaxTurnsPerGame, Result);
		Result.WallClockSeconds = (float)(FPlatformTime::Seconds() - StartGame);
		Result.ErrorCount   = CaptureDev.ErrorCount;
		Result.WarningCount = CaptureDev.WarningCount;
		Result.CapturedErrors = CaptureDev.Captured;

		GLog->RemoveOutputDevice(&CaptureDev);

		LastResults.Add(Result);
		UE_LOG(LogTemp, Display,
			TEXT("[Playtest] Game %d/%d: %d turns, winner=%d, errors=%d, %.2fs"),
			G + 1, NumGames, Result.TurnsPlayed, Result.WinnerWizardId,
			Result.ErrorCount, Result.WallClockSeconds);
	}

	const double Elapsed = FPlatformTime::Seconds() - StartAll;
	UE_LOG(LogTemp, Display, TEXT("[Playtest] === Done in %.2fs ==="), Elapsed);

	FString FinalPath = OutFilePath;
	if (FinalPath.IsEmpty())
	{
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		FinalPath = FPaths::ProjectSavedDir() / TEXT("Playtest") /
			FString::Printf(TEXT("playtest_%s.json"), *Stamp);
	}
	WriteJsonReport(LastResults, FinalPath);
}

// =============================================================================
// Per-game flow
// =============================================================================

void UCoMPlaytestSubsystem::RunOneGame(int32 GameIndex, int32 Seed, int32 MaxTurns, FCoMPlaytestGameResult& OutResult)
{
	OutResult.GameIndex = GameIndex;
	OutResult.Seed      = Seed;

	// Enable per-phase timing for this game only.
	CoMTurnTimings::Reset();
	CoMTurnTimings::bEnabled = true;

	const double BootstrapStart = FPlatformTime::Seconds();
	BootstrapGame(Seed);
	OutResult.BootstrapSeconds = (float)(FPlatformTime::Seconds() - BootstrapStart);

	UGameInstance* GI = GetGameInstance();
	UCoMTurnSubsystem*    Turn    = GI ? GI->GetSubsystem<UCoMTurnSubsystem>()    : nullptr;
	UCoMVictorySubsystem* Victory = GI ? GI->GetSubsystem<UCoMVictorySubsystem>() : nullptr;
	if (!Turn)
	{
		CoMTurnTimings::bEnabled = false;
		return;
	}

	int32 TurnsPlayed = 0;
	for (; TurnsPlayed < MaxTurns; ++TurnsPlayed)
	{
		Turn->ProcessFullTurn();

		if (Victory && Victory->IsGameOver())
		{
			OutResult.bReachedVictory   = true;
			OutResult.WinnerWizardId    = Victory->GetWinningWizard();
			OutResult.WinningCondition  = static_cast<uint8>(Victory->GetWinningCondition());
			break;
		}
	}
	OutResult.TurnsPlayed = TurnsPlayed;

	CaptureGameResult(OutResult, TurnsPlayed);

	// Snapshot the timings before teardown so we have a clean per-game view.
	for (const auto& Pair : CoMTurnTimings::Elapsed)
	{
		FCoMPlaytestPhaseTime PT;
		PT.Phase        = Pair.Key;
		PT.TotalSeconds = Pair.Value;
		PT.CallCount    = CoMTurnTimings::CallCounts.FindRef(Pair.Key);
		OutResult.PhaseTimes.Add(PT);
	}
	CoMTurnTimings::bEnabled = false;
	CoMTurnTimings::Reset();

	const double TeardownStart = FPlatformTime::Seconds();
	TeardownGame();
	OutResult.TeardownSeconds = (float)(FPlatformTime::Seconds() - TeardownStart);
}

void UCoMPlaytestSubsystem::BootstrapGame(int32 Seed)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// Clear win/elimination state from the previous game — the subsystem instance
	// is reused across the batch, so a stale victory would end the next game at
	// turn 0.
	if (UCoMVictorySubsystem* Victory = GI->GetSubsystem<UCoMVictorySubsystem>())
	{
		Victory->ResetForNewGame();
	}

	UCoMWorldMapSubsystem* Map = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	if (!Map) return;
	Map->InitializeLayers();

	// Full world gen — placeholder uses simple deterministic terrain across
	// Aurelith surface; per-plane gen lives on UCoMWorldGenerator which the
	// commandlet path can invoke for fuller coverage.
	{
		UCoMWorldGenerator* Gen = NewObject<UCoMWorldGenerator>(this);
		Gen->GenerateWorld(Map, Seed);
	}

	// Re-seed every RNG-owning subsystem from this game's seed. Without this the
	// subsystems keep the fixed constants from their Initialize() (0x4865726F,
	// 0x4D616769, ...) so every game in a batch plays out identically. Each
	// ReseedRandom mixes its own salt, so passing the same Seed still yields an
	// independent stream per subsystem while staying reproducible.
	if (UCoMHeroSubsystem*       S = GI->GetSubsystem<UCoMHeroSubsystem>())       { S->ReseedRandom(Seed); }
	if (UCoMMagicSubsystem*      S = GI->GetSubsystem<UCoMMagicSubsystem>())      { S->ReseedRandom(Seed); }
	if (UCoMCombatSubsystem*     S = GI->GetSubsystem<UCoMCombatSubsystem>())     { S->ReseedRandom(Seed); }
	if (UCoMSiegeSubsystem*      S = GI->GetSubsystem<UCoMSiegeSubsystem>())      { S->ReseedRandom(Seed); }
	if (UCoMResourceSubsystem*   S = GI->GetSubsystem<UCoMResourceSubsystem>())   { S->ReseedRandom(Seed); }
	if (UCoMEspionageSubsystem*  S = GI->GetSubsystem<UCoMEspionageSubsystem>())  { S->ReseedRandom(Seed); }
	if (UCoMQuestSubsystem*      S = GI->GetSubsystem<UCoMQuestSubsystem>())      { S->ReseedRandom(Seed); }
	if (UCoMDragonSubsystem*     S = GI->GetSubsystem<UCoMDragonSubsystem>())     { S->ReseedRandom(Seed); }
	if (UCoMWorldEventSubsystem* S = GI->GetSubsystem<UCoMWorldEventSubsystem>()) { S->ReseedRandom(Seed); }
	if (UCoMWeatherSubsystem*    S = GI->GetSubsystem<UCoMWeatherSubsystem>())    { S->InitializeWeather(Seed); }

	// Spawn 14 ACoMPlayerState actors so gold/mana/fame tracking works for
	// AI wizards. Without this, GetWizardByIndex returns nullptr and every
	// economy hook silently no-ops.
	if (UWorld* World = GI->GetWorld())
	{
		if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
		{
			for (int32 W = 0; W < ActiveWizardCount; ++W)
			{
				if (GS->GetWizardByIndex(W)) continue; // already present
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ACoMPlayerState* PS = World->SpawnActor<ACoMPlayerState>(
					ACoMPlayerState::StaticClass(), FTransform::Identity, Params);
				if (!PS) continue;
				PS->WizardIndex   = W;
				PS->bIsHumanPlayer= (W == 0);
				PS->WizardName    = FString::Printf(TEXT("Wizard %d"), W);
				PS->Gold          = 400; // starter gold so AI can hire + forge a Tier 2 hero
				PS->Mana          = 50;
				GS->AddPlayerState(PS);
			}
		}
	}

	// Spawn 14 AI wizards: capitals + a starting army with settler + swordsman.
	// Capital placement is a simple stride across the map; we accept overlaps
	// with terrain since the goal is the AI loop / economy / combat ticks.
	UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>();
	UCoMUnitSubsystem* Units  = GI->GetSubsystem<UCoMUnitSubsystem>();
	const TArray<FGameplayTag> Races = {
		CoMTags::Race::HighMen,    CoMTags::Race::DarkElves,
		CoMTags::Race::HighElves,  CoMTags::Race::Dwarves,
		CoMTags::Race::Orcs,       CoMTags::Race::Trolls,
		CoMTags::Race::Halflings,  CoMTags::Race::Gnolls,
		CoMTags::Race::Lizardmen,  CoMTags::Race::Beastmen,
		CoMTags::Race::Chithari,   CoMTags::Race::Draconians,
		CoMTags::Race::Nomads,     CoMTags::Race::Barbarians
	};
	// Wider spread so capitals don't cluster on dense site/node terrain.
	// 14 wizards on a 160x100 map -> ~7 per row, 30 X-stride / 40 Y-stride.
	for (int32 W = 0; W < ActiveWizardCount; ++W)
	{
		const int32 Col = W % 7;
		const int32 Row = W / 7;
		const FIntPoint Pos(20 + Col * 20, 25 + Row * 50);
		if (Cities)
		{
			Cities->FoundCity(W, ECoMPlane::Aurelith, ECoMMapLayer::Surface, Pos,
				Races[W % Races.Num()],
				FText::FromString(FString::Printf(TEXT("Capital %d"), W)));
		}
		if (Units)
		{
			const FIntPoint ArmyPos(Pos.X + 1, Pos.Y);
			const int32 ArmyID = Units->CreateArmy(W, ECoMPlane::Aurelith, ECoMMapLayer::Surface, ArmyPos);
			// 2 infantry + 1 hero + 1 settler. The hero significantly boosts the
			// stack's encounter power so they can clear early-tier sites.
			for (int32 i = 0; i < 2; ++i)
			{
				const int32 InfID = Units->SpawnUnitByName(FName(TEXT("HighMen_Infantry")),
					ECoMPlane::Aurelith, ECoMMapLayer::Surface, ArmyPos, W);
				if (InfID > 0) Units->AddUnitToArmy(InfID, ArmyID);
			}
			const int32 HeroID = Units->SpawnUnitByName(FName(TEXT("Hero_Fighter")),
				ECoMPlane::Aurelith, ECoMMapLayer::Surface, ArmyPos, W);
			if (HeroID > 0)
			{
				Units->AddUnitToArmy(HeroID, ArmyID);
				if (FCoMUnitInstance* HMut = Units->GetUnitMutable(HeroID))
				{
					HMut->bIsHero = true;
				}
			}
			const int32 SettlerID = Units->SpawnUnitByName(FName(TEXT("Settler")),
				ECoMPlane::Aurelith, ECoMMapLayer::Surface, ArmyPos, W);
			if (SettlerID > 0) Units->AddUnitToArmy(SettlerID, ArmyID);

			// Engineer-capable races start with an engineer so the playtest
			// exercises road building (engineers drop roads as they travel).
			FName EngineerSpec = NAME_None;
			const FGameplayTag Race = Races[W % Races.Num()];
			if      (Race == CoMTags::Race::HighMen)   { EngineerSpec = TEXT("HighMen_Engineer"); }
			else if (Race == CoMTags::Race::Dwarves)   { EngineerSpec = TEXT("Dwarves_Engineer"); }
			else if (Race == CoMTags::Race::Beastmen)  { EngineerSpec = TEXT("Beastmen_Engineer"); }
			else if (Race == CoMTags::Race::Lizardmen) { EngineerSpec = TEXT("Lizardmen_Engineer"); }
			if (!EngineerSpec.IsNone())
			{
				const int32 EngID = Units->SpawnUnitByName(EngineerSpec,
					ECoMPlane::Aurelith, ECoMMapLayer::Surface, ArmyPos, W);
				if (EngID > 0) Units->AddUnitToArmy(EngID, ArmyID);
			}
		}
	}

	// Seed each AI wizard with a starting spell book + a handful of known
	// spells. Without this AI never casts (ManageMagic iterates KnownSpells).
	if (UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>())
	{
		const TArray<ECoMSpellRealm> Realms = {
			ECoMSpellRealm::Life,    ECoMSpellRealm::Death,
			ECoMSpellRealm::Chaos,   ECoMSpellRealm::Nature,
			ECoMSpellRealm::Sorcery, ECoMSpellRealm::Arcane,
			ECoMSpellRealm::Binding, ECoMSpellRealm::Spirit,
			ECoMSpellRealm::Glamour
		};
		for (int32 W = 0; W < ActiveWizardCount; ++W)
		{
			FCoMWizardMagicState& State = Magic->GetWizardMagic(W);
			State.WizardId      = W;
			State.PrimaryRealm  = Realms[W % Realms.Num()];
			State.SpellBooks.Add(State.PrimaryRealm, 4); // 4 books = Common + Uncommon
			State.MaxMana       = 200;
			State.CurrentMana   = 60;
			State.CastingSkill  = 10;

			// Hand out the starting / learnable common-tier spells for the realm.
			TArray<FName> Learnable, Starting;
			CoMSpellDatabase::GetSpellsForBookCount(State.PrimaryRealm, 4, Learnable, Starting);
			for (const FName& SId : Starting)
			{
				State.KnownSpells.AddUnique(SId);
			}
			// Pick the first learnable spell as initial research target.
			if (State.CurrentResearchSpell.IsNone() && Learnable.Num() > 0)
			{
				State.CurrentResearchSpell = Learnable[0];
				State.ResearchAllocation   = 5;
			}

			// Seed each wizard with their realm's signature global enchantment so
			// the playtest actually exercises the global-enchantment system (these
			// are Rare/Very Rare and would never be researched in a short AI game).
			// Extra mana lets the AI afford the high cast cost within a few turns.
			FName GlobalForRealm = NAME_None;
			switch (State.PrimaryRealm)
			{
			case ECoMSpellRealm::Life:    GlobalForRealm = TEXT("Just_Cause");     break;
			case ECoMSpellRealm::Death:   GlobalForRealm = TEXT("Zombie_Mastery"); break;
			case ECoMSpellRealm::Chaos:   GlobalForRealm = TEXT("Great_Wasting");  break;
			case ECoMSpellRealm::Nature:  GlobalForRealm = TEXT("Gaia_Force");     break;
			case ECoMSpellRealm::Sorcery: GlobalForRealm = TEXT("Suppress_Magic"); break;
			case ECoMSpellRealm::Arcane:  GlobalForRealm = TEXT("Detect_Magic");   break;
			case ECoMSpellRealm::Spirit:  GlobalForRealm = TEXT("Spirit_T1_Dream_Vision"); break;
			default: break;
			}
			if (!GlobalForRealm.IsNone())
			{
				State.KnownSpells.AddUnique(GlobalForRealm);
				State.MaxMana     = 500;
				State.CurrentMana = 350; // enough to cast a Very Rare global early
			}

			// Also seed a dispel so the global-enchantment counterplay (AI strips
			// rival enchantments) is exercised in the playtest.
			State.KnownSpells.AddUnique(FName(TEXT("Arcane_T2_Dispel_Magic")));

			// Nature wizards know Enchant Road so they cast it once roads exist,
			// upgrading every built road to an enchanted road (0.25x move cost).
			if (State.PrimaryRealm == ECoMSpellRealm::Nature)
			{
				State.KnownSpells.AddUnique(FName(TEXT("Nature_T2_Enchant_Road")));
			}
		}
	}

	// Bootstrap turn cycle to turn 1.
	if (UCoMTurnSubsystem* Turn = GI->GetSubsystem<UCoMTurnSubsystem>())
	{
		if (!Turn->HasGameStarted())
		{
			Turn->StartGame();
		}
	}
}

void UCoMPlaytestSubsystem::TeardownGame()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// Best-effort reset of mutable per-game state. Subsystem Deinit/Init is
	// not callable mid-session; we wipe the bits that matter to the next run.
	if (UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>())
	{
		Units->ImportAll({}, {}, 1, 1);
	}
	if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
	{
		Items->ImportAll({});
	}
	if (UCoMWorldMapSubsystem* Map = GI->GetSubsystem<UCoMWorldMapSubsystem>())
	{
		Map->ImportAllSites({});
	}
	if (UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>())
	{
		Cities->ImportAll({}, 1);
	}
	if (UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>())
	{
		Magic->ImportAll({}, {}, {}, {}, {});
	}

	// Tear down spawned PlayerStates so the next bootstrap can recreate them
	// fresh. Without this their gold/mana carry over between games.
	if (UWorld* World = GI->GetWorld())
	{
		if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
		{
			for (int32 W = 0; W < ActiveWizardCount; ++W)
			{
				if (ACoMPlayerState* PS = GS->GetWizardByIndex(W))
				{
					GS->RemovePlayerState(PS);
					PS->Destroy();
				}
			}
		}
	}
}

// =============================================================================
// Capture
// =============================================================================

void UCoMPlaytestSubsystem::CaptureGameResult(FCoMPlaytestGameResult& OutResult, int32 TurnsPlayed)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UCoMCitySubsystem*  Cities = GI->GetSubsystem<UCoMCitySubsystem>();
	UCoMUnitSubsystem*  Units  = GI->GetSubsystem<UCoMUnitSubsystem>();
	UCoMMagicSubsystem* Magic  = GI->GetSubsystem<UCoMMagicSubsystem>();
	UCoMItemSubsystem*  Items  = GI->GetSubsystem<UCoMItemSubsystem>();
	UCoMWorldMapSubsystem* Map = GI->GetSubsystem<UCoMWorldMapSubsystem>();

	// Per-wizard rollup.
	for (int32 W = 0; W < ActiveWizardCount; ++W)
	{
		FCoMPlaytestWizardResult WR;
		WR.WizardIndex = W;

		if (Cities)
		{
			const TArray<const FCoMCityData*> Owned = Cities->GetCitiesForWizard(W);
			WR.Cities = Owned.Num();
		}
		if (Units)
		{
			const TArray<const FCoMArmyGroup*> Armies = Units->GetArmiesForWizard(W);
			WR.Armies = Armies.Num();
			for (const FCoMArmyGroup* A : Armies)
			{
				if (!A) continue;
				WR.Units  += A->UnitIDs.Num();
				for (int32 UID : A->UnitIDs)
				{
					const FCoMUnitInstance* U = Units->GetUnit(UID);
					if (U && U->bIsHero) { ++WR.Heroes; }
				}
			}
		}
		if (Magic)
		{
			const FCoMWizardMagicState& State = Magic->GetWizardMagic(W);
			WR.Mana        = State.CurrentMana;
			WR.SpellsKnown = State.KnownSpells.Num();
		}
		if (Items)
		{
			const TArray<FCoMItemInstance> Owned = Items->GetItemsForWizard(W);
			WR.ItemsForged = Owned.Num();
		}
		if (Map)
		{
			const TArray<FCoMSite> All = Map->GetAllSites();
			for (const FCoMSite& S : All)
			{
				if (S.bCleared && S.ClearedByWizard == W) { ++WR.SitesCleared; }
			}
		}

		// Gold via player state, if present.
		if (UWorld* World = GI->GetWorld())
		{
			if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
			{
				if (ACoMPlayerState* PS = GS->GetWizardByIndex(W))
				{
					WR.Gold   = PS->Gold;
					WR.bAlive = true;
				}
			}
		}

		OutResult.Wizards.Add(WR);

		OutResult.TotalCities       += WR.Cities;
		OutResult.TotalUnits        += WR.Units;
		OutResult.TotalSitesCleared += WR.SitesCleared;
		OutResult.TotalItemsForged  += WR.ItemsForged;
		// HeroesHired = heroes alive; approximation.
		OutResult.TotalHeroesHired  += WR.Heroes;
	}
}

// =============================================================================
// JSON dump
// =============================================================================

void UCoMPlaytestSubsystem::WriteJsonReport(const TArray<FCoMPlaytestGameResult>& Results, const FString& OutFilePath) const
{
	FString OutString;
	auto W = TJsonWriterFactory<>::Create(&OutString);
	W->WriteObjectStart();
	W->WriteValue(TEXT("generated_at"), FDateTime::Now().ToString());
	W->WriteValue(TEXT("game_count"), Results.Num());

	// Aggregate top-line numbers.
	int32 TotalErrors = 0;
	int32 TotalTurns  = 0;
	int32 TotalVictories = 0;
	TMap<int32, int32> WinsByWizard;
	float TotalSeconds = 0.0f;
	for (const FCoMPlaytestGameResult& R : Results)
	{
		TotalErrors  += R.ErrorCount;
		TotalTurns   += R.TurnsPlayed;
		TotalSeconds += R.WallClockSeconds;
		if (R.bReachedVictory)
		{
			++TotalVictories;
			int32& C = WinsByWizard.FindOrAdd(R.WinnerWizardId);
			++C;
		}
	}
	W->WriteValue(TEXT("total_turns"),    TotalTurns);
	W->WriteValue(TEXT("total_errors"),   TotalErrors);
	W->WriteValue(TEXT("total_victories"),TotalVictories);
	W->WriteValue(TEXT("total_seconds"),  TotalSeconds);

	W->WriteArrayStart(TEXT("wins_by_wizard"));
	for (const auto& Pair : WinsByWizard)
	{
		W->WriteObjectStart();
		W->WriteValue(TEXT("wizard"), Pair.Key);
		W->WriteValue(TEXT("wins"),   Pair.Value);
		W->WriteObjectEnd();
	}
	W->WriteArrayEnd();

	W->WriteArrayStart(TEXT("games"));
	for (const FCoMPlaytestGameResult& R : Results)
	{
		W->WriteObjectStart();
		W->WriteValue(TEXT("game"),                R.GameIndex);
		W->WriteValue(TEXT("seed"),                R.Seed);
		W->WriteValue(TEXT("turns_played"),        R.TurnsPlayed);
		W->WriteValue(TEXT("reached_victory"),     R.bReachedVictory);
		W->WriteValue(TEXT("winner_wizard"),       R.WinnerWizardId);
		W->WriteValue(TEXT("winning_condition"),   (int32)R.WinningCondition);
		W->WriteValue(TEXT("total_cities"),        R.TotalCities);
		W->WriteValue(TEXT("total_units"),         R.TotalUnits);
		W->WriteValue(TEXT("total_sites_cleared"), R.TotalSitesCleared);
		W->WriteValue(TEXT("total_items_forged"),  R.TotalItemsForged);
		W->WriteValue(TEXT("total_heroes_hired"),  R.TotalHeroesHired);
		W->WriteValue(TEXT("errors"),              R.ErrorCount);
		W->WriteValue(TEXT("warnings"),            R.WarningCount);
		W->WriteValue(TEXT("seconds"),             R.WallClockSeconds);
		W->WriteValue(TEXT("bootstrap_seconds"),   R.BootstrapSeconds);
		W->WriteValue(TEXT("teardown_seconds"),    R.TeardownSeconds);

		W->WriteArrayStart(TEXT("phase_times"));
		for (const FCoMPlaytestPhaseTime& PT : R.PhaseTimes)
		{
			W->WriteObjectStart();
			W->WriteValue(TEXT("phase"), PT.Phase.ToString());
			W->WriteValue(TEXT("total_seconds"), PT.TotalSeconds);
			W->WriteValue(TEXT("call_count"),    PT.CallCount);
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();

		W->WriteArrayStart(TEXT("wizards"));
		for (const FCoMPlaytestWizardResult& WR : R.Wizards)
		{
			W->WriteObjectStart();
			W->WriteValue(TEXT("idx"),     WR.WizardIndex);
			W->WriteValue(TEXT("alive"),   WR.bAlive);
			W->WriteValue(TEXT("cities"),  WR.Cities);
			W->WriteValue(TEXT("armies"),  WR.Armies);
			W->WriteValue(TEXT("units"),   WR.Units);
			W->WriteValue(TEXT("heroes"),  WR.Heroes);
			W->WriteValue(TEXT("gold"),    WR.Gold);
			W->WriteValue(TEXT("mana"),    WR.Mana);
			W->WriteValue(TEXT("spells_known"),  WR.SpellsKnown);
			W->WriteValue(TEXT("items_forged"),  WR.ItemsForged);
			W->WriteValue(TEXT("sites_cleared"), WR.SitesCleared);
			W->WriteObjectEnd();
		}
		W->WriteArrayEnd();

		W->WriteArrayStart(TEXT("captured_errors"));
		for (const FString& Err : R.CapturedErrors)
		{
			W->WriteValue(Err);
		}
		W->WriteArrayEnd();

		W->WriteObjectEnd();
	}
	W->WriteArrayEnd();

	W->WriteObjectEnd();
	W->Close();

	// Ensure dir exists, write file.
	const FString Dir = FPaths::GetPath(OutFilePath);
	IFileManager::Get().MakeDirectory(*Dir, /*Tree*/ true);
	FFileHelper::SaveStringToFile(OutString, *OutFilePath);
	UE_LOG(LogTemp, Display, TEXT("[Playtest] Report written to %s (%d games)"),
		*OutFilePath, Results.Num());
}

// =============================================================================
// Console command: com.playtest <games> <maxturns> <seed>
// =============================================================================

// World-bound variant — usable from the in-game tilde console.
static FAutoConsoleCommandWithWorldAndArgs GPlaytestCmd(
	TEXT("com.playtest"),
	TEXT("Run AI-vs-AI playtest batch.  Args: <games> <maxturns> <seed> <numWizards>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) return;
			UGameInstance* GI = World->GetGameInstance();
			if (!GI) return;
			UCoMPlaytestSubsystem* Sub = GI->GetSubsystem<UCoMPlaytestSubsystem>();
			if (!Sub) return;

			const int32 Games    = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 5;
			const int32 MaxTurns = (Args.Num() > 1) ? FCString::Atoi(*Args[1]) : 100;
			const int32 Seed     = (Args.Num() > 2) ? FCString::Atoi(*Args[2]) : 42;
			const int32 Wizards  = (Args.Num() > 3) ? FCString::Atoi(*Args[3]) : 0;
			Sub->RunPlaytest(Games, MaxTurns, Seed, /*OutFilePath*/ TEXT(""), Wizards);
		}));

// World-less variant — usable from `-ExecCmds` on a no-map commandlet.
// Walks every loaded GameInstance to find the subsystem.
static FAutoConsoleCommand GPlaytestCmdNoWorld(
	TEXT("com.playtest_headless"),
	TEXT("Headless variant of com.playtest. Args: <games> <maxturns> <seed> <numWizards>"),
	FConsoleCommandWithArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args)
		{
			// Find a GameInstance via the engine.
			UGameInstance* GI = nullptr;
			if (GEngine)
			{
				for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
				{
					if (Ctx.OwningGameInstance)
					{
						GI = Ctx.OwningGameInstance;
						break;
					}
				}
			}
			if (!GI) return;
			UCoMPlaytestSubsystem* Sub = GI->GetSubsystem<UCoMPlaytestSubsystem>();
			if (!Sub) return;

			const int32 Games    = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 5;
			const int32 MaxTurns = (Args.Num() > 1) ? FCString::Atoi(*Args[1]) : 100;
			const int32 Seed     = (Args.Num() > 2) ? FCString::Atoi(*Args[2]) : 42;
			const int32 Wizards  = (Args.Num() > 3) ? FCString::Atoi(*Args[3]) : 0;
			Sub->RunPlaytest(Games, MaxTurns, Seed, /*OutFilePath*/ TEXT(""), Wizards);
		}));
