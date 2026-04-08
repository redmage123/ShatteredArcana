// Copyright Mythforge Studios. All Rights Reserved.
// CoMEspionageSubsystem.h — Spy agents, missions, counterintelligence, organizations
// Phase 7 — Shattered Arcana

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMEspionageSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// SPY AGENT
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMSpyAgent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AgentId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OwnerWizardId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AgentName;

    /** Skill level 1-100. Higher = better success rate, harder to catch. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "100"))
    int32 SkillLevel = 20;

    /** Experience points — accumulates from missions, levels up skill. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Experience = 0;

    /** Current mission (None if idle). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMAgentMission CurrentMission = ECoMAgentMission::MAX;

    /** Target wizard for current mission. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetWizardId = -1;

    /** Target tile for location-based missions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint TargetTile;

    /** Turns remaining on current mission. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MissionTurnsLeft = 0;

    /** Whether this agent has been captured/detected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCaptured = false;

    /** Whether this agent is a double agent (turned by enemy). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDoubleAgent = false;

    /** If double agent, who controls them now. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TrueControllerWizardId = -1;

    /** Plane the agent is currently operating in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMPlane CurrentPlane = ECoMPlane::Aurelith;

    /** Cover identity — affects detection in certain races/cities. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMRace CoverRace = ECoMRace::HighMen;

    /** Turn recruited. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RecruitedTurn = 0;

    /** Number of successful missions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SuccessfulMissions = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// MISSION RESULT
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMMissionResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AgentId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECoMAgentMission Mission = ECoMAgentMission::MAX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDetected = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCaptured = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bKilled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ResultDescription;

    /** Stolen intel (for spy missions). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> StolenIntel;

    /** Resources stolen (for theft missions). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ECoMResource, int32> StolenResources;

    /** Damage dealt (for sabotage missions). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DamageDealt = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CompletedTurn = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// COUNTERINTELLIGENCE
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FCoMCounterIntel
{
    GENERATED_BODY()

    /** Global counterintelligence level for this wizard (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
    int32 SecurityLevel = 20;

    /** Number of dedicated counter-spy agents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CounterSpyAgents = 0;

    /** Resources invested in security this turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SecurityBudget = 0;

    /** Known enemy agents (detected but not yet caught). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> DetectedEnemyAgents;

    /** Captured enemy agents (can be executed, ransomed, or turned). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> CapturedEnemyAgents;
};

// ─────────────────────────────────────────────────────────────────────────────
// ESPIONAGE SUBSYSTEM
// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class COMCORE_API UCoMEspionageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── Agent Management ─────────────────────────────────────────────────────

    /** Recruit a new spy agent. Cost depends on skill level. */
    UFUNCTION(BlueprintCallable, Category = "Espionage")
    int32 RecruitAgent(int32 WizardId, const FString& Name, int32 SkillLevel, int32 CurrentTurn);

    /** Get all agents owned by a wizard. */
    UFUNCTION(BlueprintCallable, Category = "Espionage")
    TArray<FCoMSpyAgent> GetAgents(int32 WizardId) const;

    /** Get a specific agent. */

    /** Dismiss an agent. */
    FCoMSpyAgent* GetAgent(int32 AgentId);

	void DismissAgent(int32 AgentId);

    // ── Missions ─────────────────────────────────────────────────────────────

    /** Send an agent on a mission. Returns true if mission started. */
    bool AssignMission(int32 AgentId, ECoMAgentMission Mission, int32 TargetWizardId,
                       FIntPoint TargetTile);

    /** Cancel a mission in progress. Agent returns to idle. */
    void CancelMission(int32 AgentId);

    /** Get estimated success chance for a mission (0-100). */
    int32 GetMissionSuccessChance(int32 AgentId, ECoMAgentMission Mission,
                                  int32 TargetWizardId) const;

    /** Get duration in turns for a mission type. */
    int32 GetMissionDuration(ECoMAgentMission Mission) const;

    /** Get recent mission results for a wizard. */
    TArray<FCoMMissionResult> GetMissionHistory(int32 WizardId, int32 MaxResults = 10) const;

    // ── Counterintelligence ──────────────────────────────────────────────────

    /** Set security budget for a wizard (gold per turn spent on counter-espionage). */
    void SetSecurityBudget(int32 WizardId, int32 Budget);

    /** Assign agents to counterintelligence duty. */
    void AssignCounterSpy(int32 WizardId, int32 AgentId);

    /** Get counterintelligence status for a wizard. */
    FCoMCounterIntel GetCounterIntel(int32 WizardId) const;

    /** Execute a captured enemy agent (removes them, reputation impact). */
    void ExecuteCapturedAgent(int32 WizardId, int32 CapturedAgentId);

    /** Ransom a captured agent back (gain resources, agent returns to enemy). */
    void RansomCapturedAgent(int32 WizardId, int32 CapturedAgentId);

    /** Turn a captured agent into a double agent. */
    bool TurnCapturedAgent(int32 WizardId, int32 CapturedAgentId);

    // ── Turn Processing ──────────────────────────────────────────────────────

    /** Process all espionage activities for the turn. */
    void ProcessTurn(int32 CurrentTurn);

private:
    /** Resolve a single mission. */
    FCoMMissionResult ResolveMission(FCoMSpyAgent& Agent, int32 CurrentTurn);

    /** Calculate detection roll against target's counter-intelligence. */
    bool RollDetection(const FCoMSpyAgent& Agent, int32 TargetWizardId) const;

    /** Level up agent based on experience. */
    void CheckLevelUp(FCoMSpyAgent& Agent);

    /** Generate a random agent name. */
    FString GenerateAgentName() const;

    UPROPERTY()
    TMap<int32, FCoMSpyAgent> AllAgents;

    UPROPERTY()
    TMap<int32, FCoMCounterIntel> CounterIntelMap;

    UPROPERTY()
    TArray<FCoMMissionResult> MissionHistory;

    int32 NextAgentId = 1;

    FRandomStream RngStream;
};
