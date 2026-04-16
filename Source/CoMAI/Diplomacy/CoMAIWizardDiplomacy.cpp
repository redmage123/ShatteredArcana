// Copyright Mythforge Studios. All Rights Reserved.
// CoMAIWizardDiplomacy.cpp -- AI wizard-to-wizard diplomatic decision engine.

#include "Diplomacy/CoMAIWizardDiplomacy.h"

#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Wizards/CoMPlayerState.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// INITIALIZATION
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAIWizardDiplomacy::InitializeDefaultPersonalities()
{
	DiplomacyRng.Initialize(42);

	// Named wizard personalities — lore-driven defaults.
	// Index order matches game-creation wizard slots.
	//                                      Aggr  Trust  Greed  Expan  Magic
	struct FNamedWizard { int32 Id; FCoMAIPersonality P; };
	const FNamedWizard Defaults[] =
	{
		{  0, { 0.20f, 0.90f, 0.20f, 0.30f, 0.95f } }, // Merlin     — wise sage
		{  1, { 0.85f, 0.15f, 0.70f, 0.60f, 0.80f } }, // Nekros     — death mage
		{  2, { 0.90f, 0.50f, 0.40f, 0.80f, 0.25f } }, // Pyraxis    — fire warrior
		{  3, { 0.15f, 0.85f, 0.25f, 0.40f, 0.60f } }, // Gaia       — nature druid
		{  4, { 0.70f, 0.30f, 0.80f, 0.70f, 0.50f } }, // Rjak       — chaos warlord
		{  5, { 0.30f, 0.75f, 0.35f, 0.50f, 0.85f } }, // Ariel      — sorcery enchantress
		{  6, { 0.60f, 0.40f, 0.55f, 0.75f, 0.45f } }, // Tlaloc     — nature shaman
		{  7, { 0.50f, 0.60f, 0.30f, 0.35f, 0.70f } }, // Sss'ra     — binding mystic
		{  8, { 0.75f, 0.25f, 0.65f, 0.85f, 0.35f } }, // Kali       — chaos conqueror
		{  9, { 0.25f, 0.80f, 0.40f, 0.25f, 0.75f } }, // Lo Pan     — spirit sage
		{ 10, { 0.65f, 0.35f, 0.75f, 0.60f, 0.55f } }, // Horus      — balanced ruler
		{ 11, { 0.40f, 0.70f, 0.20f, 0.45f, 0.90f } }, // Freya      — life healer
		{ 12, { 0.80f, 0.20f, 0.85f, 0.90f, 0.30f } }, // Tauron     — brute expansionist
		{ 13, { 0.35f, 0.65f, 0.50f, 0.50f, 0.65f } }, // Oberon     — glamour trickster
	};

	for (const FNamedWizard& W : Defaults)
	{
		WizardPersonalities.Add(W.Id, W.P);
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAIWizardDiplomacy: Initialized %d default wizard personalities."),
	       WizardPersonalities.Num());
}

void UCoMAIWizardDiplomacy::SetPersonality(int32 WizardId, const FCoMAIPersonality& Personality)
{
	WizardPersonalities.Add(WizardId, Personality);
}

FCoMAIPersonality UCoMAIWizardDiplomacy::GetPersonality(int32 WizardId) const
{
	const FCoMAIPersonality* Found = WizardPersonalities.Find(WizardId);
	return Found ? *Found : FCoMAIPersonality();
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN ENTRY POINT
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAIWizardDiplomacy::ProcessDiplomacy(int32 WizardId, int32 CurrentTurn)
{
	UE_LOG(LogTemp, Log, TEXT("UCoMAIWizardDiplomacy: Wizard %d evaluating diplomacy (turn %d)..."),
	       WizardId, CurrentTurn);

	TArray<FCoMAIDiplomaticDecision> Decisions = EvaluateAllRelationships(WizardId, CurrentTurn);

	// Sort by score descending — execute highest-priority actions first
	Decisions.Sort([](const FCoMAIDiplomaticDecision& A, const FCoMAIDiplomaticDecision& B)
	{
		return A.Score > B.Score;
	});

	int32 ActionsExecuted = 0;

	for (const FCoMAIDiplomaticDecision& Decision : Decisions)
	{
		if (ActionsExecuted >= MAX_ACTIONS_PER_TURN)
		{
			break;
		}

		if (Decision.Score <= 0.0f || Decision.Action == ECoMDiplomacyAction::MAX)
		{
			continue;
		}

		const bool bTargetIsHuman = IsHumanPlayer(Decision.TargetWizardId);

		switch (Decision.Action)
		{
		case ECoMDiplomacyAction::DeclareWar:
			if (bTargetIsHuman)
			{
				// War declarations against human players happen immediately (no choice to decline)
				ExecuteWarDeclaration(WizardId, Decision.TargetWizardId);
				QueuePlayerDiplomacyEvent(WizardId, Decision.TargetWizardId,
				                          ECoMDiplomacyAction::DeclareWar, CurrentTurn);
			}
			else
			{
				ExecuteWarDeclaration(WizardId, Decision.TargetWizardId);
			}
			++ActionsExecuted;
			break;

		case ECoMDiplomacyAction::ProposeTreaty:
			if (bTargetIsHuman)
			{
				QueuePlayerDiplomacyEvent(WizardId, Decision.TargetWizardId,
				                          ECoMDiplomacyAction::ProposeTreaty, CurrentTurn);
			}
			else
			{
				ExecuteAllianceProposal(WizardId, Decision.TargetWizardId);
			}
			++ActionsExecuted;
			break;

		case ECoMDiplomacyAction::RequestSpellTrade:
			if (bTargetIsHuman)
			{
				QueuePlayerDiplomacyEvent(WizardId, Decision.TargetWizardId,
				                          ECoMDiplomacyAction::RequestSpellTrade, CurrentTurn,
				                          Decision.SpellsToOffer, Decision.SpellsToRequest);
			}
			else
			{
				ExecuteSpellTrade(WizardId, Decision.TargetWizardId);
			}
			++ActionsExecuted;
			break;

		case ECoMDiplomacyAction::OfferPeace:
			if (bTargetIsHuman)
			{
				QueuePlayerDiplomacyEvent(WizardId, Decision.TargetWizardId,
				                          ECoMDiplomacyAction::OfferPeace, CurrentTurn);
			}
			else
			{
				ExecutePeaceOffer(WizardId, Decision.TargetWizardId);
			}
			++ActionsExecuted;
			break;

		case ECoMDiplomacyAction::OfferGift:
			if (bTargetIsHuman)
			{
				// Gifts to human just happen (player is notified)
				ExecuteGiftSending(WizardId, Decision.TargetWizardId);
				QueuePlayerDiplomacyEvent(WizardId, Decision.TargetWizardId,
				                          ECoMDiplomacyAction::OfferGift, CurrentTurn);
			}
			else
			{
				ExecuteGiftSending(WizardId, Decision.TargetWizardId);
			}
			++ActionsExecuted;
			break;

		default:
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UCoMAIWizardDiplomacy: Wizard %d executed %d diplomatic actions."),
	       WizardId, ActionsExecuted);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVALUATE ALL RELATIONSHIPS
// ─────────────────────────────────────────────────────────────────────────────

TArray<FCoMAIDiplomaticDecision> UCoMAIWizardDiplomacy::EvaluateAllRelationships(
	int32 WizardId, int32 CurrentTurn)
{
	TArray<FCoMAIDiplomaticDecision> Decisions;

	const ACoMGameState* GS = GetGameState();
	if (!GS) return Decisions;

	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return Decisions;

	const float OurPower = ComputeMilitaryPower(WizardId);
	const int32 ScoreLeaderId = FindScoreLeader();
	const int32 AliveCount = CountAliveWizards();

	for (int32 TargetId = 0; TargetId < CoM::MAX_WIZARDS; ++TargetId)
	{
		if (TargetId == WizardId) continue;
		if (IsEliminated(TargetId)) continue;

		const ACoMPlayerState* TargetPS = GS->GetWizardByIndex(TargetId);
		if (!TargetPS) continue;

		const float TargetPower = ComputeMilitaryPower(TargetId);
		const bool bAreNeighbors = AreNeighbors(WizardId, TargetId);
		const bool bTargetIsLeader = (TargetId == ScoreLeaderId && AliveCount >= 4);
		const bool bShareEnemies = ShareCommonEnemy(WizardId, TargetId);
		const bool bAtWar = DipSub->AreAtWar(WizardId, TargetId);
		const bool bAllied = DipSub->AreAllied(WizardId, TargetId);

		FCoMAIDiplomaticDecision BestDecision;
		BestDecision.TargetWizardId = TargetId;

		if (bAtWar)
		{
			// At war — only consider peace
			const float PeaceScore = EvaluatePeaceOffer(WizardId, TargetId, CurrentTurn,
			                                             OurPower, TargetPower);
			if (PeaceScore > BestDecision.Score)
			{
				BestDecision.Score = PeaceScore;
				BestDecision.Action = ECoMDiplomacyAction::OfferPeace;
			}
		}
		else if (bAllied)
		{
			// Allied — consider spell trade
			const float TradeScore = EvaluateSpellTrade(WizardId, TargetId);
			if (TradeScore > BestDecision.Score)
			{
				BestDecision.Score = TradeScore;
				BestDecision.Action = ECoMDiplomacyAction::RequestSpellTrade;
			}
		}
		else
		{
			// Not at war, not allied — consider war, alliance, trade, or gift

			// War
			const float WarScore = EvaluateWarDeclaration(WizardId, TargetId, CurrentTurn,
			                                               OurPower, TargetPower,
			                                               bAreNeighbors, bTargetIsLeader);
			if (WarScore > BestDecision.Score)
			{
				BestDecision.Score = WarScore;
				BestDecision.Action = ECoMDiplomacyAction::DeclareWar;
			}

			// Alliance
			const float AllyScore = EvaluateAllianceProposal(WizardId, TargetId,
			                                                   OurPower, TargetPower,
			                                                   bShareEnemies);
			if (AllyScore > BestDecision.Score)
			{
				BestDecision.Score = AllyScore;
				BestDecision.Action = ECoMDiplomacyAction::ProposeTreaty;
			}

			// Spell trade (requires peace or trade treaty)
			const ECoMTreatyType Treaty = DipSub->GetTreatyBetween(WizardId, TargetId);
			if (Treaty == ECoMTreatyType::TradeAgreement
			    || Treaty == ECoMTreatyType::NonAggression
			    || Treaty == ECoMTreatyType::WizardsPact
			    || Treaty == ECoMTreatyType::ResourcePact)
			{
				const float TradeScore = EvaluateSpellTrade(WizardId, TargetId);
				if (TradeScore > BestDecision.Score)
				{
					BestDecision.Score = TradeScore;
					BestDecision.Action = ECoMDiplomacyAction::RequestSpellTrade;
				}
			}

			// Gift
			const float GiftScore = EvaluateGiftSending(WizardId, TargetId,
			                                              OurPower, TargetPower);
			if (GiftScore > BestDecision.Score)
			{
				BestDecision.Score = GiftScore;
				BestDecision.Action = ECoMDiplomacyAction::OfferGift;
			}
		}

		if (BestDecision.Score > 0.0f && BestDecision.Action != ECoMDiplomacyAction::MAX)
		{
			// Populate spell trade details if applicable
			if (BestDecision.Action == ECoMDiplomacyAction::RequestSpellTrade)
			{
				if (UCoMDiplomacySubsystem* DipSubMut = GetDiplomacySub())
				{
					BestDecision.SpellsToOffer = DipSubMut->GetTradeableSpells(WizardId, TargetId);
					BestDecision.SpellsToRequest = DipSubMut->GetTradeableSpells(TargetId, WizardId);

					// Limit to 2 spells each side per trade
					if (BestDecision.SpellsToOffer.Num() > 2)
						BestDecision.SpellsToOffer.SetNum(2);
					if (BestDecision.SpellsToRequest.Num() > 2)
						BestDecision.SpellsToRequest.SetNum(2);
				}
			}

			Decisions.Add(BestDecision);
		}
	}

	return Decisions;
}

// ─────────────────────────────────────────────────────────────────────────────
// WAR DECLARATION EVALUATION
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::EvaluateWarDeclaration(
	int32 WizardId, int32 TargetId, int32 CurrentTurn,
	float OurPower, float TargetPower,
	bool bAreNeighbors, bool bTargetIsLeader)
{
	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return -1.0f;

	const FCoMAIPersonality Pers = GetPersonality(WizardId);
	const int32 Reputation = DipSub->GetReputation(WizardId, TargetId);
	const ECoMTreatyType Treaty = DipSub->GetTreatyBetween(WizardId, TargetId);

	// Never declare war on someone we have a treaty with if we're trustworthy
	if (Treaty != ECoMTreatyType::None && Pers.Trustworthiness > 0.5f)
	{
		return -1.0f;
	}

	// Count criteria met (need at least 2)
	int32 CriteriaMet = 0;

	// 1. No treaty AND reputation very negative
	if (Treaty == ECoMTreatyType::None && Reputation < -200)
	{
		CriteriaMet++;
	}

	// 2. We have 2x military AND personality is aggressive
	if (OurPower > TargetPower * 2.0f && Pers.Aggressiveness > 0.6f)
	{
		CriteriaMet++;
	}

	// 3. Target controls mana nodes we want AND we're neighbors
	if (bAreNeighbors && CountManaNodes(TargetId) > 0 && Pers.Expansionism > 0.5f)
	{
		CriteriaMet++;
	}

	// 4. Target is score leader AND 3+ other wizards exist
	if (bTargetIsLeader && CountAliveWizards() >= 4)
	{
		CriteriaMet++;
	}

	// 5. Target broke a treaty with us in last 20 turns
	// (check via high aggression penalty in the diplomatic relation)
	const FCoMDiplomaticRelation& Rel = const_cast<UCoMDiplomacySubsystem*>(DipSub)->GetRelation(WizardId, TargetId);
	if (Rel.TreatiesBroken > 0 && CurrentTurn - Rel.FirstContactTurn < 20)
	{
		CriteriaMet++;
	}

	if (CriteriaMet < 2)
	{
		return -1.0f;
	}

	// Score = criteria count * aggressiveness, penalized by trustworthiness if treaty exists
	float Score = CriteriaMet * 10.0f * Pers.Aggressiveness;

	// Bonus if we're much stronger
	if (OurPower > 0.0f && TargetPower > 0.0f)
	{
		Score += FMath::Clamp((OurPower / TargetPower - 1.0f) * 5.0f, 0.0f, 15.0f);
	}

	// Penalty if we already have many wars
	const int32 NumCurrentWars = DipSub->GetEnemies(WizardId).Num();
	Score -= NumCurrentWars * 8.0f;

	// Low-trust wizards more willing to attack
	Score += (1.0f - Pers.Trustworthiness) * 5.0f;

	return Score;
}

// ─────────────────────────────────────────────────────────────────────────────
// ALLIANCE PROPOSAL EVALUATION
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::EvaluateAllianceProposal(
	int32 WizardId, int32 TargetId,
	float OurPower, float TargetPower,
	bool bShareEnemies)
{
	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return -1.0f;

	const FCoMAIPersonality Pers = GetPersonality(WizardId);
	const int32 Reputation = DipSub->GetReputation(WizardId, TargetId);

	int32 CriteriaMet = 0;

	// 1. Shared enemy
	if (bShareEnemies)
	{
		CriteriaMet++;
	}

	// 2. Same primary realm affinity
	if (ShareRealm(WizardId, TargetId))
	{
		CriteriaMet++;
	}

	// 3. Reputation > 300 and not at war
	if (Reputation > 300)
	{
		CriteriaMet++;
	}

	// 4. Target is much stronger and we need protection
	if (TargetPower > OurPower * 1.5f && DipSub->GetEnemies(WizardId).Num() > 0)
	{
		CriteriaMet++;
	}

	// 5. Successful trade history (gifts given > 0)
	const FCoMDiplomaticRelation& Rel = const_cast<UCoMDiplomacySubsystem*>(DipSub)->GetRelation(WizardId, TargetId);
	if (Rel.GiftsGivenAtoB > 0 || Rel.GiftsGivenBtoA > 0)
	{
		CriteriaMet++;
	}

	if (CriteriaMet < 2)
	{
		return -1.0f;
	}

	float Score = CriteriaMet * 8.0f;

	// Peaceful / trusting wizards prefer alliances
	Score += (1.0f - Pers.Aggressiveness) * 6.0f;
	Score += Pers.Trustworthiness * 4.0f;

	// Bonus from positive reputation
	Score += FMath::Clamp(Reputation / 100.0f, 0.0f, 5.0f);

	// Penalty if already allied with many (don't overcommit)
	const int32 NumAllies = DipSub->GetAllies(WizardId).Num();
	Score -= NumAllies * 3.0f;

	return Score;
}

// ─────────────────────────────────────────────────────────────────────────────
// SPELL TRADE EVALUATION
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::EvaluateSpellTrade(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return -1.0f;

	const FCoMAIPersonality Pers = GetPersonality(WizardId);

	// Must have treaty
	const ECoMTreatyType Treaty = DipSub->GetTreatyBetween(WizardId, TargetId);
	if (Treaty == ECoMTreatyType::None || Treaty == ECoMTreatyType::War)
	{
		return -1.0f;
	}

	// Check both sides have tradeable spells
	TArray<FName> WeOffer = DipSub->GetTradeableSpells(WizardId, TargetId);
	TArray<FName> TheyOffer = DipSub->GetTradeableSpells(TargetId, WizardId);

	if (WeOffer.Num() == 0 || TheyOffer.Num() == 0)
	{
		return -1.0f;
	}

	// Random chance based on trade personality (30-70% mapped from MagicFocus)
	const float TradeChance = 0.30f + Pers.MagicFocus * 0.40f;
	const float Roll = DiplomacyRng.FRand();
	if (Roll > TradeChance)
	{
		return -1.0f;
	}

	float Score = 5.0f + Pers.MagicFocus * 10.0f;

	// Greed reduces enthusiasm
	Score -= Pers.Greed * 5.0f;

	// More tradeable spells = higher value
	Score += FMath::Min(TheyOffer.Num(), 5) * 2.0f;

	return Score;
}

// ─────────────────────────────────────────────────────────────────────────────
// PEACE OFFER EVALUATION
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::EvaluatePeaceOffer(
	int32 WizardId, int32 TargetId, int32 CurrentTurn,
	float OurPower, float TargetPower)
{
	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return -1.0f;

	// Must be at war
	if (!DipSub->AreAtWar(WizardId, TargetId))
	{
		return -1.0f;
	}

	const FCoMAIPersonality Pers = GetPersonality(WizardId);
	const FCoMDiplomaticRelation& Rel = const_cast<UCoMDiplomacySubsystem*>(DipSub)->GetRelation(WizardId, TargetId);

	int32 CriteriaMet = 0;

	// 1. Losing the war (they have more military)
	if (TargetPower > OurPower * 1.2f)
	{
		CriteriaMet++;
	}

	// 2. War has lasted 10+ turns with no progress
	const int32 WarDuration = (Rel.LastWarStartTurn >= 0) ? (CurrentTurn - Rel.LastWarStartTurn) : 0;
	if (WarDuration >= 10)
	{
		CriteriaMet++;
	}

	// 3. A third wizard is threatening us (we have multiple wars)
	if (DipSub->GetEnemies(WizardId).Num() >= 2)
	{
		CriteriaMet++;
	}

	if (CriteriaMet == 0)
	{
		return -1.0f;
	}

	float Score = CriteriaMet * 10.0f;

	// Peaceful wizards want peace sooner
	Score += (1.0f - Pers.Aggressiveness) * 8.0f;

	// War weariness increases desire for peace
	Score += FMath::Min(WarDuration, 20) * 0.5f;

	// If severely outpowered, very eager for peace
	if (OurPower > 0.0f && TargetPower > OurPower * 2.0f)
	{
		Score += 15.0f;
	}

	return Score;
}

// ─────────────────────────────────────────────────────────────────────────────
// GIFT SENDING EVALUATION
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::EvaluateGiftSending(
	int32 WizardId, int32 TargetId,
	float OurPower, float TargetPower)
{
	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return -1.0f;

	const FCoMAIPersonality Pers = GetPersonality(WizardId);
	const int32 Reputation = DipSub->GetReputation(WizardId, TargetId);

	// Gift-sending criteria: reputation is undecided (-100 to +100)
	if (Reputation < -100 || Reputation > 100)
	{
		return -1.0f;
	}

	// Greedy wizards don't send gifts
	if (Pers.Greed > 0.7f)
	{
		return -1.0f;
	}

	// Need excess resources — check gold
	const ACoMGameState* GS = GetGameState();
	if (!GS) return -1.0f;

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	if (!PS || PS->Gold < 50)
	{
		return -1.0f;
	}

	float Score = 5.0f;

	// Want to prevent them joining enemy alliance
	const TArray<int32> OurEnemies = DipSub->GetEnemies(WizardId);
	for (int32 EnemyId : OurEnemies)
	{
		if (DipSub->AreAllied(TargetId, EnemyId))
		{
			Score += 8.0f; // Urgent — bribe away from enemy alliance
			break;
		}
	}

	// Generous wizards more likely to send gifts
	Score += (1.0f - Pers.Greed) * 5.0f;

	// Higher trustworthiness makes diplomatic gestures more appealing
	Score += Pers.Trustworthiness * 3.0f;

	return Score;
}

// ─────────────────────────────────────────────────────────────────────────────
// ACTION EXECUTION
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAIWizardDiplomacy::ExecuteWarDeclaration(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return;

	DipSub->DeclareWar(WizardId, TargetId);

	UE_LOG(LogTemp, Warning,
	       TEXT("UCoMAIWizardDiplomacy: Wizard %d DECLARES WAR on Wizard %d!"),
	       WizardId, TargetId);
}

void UCoMAIWizardDiplomacy::ExecuteAllianceProposal(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return;

	FCoMTreatyProposal Proposal;
	Proposal.ProposerWizardId = WizardId;
	Proposal.TargetWizardId = TargetId;
	Proposal.ProposedTreaty = ECoMTreatyType::MilitaryAlliance;

	const int32 ProposalId = DipSub->ProposeTreaty(Proposal);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAIWizardDiplomacy: Wizard %d proposes Military Alliance to Wizard %d (proposal %d)."),
	       WizardId, TargetId, ProposalId);

	// AI targets auto-evaluate proposals through DiplomacySubsystem::ProcessAIDiplomacy
}

void UCoMAIWizardDiplomacy::ExecuteSpellTrade(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return;

	TArray<FName> WeOffer = DipSub->GetTradeableSpells(WizardId, TargetId);
	TArray<FName> TheyOffer = DipSub->GetTradeableSpells(TargetId, WizardId);

	// Pick 1-2 spells each side
	if (WeOffer.Num() > 2) WeOffer.SetNum(2);
	if (TheyOffer.Num() > 2) TheyOffer.SetNum(2);

	if (WeOffer.Num() == 0 || TheyOffer.Num() == 0) return;

	const bool bAccepted = DipSub->ProposeSpellTrade(WizardId, TargetId, WeOffer, TheyOffer);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAIWizardDiplomacy: Wizard %d proposes spell trade with Wizard %d — %s. "
	            "Offered %d spells, requested %d spells."),
	       WizardId, TargetId, bAccepted ? TEXT("ACCEPTED") : TEXT("REJECTED"),
	       WeOffer.Num(), TheyOffer.Num());
}

void UCoMAIWizardDiplomacy::ExecutePeaceOffer(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return;

	// Offer peace with modest reparations if we're weaker
	TMap<ECoMResource, int32> Reparations;
	const float OurPower = ComputeMilitaryPower(WizardId);
	const float TheirPower = ComputeMilitaryPower(TargetId);

	if (TheirPower > OurPower * 1.5f)
	{
		// We're much weaker — offer gold as reparation
		const ACoMGameState* GS = GetGameState();
		if (GS)
		{
			const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
			if (PS && PS->Gold > 20)
			{
				Reparations.Add(ECoMResource::GoldOre, FMath::Min(PS->Gold / 4, 50));
			}
		}
	}

	const int32 ProposalId = DipSub->ProposePeace(WizardId, TargetId, Reparations);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAIWizardDiplomacy: Wizard %d offers peace to Wizard %d (proposal %d)."),
	       WizardId, TargetId, ProposalId);
}

void UCoMAIWizardDiplomacy::ExecuteGiftSending(int32 WizardId, int32 TargetId)
{
	UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return;

	const ACoMGameState* GS = GetGameState();
	if (!GS) return;

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	if (!PS || PS->Gold < 30) return;

	// Gift 10-25% of our gold, at least 10
	const int32 GiftAmount = FMath::Max(10, PS->Gold / FMath::RandRange(4, 10));

	TMap<ECoMResource, int32> GiftResources;
	GiftResources.Add(ECoMResource::GoldOre, GiftAmount);

	// Also gift mana if we have plenty
	const int32 ManaGift = (PS->Mana > 50) ? FMath::Min(PS->Mana / 5, 20) : 0;

	DipSub->SendGift(WizardId, TargetId, GiftResources, ManaGift);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAIWizardDiplomacy: Wizard %d sends gift to Wizard %d — %d gold, %d mana."),
	       WizardId, TargetId, GiftAmount, ManaGift);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLAYER EVENT QUEUE
// ─────────────────────────────────────────────────────────────────────────────

void UCoMAIWizardDiplomacy::QueuePlayerDiplomacyEvent(
	int32 AIWizardId, int32 PlayerId,
	ECoMDiplomacyAction Action, int32 CurrentTurn,
	const TArray<FName>& OfferedSpells, const TArray<FName>& RequestedSpells)
{
	FCoMAIPlayerDiplomacyEvent Event;
	Event.AIWizardId = AIWizardId;
	Event.PlayerWizardId = PlayerId;
	Event.Action = Action;
	Event.OfferedSpells = OfferedSpells;
	Event.RequestedSpells = RequestedSpells;
	Event.TurnQueued = CurrentTurn;

	PendingPlayerEvents.Add(Event);

	UE_LOG(LogTemp, Log,
	       TEXT("UCoMAIWizardDiplomacy: Queued diplomacy event for player %d from AI %d — action %d."),
	       PlayerId, AIWizardId, static_cast<int32>(Action));
}

bool UCoMAIWizardDiplomacy::PopPlayerEvent(int32 PlayerId, FCoMAIPlayerDiplomacyEvent& OutEvent)
{
	for (int32 i = 0; i < PendingPlayerEvents.Num(); ++i)
	{
		if (PendingPlayerEvents[i].PlayerWizardId == PlayerId)
		{
			OutEvent = PendingPlayerEvents[i];
			PendingPlayerEvents.RemoveAt(i);
			return true;
		}
	}
	return false;
}

bool UCoMAIWizardDiplomacy::HasPendingEventsForPlayer(int32 PlayerId) const
{
	for (const FCoMAIPlayerDiplomacyEvent& Evt : PendingPlayerEvents)
	{
		if (Evt.PlayerWizardId == PlayerId)
		{
			return true;
		}
	}
	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// PERSONALITY QUERIES
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::GetWarLikelihood(int32 WizardId) const
{
	const FCoMAIPersonality P = GetPersonality(WizardId);
	// Aggressive + untrusting + expansionist = war-hungry
	return FMath::Clamp(P.Aggressiveness * 0.5f + P.Expansionism * 0.3f + (1.0f - P.Trustworthiness) * 0.2f,
	                     0.0f, 1.0f);
}

float UCoMAIWizardDiplomacy::GetTradeLikelihood(int32 WizardId) const
{
	const FCoMAIPersonality P = GetPersonality(WizardId);
	// Magic-focused + generous = trade-friendly
	return FMath::Clamp(P.MagicFocus * 0.5f + (1.0f - P.Greed) * 0.3f + P.Trustworthiness * 0.2f,
	                     0.0f, 1.0f);
}

float UCoMAIWizardDiplomacy::GetAllianceLikelihood(int32 WizardId) const
{
	const FCoMAIPersonality P = GetPersonality(WizardId);
	// Trustworthy + non-aggressive = alliance-friendly
	return FMath::Clamp(P.Trustworthiness * 0.4f + (1.0f - P.Aggressiveness) * 0.3f + (1.0f - P.Greed) * 0.3f,
	                     0.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

float UCoMAIWizardDiplomacy::ComputeMilitaryPower(int32 WizardId) const
{
	const UCoMUnitSubsystem* UnitSub = GetUnitSub();
	if (!UnitSub) return 0.0f;

	TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);
	float TotalPower = 0.0f;
	for (const FCoMArmyGroup* Army : Armies)
	{
		if (Army)
		{
			TotalPower += Army->UnitIDs.Num() * 10.0f; // Rough power estimate
		}
	}

	// Add city count as a power factor (cities contribute garrison)
	TotalPower += CountCities(WizardId) * 15.0f;

	return TotalPower;
}

int32 UCoMAIWizardDiplomacy::CountCities(int32 WizardId) const
{
	const UCoMCitySubsystem* CitySub = GetCitySub();
	if (!CitySub) return 0;
	return CitySub->GetCitiesForWizard(WizardId).Num();
}

int32 UCoMAIWizardDiplomacy::CountManaNodes(int32 WizardId) const
{
	const UCoMMagicSubsystem* MagicSub = GetMagicSub();
	if (!MagicSub) return 0;

	const FCoMWizardMagicState& MagicState = const_cast<UCoMMagicSubsystem*>(MagicSub)->GetWizardMagic(WizardId);
	return MagicState.ControlledNodes.Num();
}

bool UCoMAIWizardDiplomacy::AreNeighbors(int32 WizardA, int32 WizardB) const
{
	const UCoMCitySubsystem* CitySub = GetCitySub();
	if (!CitySub) return false;

	TArray<const FCoMCityData*> CitiesA = CitySub->GetCitiesForWizard(WizardA);
	TArray<const FCoMCityData*> CitiesB = CitySub->GetCitiesForWizard(WizardB);

	for (const FCoMCityData* CityA : CitiesA)
	{
		if (!CityA) continue;
		for (const FCoMCityData* CityB : CitiesB)
		{
			if (!CityB) continue;

			// Wrapped Manhattan distance (160-wide map)
			const int32 DX = FMath::Abs(CityA->Position.X - CityB->Position.X);
			const int32 DY = FMath::Abs(CityA->Position.Y - CityB->Position.Y);
			const int32 WDX = FMath::Min(DX, 160 - DX);
			const int32 Dist = WDX + DY;

			if (Dist <= NEIGHBOR_RADIUS)
			{
				return true;
			}
		}
	}

	return false;
}

int32 UCoMAIWizardDiplomacy::FindScoreLeader() const
{
	const ACoMGameState* GS = GetGameState();
	if (!GS) return -1;

	int32 BestId = -1;
	int32 BestScore = -1;

	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		const ACoMPlayerState* PS = GS->GetWizardByIndex(i);
		if (!PS || PS->bIsEliminated) continue;

		if (PS->VictoryPoints > BestScore)
		{
			BestScore = PS->VictoryPoints;
			BestId = i;
		}
	}

	return BestId;
}

int32 UCoMAIWizardDiplomacy::CountAliveWizards() const
{
	const ACoMGameState* GS = GetGameState();
	if (!GS) return 0;

	int32 Count = 0;
	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		const ACoMPlayerState* PS = GS->GetWizardByIndex(i);
		if (PS && !PS->bIsEliminated)
		{
			++Count;
		}
	}
	return Count;
}

bool UCoMAIWizardDiplomacy::ShareCommonEnemy(int32 WizardA, int32 WizardB) const
{
	const UCoMDiplomacySubsystem* DipSub = GetDiplomacySub();
	if (!DipSub) return false;

	const TArray<int32> EnemiesA = DipSub->GetEnemies(WizardA);
	const TArray<int32> EnemiesB = DipSub->GetEnemies(WizardB);

	for (int32 EnemyA : EnemiesA)
	{
		if (EnemiesB.Contains(EnemyA))
		{
			return true;
		}
	}

	return false;
}

bool UCoMAIWizardDiplomacy::ShareRealm(int32 WizardA, int32 WizardB) const
{
	const UCoMMagicSubsystem* MagicSub = GetMagicSub();
	if (!MagicSub) return false;

	const FCoMWizardMagicState& MagicA = const_cast<UCoMMagicSubsystem*>(MagicSub)->GetWizardMagic(WizardA);
	const FCoMWizardMagicState& MagicB = const_cast<UCoMMagicSubsystem*>(MagicSub)->GetWizardMagic(WizardB);

	return MagicA.PrimaryRealm == MagicB.PrimaryRealm && MagicA.PrimaryRealm != ECoMSpellRealm::None;
}

bool UCoMAIWizardDiplomacy::IsHumanPlayer(int32 WizardId) const
{
	const ACoMGameState* GS = GetGameState();
	if (!GS) return false;

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	return PS && PS->bIsHumanPlayer;
}

bool UCoMAIWizardDiplomacy::IsEliminated(int32 WizardId) const
{
	const ACoMGameState* GS = GetGameState();
	if (!GS) return true;

	const ACoMPlayerState* PS = GS->GetWizardByIndex(WizardId);
	return !PS || PS->bIsEliminated;
}

// ─────────────────────────────────────────────────────────────────────────────
// SUBSYSTEM ACCESSORS
// ─────────────────────────────────────────────────────────────────────────────

UCoMDiplomacySubsystem* UCoMAIWizardDiplomacy::GetDiplomacySub() const
{
	const UGameInstance* GI = GetTypedOuter<UGameInstance>();
	if (!GI)
	{
		// Walk through outer chain — this object is owned by UCoMAISubsystem which is a GI subsystem
		const UObject* Outer = GetOuter();
		while (Outer)
		{
			GI = Cast<UGameInstance>(Outer);
			if (GI) break;
			Outer = Outer->GetOuter();
		}
	}
	return GI ? GI->GetSubsystem<UCoMDiplomacySubsystem>() : nullptr;
}

UCoMMagicSubsystem* UCoMAIWizardDiplomacy::GetMagicSub() const
{
	const UGameInstance* GI = GetTypedOuter<UGameInstance>();
	if (!GI)
	{
		const UObject* Outer = GetOuter();
		while (Outer)
		{
			GI = Cast<UGameInstance>(Outer);
			if (GI) break;
			Outer = Outer->GetOuter();
		}
	}
	return GI ? GI->GetSubsystem<UCoMMagicSubsystem>() : nullptr;
}

UCoMUnitSubsystem* UCoMAIWizardDiplomacy::GetUnitSub() const
{
	const UGameInstance* GI = GetTypedOuter<UGameInstance>();
	if (!GI)
	{
		const UObject* Outer = GetOuter();
		while (Outer)
		{
			GI = Cast<UGameInstance>(Outer);
			if (GI) break;
			Outer = Outer->GetOuter();
		}
	}
	return GI ? GI->GetSubsystem<UCoMUnitSubsystem>() : nullptr;
}

UCoMCitySubsystem* UCoMAIWizardDiplomacy::GetCitySub() const
{
	const UGameInstance* GI = GetTypedOuter<UGameInstance>();
	if (!GI)
	{
		const UObject* Outer = GetOuter();
		while (Outer)
		{
			GI = Cast<UGameInstance>(Outer);
			if (GI) break;
			Outer = Outer->GetOuter();
		}
	}
	return GI ? GI->GetSubsystem<UCoMCitySubsystem>() : nullptr;
}

ACoMGameState* UCoMAIWizardDiplomacy::GetGameState() const
{
	const UGameInstance* GI = GetTypedOuter<UGameInstance>();
	if (!GI)
	{
		const UObject* Outer = GetOuter();
		while (Outer)
		{
			GI = Cast<UGameInstance>(Outer);
			if (GI) break;
			Outer = Outer->GetOuter();
		}
	}
	if (!GI) return nullptr;

	UWorld* World = GI->GetWorld();
	if (!World) return nullptr;

	return Cast<ACoMGameState>(World->GetGameState());
}
