// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIWizardDiplomacy.h -- AI wizard-to-wizard diplomatic decision engine.
// Evaluates relationships, personality, and game state to drive alliance/war/trade AI.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMAIWizardDiplomacy.generated.h"

class UCoMDiplomacySubsystem;
class UCoMMagicSubsystem;
class UCoMUnitSubsystem;
class UCoMCitySubsystem;
class ACoMGameState;
class ACoMPlayerState;
struct FCoMDiplomaticRelation;
struct FCoMDiplomaticPersonality;
struct FCoMWizardMagicState;

// ---------------------------------------------------------------------------
// AI PERSONALITY (extended for diplomacy decisions)
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCoMAIPersonality
{
	GENERATED_BODY()

	/** 0 = peaceful sage, 1 = warmonger. Affects war declaration frequency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Aggressiveness = 0.5f;

	/** 0 = treacherous, 1 = honorable. Affects treaty-breaking and trust thresholds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Trustworthiness = 0.5f;

	/** 0 = generous, 1 = hoards resources. Affects gift-giving and trade generosity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Greed = 0.5f;

	/** 0 = isolationist, 1 = empire builder. Affects expansion and territory contestation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Expansionism = 0.5f;

	/** 0 = military focus, 1 = research focus. Affects spell trade enthusiasm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MagicFocus = 0.5f;
};

// ---------------------------------------------------------------------------
// DIPLOMATIC DECISION (output of evaluation)
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCoMAIDiplomaticDecision
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetWizardId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMDiplomacyAction Action = ECoMDiplomacyAction::MAX;

	/** Spells this AI wizard is willing to offer in a trade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> SpellsToOffer;

	/** Spells this AI wizard wants from the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> SpellsToRequest;

	/** Priority/score — higher = more urgent action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score = 0.0f;
};

// ---------------------------------------------------------------------------
// PLAYER DIPLOMACY EVENT (queued for UI response)
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCoMAIPlayerDiplomacyEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AIWizardId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerWizardId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECoMDiplomacyAction Action = ECoMDiplomacyAction::MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> OfferedSpells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> RequestedSpells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TurnQueued = 0;
};

// ---------------------------------------------------------------------------
// AI WIZARD DIPLOMACY ENGINE
// ---------------------------------------------------------------------------

UCLASS()
class COMAI_API UCoMAIWizardDiplomacy : public UObject
{
	GENERATED_BODY()

public:
	// ---- Initialization ---------------------------------------------------

	/** Populate default personalities for all named wizards. */
	void InitializeDefaultPersonalities();

	/** Override or set a personality for a specific wizard. */
	void SetPersonality(int32 WizardId, const FCoMAIPersonality& Personality);

	/** Get personality for a wizard. Returns default (0.5 all) if unset. */
	FCoMAIPersonality GetPersonality(int32 WizardId) const;

	// ---- Main Entry Point -------------------------------------------------

	/**
	 * Called each AI wizard's turn to evaluate and execute diplomatic actions
	 * against all other non-eliminated wizards. Fires declarations, proposals,
	 * and trades through the DiplomacySubsystem.
	 */
	void ProcessDiplomacy(int32 WizardId, int32 CurrentTurn);

	// ---- Evaluation -------------------------------------------------------

	/**
	 * Evaluate all living wizards and decide what diplomatic action (if any)
	 * to take against each one. Returns sorted by priority (highest first).
	 */
	TArray<FCoMAIDiplomaticDecision> EvaluateAllRelationships(int32 WizardId, int32 CurrentTurn);

	// ---- Player Event Queue -----------------------------------------------

	/** Events queued for the human player to respond to during their turn. */
	UPROPERTY(BlueprintReadOnly, Category = "AI|Diplomacy")
	TArray<FCoMAIPlayerDiplomacyEvent> PendingPlayerEvents;

	/** Pop the next event for a specific player. Returns false if queue is empty. */
	bool PopPlayerEvent(int32 PlayerId, FCoMAIPlayerDiplomacyEvent& OutEvent);

	/** Check if there are pending diplomatic events for a player. */
	bool HasPendingEventsForPlayer(int32 PlayerId) const;

	// ---- Personality Queries (for debug / UI) -----------------------------

	float GetWarLikelihood(int32 WizardId) const;
	float GetTradeLikelihood(int32 WizardId) const;
	float GetAllianceLikelihood(int32 WizardId) const;

private:
	// ---- Per-Target Evaluation -------------------------------------------

	/** Evaluate whether to declare war on TargetId. Returns score (>0 means declare). */
	float EvaluateWarDeclaration(int32 WizardId, int32 TargetId, int32 CurrentTurn,
	                              float OurPower, float TargetPower,
	                              bool bAreNeighbors, bool bTargetIsLeader);

	/** Evaluate whether to propose an alliance with TargetId. */
	float EvaluateAllianceProposal(int32 WizardId, int32 TargetId,
	                                float OurPower, float TargetPower,
	                                bool bShareEnemies);

	/** Evaluate whether to propose a spell trade with TargetId. */
	float EvaluateSpellTrade(int32 WizardId, int32 TargetId);

	/** Evaluate whether to offer peace to TargetId. */
	float EvaluatePeaceOffer(int32 WizardId, int32 TargetId, int32 CurrentTurn,
	                          float OurPower, float TargetPower);

	/** Evaluate whether to send a gift to TargetId. */
	float EvaluateGiftSending(int32 WizardId, int32 TargetId,
	                           float OurPower, float TargetPower);

	// ---- Action Execution ------------------------------------------------

	void ExecuteWarDeclaration(int32 WizardId, int32 TargetId);
	void ExecuteAllianceProposal(int32 WizardId, int32 TargetId);
	void ExecuteSpellTrade(int32 WizardId, int32 TargetId);
	void ExecutePeaceOffer(int32 WizardId, int32 TargetId);
	void ExecuteGiftSending(int32 WizardId, int32 TargetId);

	/** Queue a diplomacy event for a human player (they see it during their turn). */
	void QueuePlayerDiplomacyEvent(int32 AIWizardId, int32 PlayerId,
	                                ECoMDiplomacyAction Action, int32 CurrentTurn,
	                                const TArray<FName>& OfferedSpells = {},
	                                const TArray<FName>& RequestedSpells = {});

	// ---- Helpers ----------------------------------------------------------

	/** Compute aggregate military power for a wizard (army count * strength). */
	float ComputeMilitaryPower(int32 WizardId) const;

	/** Count cities owned by a wizard. */
	int32 CountCities(int32 WizardId) const;

	/** Count controlled mana nodes for a wizard. */
	int32 CountManaNodes(int32 WizardId) const;

	/** Check if two wizards have cities near each other (within NeighborRadius). */
	bool AreNeighbors(int32 WizardA, int32 WizardB) const;

	/** Identify the score leader among non-eliminated wizards. */
	int32 FindScoreLeader() const;

	/** Count non-eliminated wizards. */
	int32 CountAliveWizards() const;

	/** Check if two wizards share a common enemy (both at war with same wizard). */
	bool ShareCommonEnemy(int32 WizardA, int32 WizardB) const;

	/** Check if two wizards share the same primary magic realm. */
	bool ShareRealm(int32 WizardA, int32 WizardB) const;

	/** Check if a target wizard is a human player. */
	bool IsHumanPlayer(int32 WizardId) const;

	/** Check if wizard is eliminated. */
	bool IsEliminated(int32 WizardId) const;

	/** Get subsystem pointers (resolved from game instance). */
	UCoMDiplomacySubsystem* GetDiplomacySub() const;
	UCoMMagicSubsystem* GetMagicSub() const;
	UCoMUnitSubsystem* GetUnitSub() const;
	UCoMCitySubsystem* GetCitySub() const;
	ACoMGameState* GetGameState() const;

	// ---- Data ------------------------------------------------------------

	UPROPERTY()
	TMap<int32, FCoMAIPersonality> WizardPersonalities;

	/** Seeded RNG for deterministic AI decisions. */
	FRandomStream DiplomacyRng;

	static constexpr int32 NEIGHBOR_RADIUS = 12;
	static constexpr int32 MAX_ACTIONS_PER_TURN = 3;
};
