// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlaytestSubsystem.h -- Headless AI-vs-AI playtest harness for finding
// bugs, balance issues, and gameplay pacing problems.
//
// Run paths:
//   1. From an in-editor PIE session: ~ console -> `com.playtest 5 100 42`
//   2. From the editor with a map open: same console command
//   3. From Python (in-editor):
//        gi = unreal.GameplayStatics.get_game_instance(world)
//        sub = gi.get_subsystem(unreal.CoMPlaytestSubsystem)
//        sub.run_playtest(games, maxturns, seed, "")
//
// Output: Saved/Playtest/playtest_<timestamp>.json (per-game + aggregate).
//
// The harness blocks the game thread for the duration of the batch — this is
// dev-only tooling. Don't ship it bound to a hotkey.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMPlaytestSubsystem.generated.h"

/** Per-wizard end-state snapshot for one game. */
USTRUCT()
struct COMCORE_API FCoMPlaytestWizardResult
{
	GENERATED_BODY()

	UPROPERTY() int32 WizardIndex   = -1;
	UPROPERTY() bool  bAlive        = false;
	UPROPERTY() int32 Cities        = 0;
	UPROPERTY() int32 Armies        = 0;
	UPROPERTY() int32 Units         = 0;
	UPROPERTY() int32 Heroes        = 0;
	UPROPERTY() int32 Gold          = 0;
	UPROPERTY() int32 Mana          = 0;
	UPROPERTY() int32 SpellsKnown   = 0;
	UPROPERTY() int32 ItemsForged   = 0;
	UPROPERTY() int32 SitesCleared  = 0;
};

/** One full game's worth of summary data. */
USTRUCT()
struct COMCORE_API FCoMPlaytestGameResult
{
	GENERATED_BODY()

	UPROPERTY() int32   GameIndex      = -1;
	UPROPERTY() int32   Seed           = 0;
	UPROPERTY() int32   TurnsPlayed    = 0;
	UPROPERTY() bool    bReachedVictory= false;
	UPROPERTY() int32   WinnerWizardId = -1;
	UPROPERTY() uint8   WinningCondition = 0;
	UPROPERTY() int32   TotalCities    = 0;
	UPROPERTY() int32   TotalUnits     = 0;
	UPROPERTY() int32   TotalSitesCleared = 0;
	UPROPERTY() int32   TotalItemsForged  = 0;
	UPROPERTY() int32   TotalHeroesHired  = 0;
	UPROPERTY() int32   ErrorCount     = 0;
	UPROPERTY() int32   WarningCount   = 0;
	UPROPERTY() float   WallClockSeconds = 0.0f;
	UPROPERTY() TArray<FCoMPlaytestWizardResult> Wizards;
	UPROPERTY() TArray<FString> CapturedErrors;   // first ~20 errors only
};

/**
 * UCoMPlaytestSubsystem
 *
 * Drives the headless playtest harness. RunPlaytest:
 *   1. For each game:
 *      a. Initialize world map + populate all 14 slots as AI
 *      b. Loop ProcessFullTurn() until victory or MaxTurns
 *      c. Capture per-wizard end state + counters
 *      d. Reset for next game
 *   2. After all games: write JSON + log summary
 *
 * The subsystem hooks an FOutputDevice so any LogTemp errors or ensures
 * during the run get counted and (first N) captured for the report.
 */
UCLASS()
class COMCORE_API UCoMPlaytestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Run a batch of headless AI-vs-AI games.
	 *   NumGames           Games to run
	 *   MaxTurnsPerGame    Turn cap; runs end early on victory
	 *   BaseSeed           Seed for game 0; game N uses BaseSeed + N
	 *   OutFilePath        Empty = Saved/Playtest/playtest_<timestamp>.json
	 *
	 * Blocks the game thread; intended for editor / dev use.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Playtest")
	void RunPlaytest(int32 NumGames, int32 MaxTurnsPerGame,
	                 int32 BaseSeed, const FString& OutFilePath);

	/** Returns last batch's results (in-memory, for tests). */
	const TArray<FCoMPlaytestGameResult>& GetLastResults() const { return LastResults; }

private:
	void RunOneGame(int32 GameIndex, int32 Seed, int32 MaxTurns, FCoMPlaytestGameResult& OutResult);
	void BootstrapGame(int32 Seed);
	void TeardownGame();
	void CaptureGameResult(FCoMPlaytestGameResult& OutResult, int32 TurnsPlayed);
	void WriteJsonReport(const TArray<FCoMPlaytestGameResult>& Results, const FString& OutFilePath) const;

	TArray<FCoMPlaytestGameResult> LastResults;
};
