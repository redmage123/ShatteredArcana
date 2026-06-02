// Copyright Shattered Arcana. All Rights Reserved.
// CoMStatsSubsystem.h -- Career-wide stats + achievement tracking.
//
// Lives at GameInstance scope so it persists across maps. Serialises to
// Saved/com_stats.json out-of-band from the save-slot subsystem (career
// stats outlive any single playthrough).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMStatsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct COMCORE_API FCoMAchievement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName  AchievementID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool   bUnlocked  = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32  UnlockedAtGame = -1; // GamesPlayed at unlock
};

USTRUCT(BlueprintType)
struct COMCORE_API FCoMCareerStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GamesPlayed   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GamesWon      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GamesLost     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 FastestWinTurns = -1; // -1 = no wins yet
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 LongestGameTurns = 0;

	// Per-victory-type wins (indexed by static_cast<int32>(ECoMVictoryType)).
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> WinsByVictoryType;

	// Per-wizard-slot wins (0..13).
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> WinsByWizardSlot;

	// Lifetime totals across all games.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BattlesFought   = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 UnitsKilled     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CitiesCaptured  = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SpellsCast      = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ManaSpent       = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SitesCleared    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ItemsForged     = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HeroesRecruited = 0;

	void Reset() { *this = FCoMCareerStats{}; }
};

UCLASS()
class COMCORE_API UCoMStatsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Read-only stats snapshot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stats")
	const FCoMCareerStats& GetStats() const { return Stats; }

	/** All achievements, locked + unlocked. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stats")
	const TArray<FCoMAchievement>& GetAchievements() const { return Achievements; }

	/** Hook: a game just ended. Updates win/loss tallies and checks
	 *  achievement unlocks. Pass the winning wizard slot (-1 if no winner). */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordGameEnd(int32 WinnerWizardSlot, int32 PlayerWizardSlot,
		ECoMVictoryType VictoryType, int32 TurnsPlayed);

	/** Lifetime counter hooks. Fired from the matching subsystems. */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordBattleFought();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordUnitsKilled(int32 N);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordCityCaptured();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordSpellCast(int32 ManaCost);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordSiteCleared();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordItemForged();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordHeroRecruited();

	/** Persist now (otherwise saved on Deinitialize and after RecordGameEnd). */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SaveToDisk();

	/** Wipe all stats + relock all achievements. */
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetAll();

private:
	void LoadFromDisk();
	void EnsureArraySizes();
	void RegisterDefaultAchievements();
	void EvaluateAchievements();

	/** Subscribe to combat / site / item delegates so the lifetime
	 *  counters fill in from real gameplay events. Deferred to BeginPlay
	 *  because the other subsystems may not have Initialize-ordered yet. */
	void BindToSubsystems();

	UFUNCTION() void HandleCombatResolved(const struct FCoMCombatResult& Result);
	UFUNCTION() void HandleSiteCleared(int32 SiteID, int32 WizardIndex, int32 GoldReward, int32 ManaReward);
	UFUNCTION() void HandleItemForged(int32 InstanceID);

	UPROPERTY()
	FCoMCareerStats Stats;

	UPROPERTY()
	TArray<FCoMAchievement> Achievements;

	bool bLoaded = false;
};
