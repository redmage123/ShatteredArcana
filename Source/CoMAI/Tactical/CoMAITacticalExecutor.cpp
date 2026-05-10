// Copyright Mythforge Studios. All Rights Reserved.
// CoMAITacticalExecutor.cpp -- Executes AI strategy via concrete subsystem commands.

#include "Tactical/CoMAITacticalExecutor.h"

#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Diplomacy/CoMDiplomacySubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "Difficulty/CoMAIDifficultyModifier.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"

// Well-known building IDs (placeholders matching the design doc).
// These will be replaced with data-asset lookups once the building catalogue is live.
namespace AIBuildings
{
	static constexpr int32 Granary          = 1;
	static constexpr int32 Marketplace      = 2;
	static constexpr int32 Smithy           = 3;
	static constexpr int32 Library          = 4;
	static constexpr int32 Barracks         = 5;
	static constexpr int32 Stables          = 6;
	static constexpr int32 ShrineOfMagic    = 7;
	static constexpr int32 Palisade         = 8;
	static constexpr int32 StoneFortress    = 9;
	static constexpr int32 SagesGuild       = 10;
	static constexpr int32 FarmersMarket    = 11;
}

// Population threshold below which we prioritize food/growth buildings.
static constexpr int32 LOW_POP_THRESHOLD = 5;

// Minimum army power ratio to attack an enemy stack.
static constexpr float ATTACK_POWER_RATIO = 1.3f;

// Maximum distance to consider for attack/defense targets.
static constexpr int32 MAX_ACTION_RANGE = 20;

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ExecuteTurn(int32 WizardId, const FCoMAIStrategy& Strategy, ECoMAIDifficulty Difficulty)
{
	UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d executing tactical turn. Priority=%d Difficulty=%d"),
	       WizardId, (int32)Strategy.TopPriority, (int32)Difficulty);

	// Cache difficulty for use by ComputeArmyPower and other helpers this turn.
	CachedDifficulty = Difficulty;

	ManageCities(WizardId, Strategy);
	ManageArmies(WizardId, Strategy);
	ManageDiplomacy(WizardId, Strategy);
	ManageResearch(WizardId, Strategy);
	ManageMagic(WizardId, Strategy);
	ConsiderItemForging(WizardId, Strategy);

	// Spirit-meld any unguarded mana node we already control a tile on.
	if (UGameInstance* GI = ResolveGameInstance())
	{
		TryMeldOwnedNodes(WizardId,
			GI->GetSubsystem<UCoMUnitSubsystem>(),
			GI->GetSubsystem<UCoMWorldMapSubsystem>(),
			GI->GetSubsystem<UCoMMagicSubsystem>());
	}
}

// ---------------------------------------------------------------------------
// City management
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ManageCities(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!CitySub) return;

	TArray<const FCoMCityData*> Cities = CitySub->GetCitiesForWizard(WizardId);
	for (const FCoMCityData* City : Cities)
	{
		if (!City) continue;

		// Skip cities that already have something in production
		if (City->CurrentBuildingID != -1) continue;

		const int32 BuildingId = ChooseBuildingForCity(*City, Strategy);
		if (BuildingId != -1)
		{
			CitySub->SetBuildingQueue(City->CityID, BuildingId);
			UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d city %d queued building %d"),
			       WizardId, City->CityID, BuildingId);
		}
	}

	// Consider producing settlers for expansion.
	ConsiderSettlerProduction(WizardId, Strategy, CitySub);
}

int32 UCoMAITacticalExecutor::ChooseBuildingForCity(
	const FCoMCityData& City, const FCoMAIStrategy& Strategy) const
{
	auto HasBuilding = [&City](int32 BuildingId) -> bool
	{
		return City.BuildingIDs.Contains(BuildingId);
	};

	// Rule 1: Always build granary first if population is low
	if (City.Population < LOW_POP_THRESHOLD && !HasBuilding(AIBuildings::Granary))
	{
		return AIBuildings::Granary;
	}

	// Rule 2: If no marketplace, build one (universal economic benefit)
	if (!HasBuilding(AIBuildings::Marketplace))
	{
		return AIBuildings::Marketplace;
	}

	// Priority-driven choices
	switch (Strategy.TopPriority)
	{
	case ECoMAIPriority::BuildEconomy:
	case ECoMAIPriority::Expand:
		if (!HasBuilding(AIBuildings::FarmersMarket)) return AIBuildings::FarmersMarket;
		if (!HasBuilding(AIBuildings::Smithy))        return AIBuildings::Smithy;
		break;

	case ECoMAIPriority::BuildMilitary:
	case ECoMAIPriority::Survive:
		if (!HasBuilding(AIBuildings::Barracks))      return AIBuildings::Barracks;
		if (!HasBuilding(AIBuildings::Stables))       return AIBuildings::Stables;
		if (!HasBuilding(AIBuildings::Palisade))      return AIBuildings::Palisade;
		if (!HasBuilding(AIBuildings::StoneFortress)) return AIBuildings::StoneFortress;
		break;

	case ECoMAIPriority::Research:
		if (!HasBuilding(AIBuildings::Library))       return AIBuildings::Library;
		if (!HasBuilding(AIBuildings::ShrineOfMagic)) return AIBuildings::ShrineOfMagic;
		if (!HasBuilding(AIBuildings::SagesGuild))    return AIBuildings::SagesGuild;
		break;

	case ECoMAIPriority::Diplomacy:
		// Balanced build -- lean toward economy
		if (!HasBuilding(AIBuildings::Library))       return AIBuildings::Library;
		if (!HasBuilding(AIBuildings::Smithy))        return AIBuildings::Smithy;
		break;

	default:
		break;
	}

	// Fallback: build walls if nothing else needed
	if (!HasBuilding(AIBuildings::Palisade)) return AIBuildings::Palisade;

	return -1; // Everything built
}

// ---------------------------------------------------------------------------
// Army management
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ManageArmies(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	UCoMCitySubsystem* CitySub = GI->GetSubsystem<UCoMCitySubsystem>();
	if (!UnitSub || !CitySub) return;

	UCoMWorldMapSubsystem* MapSub = GI->GetSubsystem<UCoMWorldMapSubsystem>();

	// We re-fetch the army list after any operation that may invalidate pointers
	// (e.g. founding a city can disband the settler army).
	bool bNeedRefresh = true;
	while (bNeedRefresh)
	{
		bNeedRefresh = false;
		TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);

		for (const FCoMArmyGroup* Army : Armies)
		{
			if (!Army || Army->UnitIDs.Num() == 0) continue;
			if (Army->bInCombat) continue; // Already engaged

			// --- Settler handling: move toward best founding location or found city ---
			const int32 SettlerId = FindSettlerInArmy(Army, UnitSub);
			if (SettlerId >= 0)
			{
				// Check if we can found a city at the current position using the settler's race tag.
				const FCoMUnitInstance* SettlerUnit = UnitSub->GetUnit(SettlerId);
				if (SettlerUnit && CitySub->CanFoundCityAt(Army->Plane, Army->Layer, Army->Position, SettlerUnit->RaceTag))
				{
					const int32 NewCityId = UnitSub->FoundCityWithSettler(Army->ArmyGroupID, SettlerId);
					if (NewCityId >= 0)
					{
						UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d settler founded city %d at (%d,%d)"),
						       WizardId, NewCityId, Army->Position.X, Army->Position.Y);
						// Army may have been disbanded — re-fetch the army list.
						bNeedRefresh = true;
						break;
					}
				}

				// Not a valid founding location -- move toward best target.
				const FIntPoint SettlerTarget = FindBestSettlerTarget(WizardId, Army->Plane, CitySub, MapSub);
				if (SettlerTarget.X >= 0)
				{
					UnitSub->MoveArmy(Army->ArmyGroupID, SettlerTarget);
					UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d settler army %d moving to (%d,%d)"),
					       WizardId, Army->ArmyGroupID, SettlerTarget.X, SettlerTarget.Y);
				}
				continue; // Don't use settler armies for combat
			}

			const float OurPower = ComputeArmyPower(Army, UnitSub);

			// --- Survival/Defense mode: move armies toward threatened cities ---
			if (Strategy.TopPriority == ECoMAIPriority::Survive ||
			    Strategy.ThreatWizardIds.Num() > 0)
			{
				const FIntPoint DefTarget = FindDefenseTarget(WizardId, Strategy, CitySub);
				if (DefTarget.X >= 0)
				{
					const int32 DistToDef = WrappedDistance(Army->Position, DefTarget);
					if (DistToDef > 1) // Not already there
					{
						UnitSub->MoveArmy(Army->ArmyGroupID, DefTarget);
						UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d army %d moving to defend (%d,%d)"),
						       WizardId, Army->ArmyGroupID, DefTarget.X, DefTarget.Y);
						continue;
					}
				}
			}

			// --- Offensive mode: attack weaker enemies ---
			if (Strategy.TopPriority == ECoMAIPriority::BuildMilitary ||
			    Strategy.MilitaryBudget > 0.4f)
			{
				const FIntPoint AtkTarget = FindAttackTarget(WizardId, Army, UnitSub, CitySub);
				if (AtkTarget.X >= 0)
				{
					// Only attack if we have a power advantage
					TArray<const FCoMArmyGroup*> EnemyArmiesAtTarget =
						UnitSub->GetArmiesAtPosition(Army->Plane, Army->Layer, AtkTarget);

					float EnemyPowerAtTarget = 0.0f;
					for (const FCoMArmyGroup* EnemyArmy : EnemyArmiesAtTarget)
					{
						if (EnemyArmy && EnemyArmy->OwnerWizardIndex != WizardId)
						{
							EnemyPowerAtTarget += ComputeArmyPower(EnemyArmy, UnitSub);
						}
					}

					if (OurPower >= EnemyPowerAtTarget * ATTACK_POWER_RATIO || EnemyPowerAtTarget <= 0.0f)
					{
						UnitSub->MoveArmy(Army->ArmyGroupID, AtkTarget);
						UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d army %d attacking toward (%d,%d)"),
						       WizardId, Army->ArmyGroupID, AtkTarget.X, AtkTarget.Y);
						continue;
					}
				}
			}

			// --- Site / mana-node hunt: nearest uncleared site or guarded node ---
			// Always considered before exploration; gives idle armies real targets.
			if (MapSub)
			{
				const FIntPoint SiteTarget = FindSiteOrNodeTarget(Army, MapSub);
				if (SiteTarget.X >= 0)
				{
					const int32 DistToSite = WrappedDistance(Army->Position, SiteTarget);
					if (DistToSite > 0)
					{
						UnitSub->MoveArmy(Army->ArmyGroupID, SiteTarget);
						UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d army %d hunting site/node at (%d,%d)"),
						       WizardId, Army->ArmyGroupID, SiteTarget.X, SiteTarget.Y);
						continue;
					}
				}
			}

			// --- Expansion / Exploration: move into unexplored territory ---
			// Simple heuristic: move armies away from our cities to explore
			// (A real implementation would query UCoMFogOfWarSubsystem for unrevealed tiles)
			if (Strategy.TopPriority == ECoMAIPriority::Expand)
			{
				// Move roughly toward the center of the map if we are in a corner
				const FIntPoint MapCenter(CoM::MAP_WIDTH / 2, CoM::MAP_HEIGHT / 2);
				const int32 DistToCenter = WrappedDistance(Army->Position, MapCenter);
				if (DistToCenter > 30)
				{
					UnitSub->MoveArmy(Army->ArmyGroupID, MapCenter);
					UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d army %d exploring toward center"),
					       WizardId, Army->ArmyGroupID);
				}
			}
		}
	}

	// --- Merge nearby friendly armies if they are small ---
	// After each successful merge the Armies array is stale, so restart.
	{
		bool bMergedAny = false;
		do
		{
			bMergedAny = false;
			TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);

			for (int32 i = 0; i < Armies.Num() && !bMergedAny; ++i)
			{
				for (int32 j = i + 1; j < Armies.Num(); ++j)
				{
					const FCoMArmyGroup* A = Armies[i];
					const FCoMArmyGroup* B = Armies[j];
					if (!A || !B) continue;

					// Same position, same plane/layer, and combined size fits
					if (A->Position == B->Position &&
					    A->Plane == B->Plane &&
					    A->Layer == B->Layer &&
					    A->UnitIDs.Num() + B->UnitIDs.Num() <= CoM::MAX_ARMY_SIZE)
					{
						UnitSub->MergeArmies(B->ArmyGroupID, A->ArmyGroupID);
						UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d merged armies %d into %d"),
						       WizardId, B->ArmyGroupID, A->ArmyGroupID);
						bMergedAny = true;
						break; // restart outer loop with fresh army list
					}
				}
			}
		} while (bMergedAny);
	}
}

float UCoMAITacticalExecutor::ComputeArmyPower(const FCoMArmyGroup* Army,
                                                 UCoMUnitSubsystem* UnitSub) const
{
	float Power = 0.0f;
	if (!Army) return Power;
	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (Unit)
		{
			float UnitPower = static_cast<float>(Unit->Level * Unit->CurrentHP);
			if (Unit->bIsHero) UnitPower *= 2.0f;
			Power += UnitPower;
		}
	}
	// Apply difficulty combat multiplier for AI armies.
	Power *= UCoMAIDifficultyModifier::GetCombatMultiplier(CachedDifficulty);
	return Power;
}

FIntPoint UCoMAITacticalExecutor::FindAttackTarget(
	int32 WizardId, const FCoMArmyGroup* Army,
	UCoMUnitSubsystem* UnitSub, UCoMCitySubsystem* CitySub) const
{
	FIntPoint BestTarget(-1, -1);
	int32 BestDist = MAX_ACTION_RANGE + 1;

	// Look for enemy armies on the same plane/layer within range
	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		if (i == WizardId) continue;

		TArray<const FCoMArmyGroup*> EnemyArmies = UnitSub->GetArmiesForWizard(i);
		for (const FCoMArmyGroup* Enemy : EnemyArmies)
		{
			if (!Enemy) continue;
			if (Enemy->Plane != Army->Plane || Enemy->Layer != Army->Layer) continue;

			const int32 Dist = WrappedDistance(Army->Position, Enemy->Position);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestTarget = Enemy->Position;
			}
		}

		// Also consider enemy cities as targets
		TArray<const FCoMCityData*> EnemyCities = CitySub->GetCitiesForWizard(i);
		for (const FCoMCityData* City : EnemyCities)
		{
			if (!City) continue;
			if (City->Plane != Army->Plane || City->Layer != Army->Layer) continue;

			const int32 Dist = WrappedDistance(Army->Position, City->Position);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestTarget = City->Position;
			}
		}
	}

	return BestTarget;
}

FIntPoint UCoMAITacticalExecutor::FindDefenseTarget(
	int32 WizardId, const FCoMAIStrategy& Strategy,
	UCoMCitySubsystem* CitySub) const
{
	// Return the position of our city that is closest to any threat wizard's army.
	// (Simplified: just return our first city's position as a rallying point)
	TArray<const FCoMCityData*> OurCities = CitySub->GetCitiesForWizard(WizardId);
	if (OurCities.Num() > 0 && OurCities[0])
	{
		return OurCities[0]->Position;
	}
	return FIntPoint(-1, -1);
}

// ---------------------------------------------------------------------------
// Diplomacy management
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ManageDiplomacy(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMDiplomacySubsystem* DiploSub = GI->GetSubsystem<UCoMDiplomacySubsystem>();
	UCoMUnitSubsystem*      UnitSub  = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!DiploSub || !UnitSub) return;

	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		if (i == WizardId) continue;
		if (!DiploSub->HaveMet(WizardId, i)) continue;

		const bool bAtWar = DiploSub->AreAtWar(WizardId, i);
		const bool bIsThreat = Strategy.ThreatWizardIds.Contains(i);
		const bool bIsAlly = Strategy.AllyWizardIds.Contains(i);
		const int32 Reputation = DiploSub->GetReputation(WizardId, i);

		// --- Propose peace with stronger enemies we are at war with ---
		if (bAtWar && bIsThreat)
		{
			// Only seek peace if the enemy is stronger
			const float TheirPower = Strategy.StrongestEnemyPower;
			if (TheirPower > Strategy.OurMilitaryPower * 1.1f)
			{
				TMap<ECoMResource, int32> EmptyReparations;
				DiploSub->ProposePeace(WizardId, i, EmptyReparations);
				UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d proposes peace to %d (they are stronger)"),
				       WizardId, i);
			}
		}

		// --- Declare war on weaker neighbors who are not allies ---
		if (!bAtWar && !bIsAlly && Strategy.TopPriority == ECoMAIPriority::BuildMilitary)
		{
			// Compute the rival wizard's total military power by iterating all their armies.
			float TheirPower = 0.0f;
			{
				TArray<const FCoMArmyGroup*> RivalArmies = UnitSub->GetArmiesForWizard(i);
				for (const FCoMArmyGroup* RivalArmy : RivalArmies)
				{
					TheirPower += ComputeArmyPower(RivalArmy, UnitSub);
				}
			}
			// Check if they are weak relative to us
			if (Strategy.OurMilitaryPower > Strategy.StrongestEnemyPower * ATTACK_POWER_RATIO
			    && Reputation < -50)
			{
				// Only declare war if we have casus belli or sufficiently bad relations
				if (DiploSub->HasCasusBelli(WizardId, i))
				{
					DiploSub->DeclareWar(WizardId, i);
					UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d declares war on %d"),
					       WizardId, i);
				}
			}
		}

		// --- Propose non-aggression pact with distant non-threatening wizards ---
		if (!bAtWar && !bIsAlly && !bIsThreat && Reputation >= 0)
		{
			const ECoMTreatyType CurrentTreaty = DiploSub->GetTreatyBetween(WizardId, i);
			if (CurrentTreaty == ECoMTreatyType::None)
			{
				FCoMTreatyProposal Proposal;
				Proposal.ProposerWizardId = WizardId;
				Proposal.TargetWizardId = i;
				Proposal.ProposedTreaty = ECoMTreatyType::NonAggression;
				DiploSub->ProposeTreaty(Proposal);
				UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d proposes non-aggression to %d"),
				       WizardId, i);
			}
		}

		// --- Upgrade existing pacts toward alliances with friendly wizards ---
		if (!bAtWar && Reputation > 100)
		{
			const ECoMTreatyType CurrentTreaty = DiploSub->GetTreatyBetween(WizardId, i);
			if (CurrentTreaty == ECoMTreatyType::NonAggression)
			{
				FCoMTreatyProposal Proposal;
				Proposal.ProposerWizardId = WizardId;
				Proposal.TargetWizardId = i;
				Proposal.ProposedTreaty = ECoMTreatyType::TradeAgreement;
				DiploSub->ProposeTreaty(Proposal);
			}
			else if (CurrentTreaty == ECoMTreatyType::TradeAgreement && Reputation > 200)
			{
				FCoMTreatyProposal Proposal;
				Proposal.ProposerWizardId = WizardId;
				Proposal.TargetWizardId = i;
				Proposal.ProposedTreaty = ECoMTreatyType::DefensivePact;
				DiploSub->ProposeTreaty(Proposal);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Research management
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ManageResearch(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMMagicSubsystem* MagicSub = GI->GetSubsystem<UCoMMagicSubsystem>();
	if (!MagicSub) return;

	FCoMWizardMagicState& MagicState = MagicSub->GetWizardMagic(WizardId);

	// Allocate mana to research based on budget
	const int32 AvailableMana = MagicState.ManaPerTurn - MagicState.MaintenanceCost;
	const int32 ResearchMana = FMath::Max(0, FMath::RoundToInt32(AvailableMana * Strategy.ResearchBudget));
	MagicSub->SetResearchAllocation(WizardId, ResearchMana);

	// If not researching anything, pick a spell
	if (MagicState.CurrentResearchSpell.IsNone())
	{
		TArray<FName> Researchable = MagicSub->GetResearchableSpells(WizardId);
		if (Researchable.Num() > 0)
		{
			// Simple heuristic: pick the first available spell.
			// A smarter system would score spells by how well they match the current priority.
			const FName SpellToResearch = Researchable[0];
			MagicSub->StartResearch(WizardId, SpellToResearch);
			UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d started researching %s"),
			       WizardId, *SpellToResearch.ToString());
		}
	}
}

// ---------------------------------------------------------------------------
// Magic management
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ManageMagic(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMMagicSubsystem* MagicSub = GI->GetSubsystem<UCoMMagicSubsystem>();
	if (!MagicSub) return;

	const FCoMWizardMagicState& MagicState = MagicSub->GetWizardMagic(WizardId);

	// Check if we are already casting something
	FCoMSpellCast CurrentCast = MagicSub->GetCurrentCasting(WizardId);
	if (!CurrentCast.SpellId.IsNone())
	{
		// Already casting -- let it finish
		return;
	}

	// If we have excess mana and known spells, try casting a beneficial global spell.
	// For now we only cast if mana is above 50% capacity to avoid draining reserves.
	if (MagicState.CurrentMana < MagicState.MaxMana / 2)
	{
		return;
	}

	// Try each known spell to see if we can cast it as a global enchantment
	for (const FName& SpellId : MagicState.KnownSpells)
	{
		FCoMSpellCast CastParams;
		CastParams.SpellId = SpellId;
		CastParams.CasterWizardId = WizardId;
		CastParams.Scope = ECoMSpellScope::Global;

		if (MagicSub->CanCastSpell(WizardId, SpellId, CastParams))
		{
			MagicSub->CastSpell(CastParams);
			UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d casting global spell %s"),
			       WizardId, *SpellId.ToString());
			break; // One spell per turn
		}
	}
}

// ---------------------------------------------------------------------------
// Settlement management
// ---------------------------------------------------------------------------

int32 UCoMAITacticalExecutor::FindSettlerInArmy(const FCoMArmyGroup* Army,
                                                  UCoMUnitSubsystem* UnitSub) const
{
	if (!Army || !UnitSub) return -1;

	for (int32 UnitID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitID);
		if (Unit && Unit->bIsSettler)
		{
			return UnitID;
		}
	}
	return -1;
}

FIntPoint UCoMAITacticalExecutor::FindBestSettlerTarget(
	int32 WizardId, ECoMPlane Plane,
	UCoMCitySubsystem* CitySub, UCoMWorldMapSubsystem* MapSub) const
{
	if (!CitySub) return FIntPoint(-1, -1);

	// Gather all cities on this plane to avoid placing near them.
	TArray<const FCoMCityData*> AllCitiesOnPlane = CitySub->GetCitiesOnPlane(Plane);

	// Get our cities as starting search points.
	TArray<const FCoMCityData*> OurCities = CitySub->GetCitiesForWizard(WizardId);
	FIntPoint SearchCenter(CoM::MAP_WIDTH / 2, CoM::MAP_HEIGHT / 2);
	if (OurCities.Num() > 0 && OurCities[0])
	{
		SearchCenter = OurCities[0]->Position;
	}

	// Preferred terrain types for founding (good food/production tiles).
	auto IsGoodFoundingTerrain = [](ECoMTerrain Terrain) -> int32
	{
		switch (Terrain)
		{
		case ECoMTerrain::Grassland:   return 5;
		case ECoMTerrain::Plains:      return 4;
		case ECoMTerrain::River:       return 5;
		case ECoMTerrain::Hills:       return 3;
		case ECoMTerrain::Forest:      return 3;
		case ECoMTerrain::Shore:       return 2;
		case ECoMTerrain::Savanna:     return 2;
		default:                       return 1;
		}
	};

	FIntPoint BestTile(-1, -1);
	int32 BestScore = -1;
	const int32 SearchRange = 25;

	for (int32 DY = -SearchRange; DY <= SearchRange; ++DY)
	{
		for (int32 DX = -SearchRange; DX <= SearchRange; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) > SearchRange) continue;

			int32 TX = SearchCenter.X + DX;
			int32 TY = SearchCenter.Y + DY;

			// Wrap X coordinate.
			TX = ((TX % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH;
			if (TY < 0 || TY >= CoM::MAP_HEIGHT) continue;

			const FIntPoint Candidate(TX, TY);

			// Check distance from all existing cities.
			bool bTooClose = false;
			for (const FCoMCityData* City : AllCitiesOnPlane)
			{
				if (City && WrappedDistance(Candidate, City->Position) < CoM::MIN_CITY_DISTANCE)
				{
					bTooClose = true;
					break;
				}
			}
			if (bTooClose) continue;

			// Check terrain suitability.
			int32 TileScore = 0;
			if (MapSub)
			{
				const FCoMTileData* Tile = MapSub->GetTileAtPos(Plane, ECoMMapLayer::Surface, Candidate);
				if (!Tile) continue;

				// Skip impassable terrain.
				if (Tile->Terrain == ECoMTerrain::Ocean || Tile->Terrain == ECoMTerrain::Mountains ||
					Tile->Terrain == ECoMTerrain::Volcano || Tile->Terrain == ECoMTerrain::DeepTrench)
				{
					continue;
				}

				TileScore = IsGoodFoundingTerrain(Tile->Terrain);
			}
			else
			{
				TileScore = 1; // No map data available, accept any non-excluded tile.
			}

			// Prefer tiles closer to our cities but not too close (good expansion distance).
			const int32 DistFromBase = WrappedDistance(Candidate, SearchCenter);
			if (DistFromBase >= CoM::MIN_CITY_DISTANCE && DistFromBase <= 15)
			{
				TileScore += 2; // Bonus for moderate distance.
			}

			if (TileScore > BestScore)
			{
				BestScore = TileScore;
				BestTile = Candidate;
			}
		}
	}

	return BestTile;
}

void UCoMAITacticalExecutor::ConsiderSettlerProduction(
	int32 WizardId, const FCoMAIStrategy& Strategy, UCoMCitySubsystem* CitySub)
{
	if (!CitySub) return;

	// Only produce settlers when expanding.
	if (Strategy.TopPriority != ECoMAIPriority::Expand &&
		Strategy.TopPriority != ECoMAIPriority::BuildEconomy)
	{
		return;
	}

	TArray<const FCoMCityData*> OurCities = CitySub->GetCitiesForWizard(WizardId);

	// Desired city count scales with game phase.
	const int32 DesiredCities = 3 + Strategy.GamePhaseEstimate * 2;
	if (OurCities.Num() >= DesiredCities)
	{
		return;
	}

	// Check if any existing army already has a settler (via unit subsystem).
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMUnitSubsystem* UnitSub = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!UnitSub) return;

	TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);
	for (const FCoMArmyGroup* Army : Armies)
	{
		if (FindSettlerInArmy(Army, UnitSub) >= 0)
		{
			return; // Already have a settler in the field.
		}
	}

	// Find the largest city with sufficient population to produce a settler.
	const FCoMCityData* BestCity = nullptr;
	int32 BestPop = 0;

	for (const FCoMCityData* City : OurCities)
	{
		if (!City) continue;
		if (City->Population >= 4 && City->Population > BestPop)
		{
			BestPop = City->Population;
			BestCity = City;
		}
	}

	if (BestCity)
	{
		const int32 SettlerId = CitySub->ProduceSettler(BestCity->CityID);
		if (SettlerId >= 0)
		{
			UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d produced settler from city %d (pop was %d)"),
			       WizardId, BestCity->CityID, BestPop);
		}
	}
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

UGameInstance* UCoMAITacticalExecutor::ResolveGameInstance() const
{
	// Walk the outer chain to find the GameInstance
	for (UObject* Obj = GetOuter(); Obj != nullptr; Obj = Obj->GetOuter())
	{
		if (UGameInstance* GI = Cast<UGameInstance>(Obj))
		{
			return GI;
		}
		if (UGameInstanceSubsystem* Sub = Cast<UGameInstanceSubsystem>(Obj))
		{
			return Sub->GetGameInstance();
		}
	}
	return nullptr;
}

int32 UCoMAITacticalExecutor::WrappedDistance(FIntPoint A, FIntPoint B)
{
	int32 DX = FMath::Abs(A.X - B.X);
	if (CoM::MAP_WRAP_X)
	{
		DX = FMath::Min(DX, CoM::MAP_WIDTH - DX);
	}
	const int32 DY = FMath::Abs(A.Y - B.Y);
	return DX + DY;
}

// ---------------------------------------------------------------------------
// Site / mana-node targeting
// ---------------------------------------------------------------------------

FIntPoint UCoMAITacticalExecutor::FindSiteOrNodeTarget(
	const FCoMArmyGroup* Army, UCoMWorldMapSubsystem* MapSub) const
{
	if (!Army || !MapSub) return FIntPoint(-1, -1);

	FIntPoint Best(-1, -1);
	int32 BestDist = MAX_ACTION_RANGE + 1;

	// Uncleared sites on this plane.
	for (const FCoMSite& Site : MapSub->GetSitesOnPlane(Army->Plane))
	{
		if (Site.bCleared) continue;
		if (Site.Layer != Army->Layer) continue;

		const int32 D = WrappedDistance(Army->Position, Site.Position);
		if (D < BestDist)
		{
			BestDist = D;
			Best = Site.Position;
		}
	}

	// Guarded mana nodes on this plane (no per-plane registry; sweep tiles).
	// The full sweep is 16k tiles and would be wasteful per army; instead we
	// rely on the existing site list because guarded nodes register no FCoMSite.
	// We do a coarse sweep limited to the search radius for cheap results.
	const int32 R = MAX_ACTION_RANGE;
	for (int32 DY = -R; DY <= R; ++DY)
	{
		for (int32 DX = -R; DX <= R; ++DX)
		{
			if (FMath::Abs(DX) + FMath::Abs(DY) > R) continue;
			const int32 X = Army->Position.X + DX;
			const int32 Y = Army->Position.Y + DY;
			if (Y < 0 || Y >= CoM::MAP_HEIGHT) continue;

			const FCoMTileData* Tile = MapSub->GetTile(Army->Plane, Army->Layer, X, Y);
			if (!Tile || !Tile->bHasManaNode) continue;
			if (Tile->NodeOwnerWizardIndex >= 0) continue; // Already claimed by someone.

			const FIntPoint Pos(((X % CoM::MAP_WIDTH) + CoM::MAP_WIDTH) % CoM::MAP_WIDTH, Y);
			const int32 D = WrappedDistance(Army->Position, Pos);
			if (D < BestDist)
			{
				BestDist = D;
				Best = Pos;
			}
		}
	}

	return Best;
}

void UCoMAITacticalExecutor::TryMeldOwnedNodes(int32 WizardId,
	UCoMUnitSubsystem* UnitSub, UCoMWorldMapSubsystem* MapSub, UCoMMagicSubsystem* MagicSub)
{
	if (!UnitSub || !MapSub || !MagicSub) return;

	const FCoMWizardMagicState& State = MagicSub->GetWizardMagic(WizardId);
	const ECoMSpellRealm Primary = State.PrimaryRealm;

	const TArray<const FCoMArmyGroup*> Armies = UnitSub->GetArmiesForWizard(WizardId);
	for (const FCoMArmyGroup* Army : Armies)
	{
		if (!Army || Army->UnitIDs.Num() == 0) continue;

		const FCoMTileData* Tile = MapSub->GetTile(Army->Plane, Army->Layer,
			Army->Position.X, Army->Position.Y);
		if (!Tile || !Tile->bHasManaNode) continue;
		if (Tile->bNodeGuarded) continue;
		if (Tile->NodeOwnerWizardIndex >= 0) continue;

		// Prefer matching realm; fall back to Arcane (always permitted).
		const ECoMSpellRealm SpiritRealm =
			(Tile->NodeRealm == Primary) ? Primary : ECoMSpellRealm::Arcane;

		if (MagicSub->MeldSpiritWithNode(WizardId, Army->Position, SpiritRealm))
		{
			UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d melded %s spirit at (%d,%d)"),
				WizardId,
				*StaticEnum<ECoMSpellRealm>()->GetDisplayNameTextByValue((int64)SpiritRealm).ToString(),
				Army->Position.X, Army->Position.Y);
		}
	}
}

// ---------------------------------------------------------------------------
// Item forging
// ---------------------------------------------------------------------------

void UCoMAITacticalExecutor::ConsiderItemForging(int32 WizardId, const FCoMAIStrategy& Strategy)
{
	UGameInstance* GI = ResolveGameInstance();
	if (!GI) return;

	UCoMItemSubsystem*  Items = GI->GetSubsystem<UCoMItemSubsystem>();
	UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
	UCoMHeroSubsystem*  Heroes= GI->GetSubsystem<UCoMHeroSubsystem>();
	UCoMUnitSubsystem*  Units = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!Items || !Magic || !Heroes || !Units) return;

	const FCoMWizardMagicState& State = Magic->GetWizardMagic(WizardId);
	// Only forge when comfortably above maintenance and with at least 200 mana on hand.
	if (State.CurrentMana < 200) return;

	// Find this wizard's strongest hero unit.
	int32 BestHeroID = -1;
	int32 BestHeroLevel = 0;
	{
		// Iterate every army; pick the highest-level hero we own.
		const TArray<const FCoMArmyGroup*> Armies = Units->GetArmiesForWizard(WizardId);
		for (const FCoMArmyGroup* Army : Armies)
		{
			if (!Army) continue;
			for (int32 UID : Army->UnitIDs)
			{
				const FCoMUnitInstance* U = Units->GetUnit(UID);
				if (!U || !U->bIsHero) continue;
				if (U->Level > BestHeroLevel)
				{
					BestHeroLevel = U->Level;
					BestHeroID = UID;
				}
			}
		}
	}
	if (BestHeroID < 0) return; // No hero to equip — don't waste mana.

	// Already has a forged weapon? Then don't re-forge each turn.
	FCoMItemInstance Existing;
	if (Items->GetHeroEquippedAt(BestHeroID, ECoMItemSlot::Weapon, Existing)) return;

	// Pick a small, safe power set: +2 Attack + (one blade skill from the catalog).
	TArray<FCoMItemPower> Picked;
	for (const FCoMItemPower& P : Items->GetPowerCatalog())
	{
		if (P.PowerID == FName(TEXT("attack_plus_2"))) { Picked.Add(P); break; }
	}
	for (const FCoMItemPower& P : Items->GetPowerCatalog())
	{
		if (P.PowerID == FName(TEXT("flame_blade"))) { Picked.Add(P); break; }
	}
	if (Picked.Num() < 2) return; // Catalog missing — bail rather than forging garbage.

	const int32 Cost = Items->ComputeForgeCost(Picked, /*bArtifact*/ false);
	if (State.CurrentMana < Cost + 50) return; // Keep a 50-mana buffer.

	const int32 NewID = Items->ForgeItem(WizardId, ECoMItemSlot::Weapon, NAME_None,
		FText::FromString(TEXT("AI Forged Blade")), Picked, false);
	if (NewID == 0) return;

	Magic->SpendManaForSpell(WizardId, ECoMSpellRealm::Arcane, Cost);
	Items->EquipItem(BestHeroID, NewID);

	UE_LOG(LogTemp, Log, TEXT("CoMAI: Wizard %d forged item %d for hero %d (cost %d mana)"),
		WizardId, NewID, BestHeroID, Cost);
}
