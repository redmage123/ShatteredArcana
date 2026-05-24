// Copyright Mythforge Studios. All Rights Reserved.
// CoMEspionageSubsystem.cpp — Full implementation
// Phase 7 — Shattered Arcana

#include "CoMEspionageSubsystem.h"

// ─────────────────────────────────────────────────────────────────────────────
// LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

void UCoMEspionageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    AllAgents.Empty();
    CounterIntelMap.Empty();
    MissionHistory.Empty();
    NextAgentId = 1;
    RngStream.Initialize(0x45737069);
}

void UCoMEspionageSubsystem::ReseedRandom(int32 MasterSeed)
{
    RngStream.Initialize(MasterSeed ^ 0x45737069);
}

void UCoMEspionageSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// AGENT MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

int32 UCoMEspionageSubsystem::RecruitAgent(int32 WizardId, const FString& Name, int32 SkillLevel, int32 CurrentTurn)
{
    FCoMSpyAgent Agent;
    Agent.AgentId = NextAgentId++;
    Agent.OwnerWizardId = WizardId;
    Agent.AgentName = Name.IsEmpty() ? GenerateAgentName() : Name;
    Agent.SkillLevel = FMath::Clamp(SkillLevel, 1, 100);
    Agent.RecruitedTurn = CurrentTurn;
    Agent.CurrentMission = ECoMAgentMission::MAX;

    AllAgents.Add(Agent.AgentId, Agent);
    return Agent.AgentId;
}

TArray<FCoMSpyAgent> UCoMEspionageSubsystem::GetAgents(int32 WizardId) const
{
    TArray<FCoMSpyAgent> Result;
    for (const auto& Pair : AllAgents)
    {
        if (Pair.Value.OwnerWizardId == WizardId)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

FCoMSpyAgent* UCoMEspionageSubsystem::GetAgent(int32 AgentId)
{
    return AllAgents.Find(AgentId);
}

void UCoMEspionageSubsystem::DismissAgent(int32 AgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
    if (Agent && Agent->CurrentMission == ECoMAgentMission::GuardAsset)
    {
        FCoMCounterIntel* CI = CounterIntelMap.Find(Agent->OwnerWizardId);
        if (CI && CI->CounterSpyAgents > 0)
        {
            CI->CounterSpyAgents--;
        }
    }
    AllAgents.Remove(AgentId);
}

// ─────────────────────────────────────────────────────────────────────────────
// MISSIONS
// ─────────────────────────────────────────────────────────────────────────────

bool UCoMEspionageSubsystem::AssignMission(int32 AgentId, ECoMAgentMission Mission,
                                            int32 TargetWizardId, FIntPoint TargetTile)
{
    FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
    if (!Agent) return false;
    if (Agent->CurrentMission != ECoMAgentMission::MAX) return false;
    if (Agent->bCaptured) return false;

    Agent->CurrentMission = Mission;
    Agent->TargetWizardId = TargetWizardId;
    Agent->TargetTile = TargetTile;
    Agent->MissionTurnsLeft = GetMissionDuration(Mission);

    return true;
}

void UCoMEspionageSubsystem::CancelMission(int32 AgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
    if (!Agent) return;

    // If agent was on counterspy duty, decrement the counter
    if (Agent->CurrentMission == ECoMAgentMission::GuardAsset)
    {
        FCoMCounterIntel* CI = CounterIntelMap.Find(Agent->OwnerWizardId);
        if (CI && CI->CounterSpyAgents > 0)
        {
            CI->CounterSpyAgents--;
        }
    }

    Agent->CurrentMission = ECoMAgentMission::MAX;
    Agent->TargetWizardId = -1;
    Agent->MissionTurnsLeft = 0;
}

int32 UCoMEspionageSubsystem::GetMissionSuccessChance(int32 AgentId, ECoMAgentMission Mission,
                                                       int32 TargetWizardId) const
{
    const FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
    if (!Agent) return 0;

    int32 BaseChance = Agent->SkillLevel;

    // Mission difficulty modifiers
    switch (Mission)
    {
    case ECoMAgentMission::Spy:           BaseChance += 20; break;  // Easiest
    case ECoMAgentMission::Sabotage:      BaseChance -= 10; break;
    case ECoMAgentMission::Steal:     BaseChance -= 15; break;
    case ECoMAgentMission::Assassinate:   BaseChance -= 30; break;  // Hardest
    case ECoMAgentMission::InfiltrateCity:    BaseChance += 0;  break;
    case ECoMAgentMission::GuardAsset:    BaseChance += 10; break;
    case ECoMAgentMission::PropagandaCampaign:    BaseChance += 5;  break;
	// case ECoMAgentMission::PropagandaCampaign: // duplicate removed
    default: break;
    }

    // Target's counterintelligence reduces chance
    if (const FCoMCounterIntel* CI = CounterIntelMap.Find(TargetWizardId))
    {
        BaseChance -= CI->SecurityLevel / 2;
        BaseChance -= CI->CounterSpyAgents * 5;
    }

    // Double agents have reduced effectiveness
    if (Agent->bDoubleAgent)
    {
        BaseChance -= 15;
    }

    return FMath::Clamp(BaseChance, 5, 95); // Always 5-95% chance
}

int32 UCoMEspionageSubsystem::GetMissionDuration(ECoMAgentMission Mission) const
{
    switch (Mission)
    {
    case ECoMAgentMission::Spy:                return 2;
    case ECoMAgentMission::Sabotage:           return 3;
    case ECoMAgentMission::Steal:              return 3;
    case ECoMAgentMission::Assassinate:        return 4;
    case ECoMAgentMission::InfiltrateCity:     return 3;
    case ECoMAgentMission::GuardAsset:         return -1;
    case ECoMAgentMission::PropagandaCampaign: return 2;
    default: return 1;
    }
}

TArray<FCoMMissionResult> UCoMEspionageSubsystem::GetMissionHistory(int32 WizardId, int32 MaxResults) const
{
    TArray<FCoMMissionResult> Result;
    for (int32 i = MissionHistory.Num() - 1; i >= 0 && Result.Num() < MaxResults; --i)
    {
        const FCoMSpyAgent* Agent = AllAgents.Find(MissionHistory[i].AgentId);
        if (Agent && Agent->OwnerWizardId == WizardId)
        {
            Result.Add(MissionHistory[i]);
        }
    }
    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// COUNTERINTELLIGENCE
// ─────────────────────────────────────────────────────────────────────────────

void UCoMEspionageSubsystem::SetSecurityBudget(int32 WizardId, int32 Budget)
{
    FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(WizardId);
    CI.SecurityBudget = FMath::Max(0, Budget);
    // Security level grows with budget investment
    CI.SecurityLevel = FMath::Clamp(20 + Budget / 5, 0, 100);
}

void UCoMEspionageSubsystem::AssignCounterSpy(int32 WizardId, int32 AgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
    if (!Agent || Agent->OwnerWizardId != WizardId) return;

    Agent->CurrentMission = ECoMAgentMission::GuardAsset;
    Agent->TargetWizardId = WizardId; // Defending own territory
    Agent->MissionTurnsLeft = -1; // Ongoing

    FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(WizardId);
    CI.CounterSpyAgents++;
}

FCoMCounterIntel UCoMEspionageSubsystem::GetCounterIntel(int32 WizardId) const
{
    if (const FCoMCounterIntel* CI = CounterIntelMap.Find(WizardId))
    {
        return *CI;
    }
    return FCoMCounterIntel();
}

void UCoMEspionageSubsystem::ExecuteCapturedAgent(int32 WizardId, int32 CapturedAgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(CapturedAgentId);
    if (!Agent || !Agent->bCaptured) return;

    FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(WizardId);
    CI.CapturedEnemyAgents.Remove(CapturedAgentId);

    // Remove the agent permanently
    AllAgents.Remove(CapturedAgentId);

    // Diplomatic impact — executing spies angers the spy's owner
    // (Would need DiplomacySubsystem reference here — deferred to integration)
}

void UCoMEspionageSubsystem::RansomCapturedAgent(int32 WizardId, int32 CapturedAgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(CapturedAgentId);
    if (!Agent || !Agent->bCaptured) return;

    FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(WizardId);
    CI.CapturedEnemyAgents.Remove(CapturedAgentId);

    // Return agent to owner (with reduced skill from captivity)
    Agent->bCaptured = false;
    Agent->CurrentMission = ECoMAgentMission::MAX;
    Agent->SkillLevel = FMath::Max(1, Agent->SkillLevel - 10);
}

bool UCoMEspionageSubsystem::TurnCapturedAgent(int32 WizardId, int32 CapturedAgentId)
{
    FCoMSpyAgent* Agent = AllAgents.Find(CapturedAgentId);
    if (!Agent || !Agent->bCaptured) return false;

    // Success chance based on agent's loyalty vs captor's cunning
    // Simplified: 30% base chance
    int32 Roll = RngStream.RandRange(1, 100);
    if (Roll > 30) return false;

    Agent->bCaptured = false;
    Agent->bDoubleAgent = true;
    Agent->TrueControllerWizardId = WizardId;
    Agent->CurrentMission = ECoMAgentMission::MAX;

    FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(WizardId);
    CI.CapturedEnemyAgents.Remove(CapturedAgentId);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TURN PROCESSING
// ─────────────────────────────────────────────────────────────────────────────

void UCoMEspionageSubsystem::ProcessTurn(int32 CurrentTurn)
{
    // Early-out: no agents to process.
    if (AllAgents.Num() == 0)
    {
        return;
    }

    TArray<int32> AgentsToProcess;
    for (auto& Pair : AllAgents)
    {
        if (Pair.Value.CurrentMission != ECoMAgentMission::MAX &&
            Pair.Value.CurrentMission != ECoMAgentMission::GuardAsset &&
            !Pair.Value.bCaptured)
        {
            AgentsToProcess.Add(Pair.Key);
        }
    }

    for (int32 AgentId : AgentsToProcess)
    {
        FCoMSpyAgent* Agent = AllAgents.Find(AgentId);
        if (!Agent) continue;

        Agent->MissionTurnsLeft--;

        if (Agent->MissionTurnsLeft <= 0)
        {
            // Mission complete — resolve
            FCoMMissionResult Result = ResolveMission(*Agent, CurrentTurn);
            MissionHistory.Add(Result);

            // Keep history manageable
            if (MissionHistory.Num() > 200)
            {
                MissionHistory.RemoveAt(0, 50);
            }
        }
    }

    // Level up agents with enough experience
    for (auto& Pair : AllAgents)
    {
        CheckLevelUp(Pair.Value);
    }
}

FCoMMissionResult UCoMEspionageSubsystem::ResolveMission(FCoMSpyAgent& Agent, int32 CurrentTurn)
{
    FCoMMissionResult Result;
    Result.AgentId = Agent.AgentId;
    Result.Mission = Agent.CurrentMission;
    Result.CompletedTurn = CurrentTurn;

    // Copy agent ID before any potential removal to avoid use-after-free
    const int32 AgentId = Agent.AgentId;

    int32 SuccessChance = GetMissionSuccessChance(AgentId, Agent.CurrentMission, Agent.TargetWizardId);
    int32 Roll = RngStream.RandRange(1, 100);

    Result.bSuccess = (Roll <= SuccessChance);

    // Detection check
    Result.bDetected = RollDetection(Agent, Agent.TargetWizardId);

    // Handle success/failure rewards BEFORE any kill/remove path
    if (Result.bSuccess)
    {
        Agent.SuccessfulMissions++;
        Agent.Experience += 20;

        switch (Agent.CurrentMission)
        {
        case ECoMAgentMission::Spy:
            Result.StolenIntel.Add(TEXT("Army composition revealed"));
            Result.StolenIntel.Add(TEXT("City production visible"));
            Result.ResultDescription = TEXT("Successfully gathered intelligence");
            break;
        case ECoMAgentMission::Sabotage:
            Result.DamageDealt = 50 + Agent.SkillLevel;
            Result.ResultDescription = TEXT("Sabotaged enemy infrastructure");
            break;
        case ECoMAgentMission::Steal:
            Result.StolenIntel.Add(TEXT("Spell knowledge acquired"));
            Result.ResultDescription = TEXT("Stole magical research");
            Agent.Experience += 10;
            break;
        case ECoMAgentMission::Assassinate:
            Result.ResultDescription = TEXT("Target eliminated");
            Agent.Experience += 30;
            break;
        case ECoMAgentMission::PropagandaCampaign:
            Result.ResultDescription = TEXT("Public opinion shifted");
            break;
        case ECoMAgentMission::InfiltrateCity:
            Result.ResultDescription = TEXT("Rebellion incited in target city");
            Agent.Experience += 15;
            break;
        default:
            Result.ResultDescription = TEXT("Mission completed");
            break;
        }
    }
    else
    {
        Agent.Experience += 5; // Small XP even for failure
        Result.ResultDescription = TEXT("Mission failed");
    }

    if (Result.bDetected)
    {
        // Capture chance if detected
        int32 CaptureRoll = RngStream.RandRange(1, 100);
        int32 EscapeChance = Agent.SkillLevel / 2 + 20;
        Result.bCaptured = (CaptureRoll > EscapeChance);

        if (Result.bCaptured)
        {
            Agent.bCaptured = true;
            FCoMCounterIntel& CI = CounterIntelMap.FindOrAdd(Agent.TargetWizardId);
            CI.CapturedEnemyAgents.Add(AgentId);
        }

        // Small chance of being killed if capture fails
        if (!Result.bCaptured && CaptureRoll > 90)
        {
            Result.bKilled = true;
            AllAgents.Remove(AgentId);
            // Agent reference is now invalid — return immediately
            return Result;
        }
    }

    // Reset agent mission state (unless captured/killed)
    if (!Result.bCaptured && !Result.bKilled)
    {
        Agent.CurrentMission = ECoMAgentMission::MAX;
        Agent.TargetWizardId = -1;
        Agent.MissionTurnsLeft = 0;
    }

    return Result;
}

bool UCoMEspionageSubsystem::RollDetection(const FCoMSpyAgent& Agent, int32 TargetWizardId)
{
    int32 DetectionChance = 30; // Base 30% chance of detection

    // Target's counterintelligence increases detection
    if (const FCoMCounterIntel* CI = CounterIntelMap.Find(TargetWizardId))
    {
        DetectionChance += CI->SecurityLevel / 3;
        DetectionChance += CI->CounterSpyAgents * 8;
    }

    // Agent skill reduces detection
    DetectionChance -= Agent.SkillLevel / 3;

    // Double agents are more likely to be detected (eventually)
    if (Agent.bDoubleAgent)
    {
        DetectionChance += 10;
    }

    DetectionChance = FMath::Clamp(DetectionChance, 5, 90);

    int32 Roll = RngStream.RandRange(1, 100);
    return Roll <= DetectionChance;
}

void UCoMEspionageSubsystem::CheckLevelUp(FCoMSpyAgent& Agent)
{
    // Every 50 XP = +1 skill level
    int32 LevelsEarned = Agent.Experience / 50;
    int32 NewSkill = FMath::Min(100, Agent.SkillLevel + LevelsEarned);
    if (NewSkill > Agent.SkillLevel)
    {
        Agent.SkillLevel = NewSkill;
        Agent.Experience = Agent.Experience % 50;
    }
}

FString UCoMEspionageSubsystem::GenerateAgentName()
{
    static const TArray<FString> FirstNames = {
        TEXT("Shadow"), TEXT("Whisper"), TEXT("Nightfall"), TEXT("Phantom"),
        TEXT("Dagger"), TEXT("Serpent"), TEXT("Wraith"), TEXT("Viper"),
        TEXT("Storm"), TEXT("Raven"), TEXT("Frost"), TEXT("Thorn"),
        TEXT("Ember"), TEXT("Ash"), TEXT("Silk"), TEXT("Iron")
    };
    static const TArray<FString> Surnames = {
        TEXT("Walker"), TEXT("Blade"), TEXT("Weaver"), TEXT("Hand"),
        TEXT("Eye"), TEXT("Tongue"), TEXT("Mask"), TEXT("Cloak"),
        TEXT("Wind"), TEXT("Fang"), TEXT("Strike"), TEXT("Song"),
        TEXT("Shade"), TEXT("Web"), TEXT("Tread"), TEXT("Veil")
    };

    int32 FI = RngStream.RandRange(0, FirstNames.Num() - 1);
    int32 SI = RngStream.RandRange(0, Surnames.Num() - 1);
    return FString::Printf(TEXT("%s %s"), *FirstNames[FI], *Surnames[SI]);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMEspionageSubsystem::ExportAll(TArray<FCoMSpyAgent>& OutAgents,
                                        TArray<FCoMCounterIntel>& OutCounterIntel,
                                        TArray<int32>& OutCounterIntelWizardIDs,
                                        TArray<FCoMMissionResult>& OutHistory) const
{
    OutAgents.Empty();
    OutAgents.Reserve(AllAgents.Num());
    for (const auto& Pair : AllAgents)
    {
        OutAgents.Add(Pair.Value);
    }

    OutCounterIntel.Empty();
    OutCounterIntelWizardIDs.Empty();
    OutCounterIntel.Reserve(CounterIntelMap.Num());
    OutCounterIntelWizardIDs.Reserve(CounterIntelMap.Num());
    for (const auto& Pair : CounterIntelMap)
    {
        OutCounterIntelWizardIDs.Add(Pair.Key);
        OutCounterIntel.Add(Pair.Value);
    }

    OutHistory = MissionHistory;
}

void UCoMEspionageSubsystem::ImportAll(const TArray<FCoMSpyAgent>& InAgents,
                                        const TArray<FCoMCounterIntel>& InCounterIntel,
                                        const TArray<int32>& InCounterIntelWizardIDs,
                                        const TArray<FCoMMissionResult>& InHistory)
{
    AllAgents.Empty();
    NextAgentId = 1;
    for (const FCoMSpyAgent& Agent : InAgents)
    {
        AllAgents.Add(Agent.AgentId, Agent);
        if (Agent.AgentId >= NextAgentId)
        {
            NextAgentId = Agent.AgentId + 1;
        }
    }

    CounterIntelMap.Empty();
    for (int32 i = 0; i < InCounterIntel.Num() && i < InCounterIntelWizardIDs.Num(); ++i)
    {
        CounterIntelMap.Add(InCounterIntelWizardIDs[i], InCounterIntel[i]);
    }

    MissionHistory = InHistory;

    UE_LOG(LogTemp, Log, TEXT("[EspionageSubsystem] ImportAll: %d agents, %d counter-intel entries imported."),
           AllAgents.Num(), CounterIntelMap.Num());
}
