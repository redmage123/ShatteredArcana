// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMHeroSubsystem.h"

// =====================================================================
// Fixed-point constants
// =====================================================================

namespace CoMHeroConstants
{
	static const FFixed64 LoyaltyStart(100, 0);
	static const FFixed64 LoyaltyMax(200, 0);
	static const FFixed64 LoyaltyMin(0, 0);
	static const FFixed64 BaseDecay(1, 0);            // 1 per turn
	static const FFixed64 DecayMultiplierAmbitious(2, 0);
	static const FFixed64 DecayMultiplierLoyal(0, 5000); // 0.5
	static const FFixed64 DesertionThresholdHigh(30, 0);
	static const FFixed64 DesertionThresholdCritical(10, 0);
	static const FFixed64 DesertionChanceHigh(5, 0);     // 5%
	static const FFixed64 DesertionChanceCritical(25, 0); // 25%
	static const FFixed64 RelationshipDecayPerTurn(0, 500); // 0.05 per turn
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMHeroSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RngStream.Initialize(FPlatformTime::Cycles());
}

void UCoMHeroSubsystem::Deinitialize()
{
	Personalities.Empty();
	Relationships.Empty();
	HeroTiers.Empty();
	HeroClasses.Empty();
	Super::Deinitialize();
}

// =====================================================================
// Hero Tiers & Classes
// =====================================================================

FCoMHeroTierConfig UCoMHeroSubsystem::GetTierConfig(ECoMHeroTier Tier)
{
	FCoMHeroTierConfig Config;
	Config.Tier = Tier;

	switch (Tier)
	{
	case ECoMHeroTier::Adventurer:
		Config.StatMultiplier = FFixed64(1);
		Config.AbilitySlots   = 1;
		Config.MaxLevel       = 6;
		Config.XPMultiplier   = FFixed64(1);
		Config.HireCostMin    = 25;
		Config.HireCostMax    = 50;
		Config.FameRequired   = 0;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired = false;
		Config.MaxPerWizard   = 4;
		Config.BaseLoyalty    = FFixed64(100);
		break;

	case ECoMHeroTier::Hero:
		Config.StatMultiplier = FFixed64(1, 5000); // 1.5
		Config.AbilitySlots   = 2;
		Config.MaxLevel       = 9;
		Config.XPMultiplier   = FFixed64(1, 5000);
		Config.HireCostMin    = 100;
		Config.HireCostMax    = 200;
		Config.FameRequired   = 5;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired = false;
		Config.MaxPerWizard   = 3;
		Config.BaseLoyalty    = FFixed64(100);
		break;

	case ECoMHeroTier::Champion:
		Config.StatMultiplier = FFixed64(2);
		Config.AbilitySlots   = 3;
		Config.MaxLevel       = 12;
		Config.XPMultiplier   = FFixed64(2);
		Config.HireCostMin    = 300;
		Config.HireCostMax    = 500;
		Config.FameRequired   = 15;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired = false;
		Config.MaxPerWizard   = 2;
		Config.BaseLoyalty    = FFixed64(100);
		break;

	case ECoMHeroTier::Demigod:
		Config.StatMultiplier = FFixed64(3);
		Config.AbilitySlots   = 5;
		Config.MaxLevel       = 15;
		Config.XPMultiplier   = FFixed64(3);
		Config.HireCostMin    = 0;
		Config.HireCostMax    = 0;
		Config.FameRequired   = 40;
		Config.bHirableFromTavern = false;
		Config.bQuestRequired = true;
		Config.MaxPerWizard   = 1;
		Config.BaseLoyalty    = FFixed64(80);
		break;

	default:
		break;
	}

	return Config;
}

ECoMHeroTier UCoMHeroSubsystem::GetHeroTier(int32 HeroUnitID) const
{
	if (const ECoMHeroTier* Found = HeroTiers.Find(HeroUnitID))
	{
		return *Found;
	}
	return ECoMHeroTier::Adventurer;
}

void UCoMHeroSubsystem::SetHeroTier(int32 HeroUnitID, ECoMHeroTier Tier)
{
	HeroTiers.Add(HeroUnitID, Tier);
}

bool UCoMHeroSubsystem::CanRecruitTier(int32 WizardIndex, ECoMHeroTier Tier, int32 WizardFame) const
{
	const FCoMHeroTierConfig Config = GetTierConfig(Tier);

	// Check fame requirement.
	if (WizardFame < Config.FameRequired)
	{
		return false;
	}

	// Check max-per-wizard cap.
	const int32 CurrentCount = CountHeroesOfTier(WizardIndex, Tier);
	if (CurrentCount >= Config.MaxPerWizard)
	{
		return false;
	}

	return true;
}

ECoMHeroClass UCoMHeroSubsystem::GetHeroClass(int32 HeroUnitID) const
{
	if (const ECoMHeroClass* Found = HeroClasses.Find(HeroUnitID))
	{
		return *Found;
	}
	return ECoMHeroClass::Fighter;
}

void UCoMHeroSubsystem::SetHeroClass(int32 HeroUnitID, ECoMHeroClass HeroClass)
{
	HeroClasses.Add(HeroUnitID, HeroClass);
}

TArray<ECoMHeroClass> UCoMHeroSubsystem::GetAvailableClasses(ECoMHeroTier Tier, ECoMWizardClass WizardClass)
{
	// Static class-tier mapping table.
	static const FCoMHeroClassTierEntry ClassTable[] = {
		{ ECoMHeroClass::Fighter,      ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Bowman,        ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Cleric,        ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Magician,      ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Paladin,       ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Assassin,      ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Bard,          ECoMHeroTier::Adventurer, ECoMHeroTier::Hero,      ECoMWizardClass::MAX },
		{ ECoMHeroClass::Beastmaster,   ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Necromancer,   ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Druid,         ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Psyker,        ECoMHeroTier::Hero,       ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::Telepath,      ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Dominator,     ECoMHeroTier::Champion,   ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::Empath,        ECoMHeroTier::Adventurer, ECoMHeroTier::Hero,      ECoMWizardClass::MAX },
		{ ECoMHeroClass::Warlock,       ECoMHeroTier::Hero,       ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::WitchHunter,   ECoMHeroTier::Adventurer, ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Demonologist,  ECoMHeroTier::Champion,   ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::Feysworn,      ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::SoulReaper,    ECoMHeroTier::Champion,   ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::DragonKnight,  ECoMHeroTier::Champion,   ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::PlanarWalker,  ECoMHeroTier::Champion,   ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::Warlord,       ECoMHeroTier::Hero,       ECoMHeroTier::Champion,  ECoMWizardClass::MAX },
		{ ECoMHeroClass::Ascendant,     ECoMHeroTier::Demigod,    ECoMHeroTier::Demigod,   ECoMWizardClass::MAX },
	};

	TArray<ECoMHeroClass> Result;
	for (const auto& Entry : ClassTable)
	{
		// Check tier range.
		if (Tier < Entry.MinTier || Tier > Entry.MaxTier)
		{
			continue;
		}

		// Check wizard class restriction (MAX = any wizard class allowed).
		if (Entry.RequiredWizardClass != ECoMWizardClass::MAX && Entry.RequiredWizardClass != WizardClass)
		{
			continue;
		}

		Result.Add(Entry.HeroClass);
	}
	return Result;
}

// =====================================================================
// Hero Personality & Loyalty
// =====================================================================

void UCoMHeroSubsystem::InitializeHeroPersonality(int32 HeroUnitID, FRandomStream& Rng)
{
	FCoMHeroPersonality P;

	// Pick two distinct personality traits.
	const int32 TraitCount = static_cast<int32>(ECoMPersonalityTrait::Protective) + 1;
	P.PrimaryTrait   = static_cast<ECoMPersonalityTrait>(Rng.RandRange(0, TraitCount - 1));
	P.SecondaryTrait  = P.PrimaryTrait;
	while (P.SecondaryTrait == P.PrimaryTrait)
	{
		P.SecondaryTrait = static_cast<ECoMPersonalityTrait>(Rng.RandRange(0, TraitCount - 1));
	}

	// Random realm preference.
	const int32 RealmCount = static_cast<int32>(ECoMSpellRealm::Glamour) + 1;
	P.PreferredRealm = static_cast<ECoMSpellRealm>(Rng.RandRange(0, RealmCount - 1));

	// Race preferences — random distinct indices. Actual race resolution is
	// handled by the unit subsystem; we store raw int32 indices here.
	P.PreferredRace = Rng.RandRange(0, 13);
	P.DislikedRace  = P.PreferredRace;
	while (P.DislikedRace == P.PreferredRace)
	{
		P.DislikedRace = Rng.RandRange(0, 13);
	}

	// Loyalty starts at 100. Ambition is a random value [30..90].
	P.Loyalty  = CoMHeroConstants::LoyaltyStart;
	P.Ambition = FFixed64(Rng.RandRange(30, 90), 0);

	Personalities.Add(HeroUnitID, P);
}

FCoMHeroPersonality UCoMHeroSubsystem::GetPersonality(int32 HeroUnitID) const
{
	if (const FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID))
	{
		return *Found;
	}
	return FCoMHeroPersonality();
}

FFixed64 UCoMHeroSubsystem::GetLoyalty(int32 HeroUnitID) const
{
	if (const FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID))
	{
		return Found->Loyalty;
	}
	return CoMHeroConstants::LoyaltyStart;
}

void UCoMHeroSubsystem::ModifyLoyalty(int32 HeroUnitID, FFixed64 Delta)
{
	FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID);
	if (!Found)
	{
		return;
	}

	Found->Loyalty = Found->Loyalty + Delta;

	// Clamp [0, 200].
	if (Found->Loyalty < CoMHeroConstants::LoyaltyMin)
	{
		Found->Loyalty = CoMHeroConstants::LoyaltyMin;
	}
	else if (Found->Loyalty > CoMHeroConstants::LoyaltyMax)
	{
		Found->Loyalty = CoMHeroConstants::LoyaltyMax;
	}
}

bool UCoMHeroSubsystem::CheckDesertion(int32 HeroUnitID)
{
	const FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID);
	if (!Found)
	{
		return false;
	}

	const FFixed64 Loyalty = Found->Loyalty;

	FFixed64 DesertionChance(0, 0);
	if (Loyalty < CoMHeroConstants::DesertionThresholdCritical)
	{
		DesertionChance = CoMHeroConstants::DesertionChanceCritical;
	}
	else if (Loyalty < CoMHeroConstants::DesertionThresholdHigh)
	{
		DesertionChance = CoMHeroConstants::DesertionChanceHigh;
	}
	else
	{
		return false; // loyalty is fine
	}

	// Roll 0..99; if roll < chance, the hero deserts.
	const int32 Roll = RngStream.RandRange(0, 99);
	const int32 Threshold = static_cast<int32>(DesertionChance.IsValid()
		? DesertionChance.GetWhole()
		: 0);

	if (Roll < Threshold)
	{
		OnHeroDeserted.Broadcast(HeroUnitID, Loyalty);
		return true;
	}

	return false;
}

// =====================================================================
// Hero Relationships
// =====================================================================

void UCoMHeroSubsystem::UpdateRelationship(int32 HeroA, int32 HeroB, ECoMHeroRelationType Type, FFixed64 StrengthDelta)
{
	// Ensure consistent ordering.
	if (HeroA > HeroB)
	{
		Swap(HeroA, HeroB);
	}

	// Search for an existing relationship of this type.
	for (FCoMHeroRelationship& Rel : Relationships)
	{
		if (Rel.HeroA_UnitID == HeroA && Rel.HeroB_UnitID == HeroB && Rel.Type == Type)
		{
			Rel.RelationStrength = Rel.RelationStrength + StrengthDelta;
			Rel.TurnsSinceLastInteraction = 0;
			return;
		}
	}

	// Create a new relationship.
	FCoMHeroRelationship NewRel;
	NewRel.HeroA_UnitID             = HeroA;
	NewRel.HeroB_UnitID             = HeroB;
	NewRel.Type                     = Type;
	NewRel.RelationStrength         = StrengthDelta;
	NewRel.TurnsSinceLastInteraction = 0;
	Relationships.Add(NewRel);
}

TArray<FCoMHeroRelationship> UCoMHeroSubsystem::GetRelationshipsForHero(int32 HeroUnitID) const
{
	TArray<FCoMHeroRelationship> Result;
	for (const FCoMHeroRelationship& Rel : Relationships)
	{
		if (Rel.HeroA_UnitID == HeroUnitID || Rel.HeroB_UnitID == HeroUnitID)
		{
			Result.Add(Rel);
		}
	}
	return Result;
}

// =====================================================================
// Hero Turn Processing
// =====================================================================

void UCoMHeroSubsystem::ProcessHeroTurn()
{
	// 1. Decay loyalty for every hero.
	for (auto& Pair : Personalities)
	{
		const FFixed64 Decay = ComputeLoyaltyDecay(Pair.Value);
		Pair.Value.Loyalty = Pair.Value.Loyalty - Decay;

		if (Pair.Value.Loyalty < CoMHeroConstants::LoyaltyMin)
		{
			Pair.Value.Loyalty = CoMHeroConstants::LoyaltyMin;
		}
	}

	// 2. Check desertion for heroes with low loyalty.
	TArray<int32> DesertedHeroes;
	for (const auto& Pair : Personalities)
	{
		if (Pair.Value.Loyalty < CoMHeroConstants::DesertionThresholdHigh)
		{
			if (CheckDesertion(Pair.Key))
			{
				DesertedHeroes.Add(Pair.Key);
			}
		}
	}

	// Remove deserted heroes from personality tracking.
	for (const int32 ID : DesertedHeroes)
	{
		Personalities.Remove(ID);
	}

	// 3. Age relationships: increment interaction counter, decay weak relationships.
	for (int32 i = Relationships.Num() - 1; i >= 0; --i)
	{
		FCoMHeroRelationship& Rel = Relationships[i];
		Rel.TurnsSinceLastInteraction++;

		// Slowly decay relationship strength toward zero.
		if (Rel.RelationStrength > FFixed64(0, 0))
		{
			Rel.RelationStrength = Rel.RelationStrength - CoMHeroConstants::RelationshipDecayPerTurn;
			if (Rel.RelationStrength < FFixed64(0, 0))
			{
				Rel.RelationStrength = FFixed64(0, 0);
			}
		}

		// Prune dead relationships.
		if (Rel.RelationStrength <= FFixed64(0, 0) && Rel.TurnsSinceLastInteraction > 20)
		{
			Relationships.RemoveAt(i);
		}
	}
}

FFixed64 UCoMHeroSubsystem::ComputeLoyaltyDecay(const FCoMHeroPersonality& Personality) const
{
	FFixed64 Decay = CoMHeroConstants::BaseDecay;

	// Ambitious heroes lose loyalty twice as fast.
	if (Personality.PrimaryTrait == ECoMPersonalityTrait::Ambitious ||
		Personality.SecondaryTrait == ECoMPersonalityTrait::Ambitious)
	{
		Decay = Decay * CoMHeroConstants::DecayMultiplierAmbitious;
	}

	// Loyal heroes lose loyalty half as fast.
	if (Personality.PrimaryTrait == ECoMPersonalityTrait::Loyal ||
		Personality.SecondaryTrait == ECoMPersonalityTrait::Loyal)
	{
		Decay = Decay * CoMHeroConstants::DecayMultiplierLoyal;
	}

	return Decay;
}

int32 UCoMHeroSubsystem::CountHeroesOfTier(int32 WizardIndex, ECoMHeroTier Tier) const
{
	int32 Count = 0;
	for (const auto& Pair : HeroTiers)
	{
		if (Pair.Value == Tier)
		{
			// TODO: verify ownership via UCoMUnitSubsystem
			// For now, count all heroes of tier (known limitation)
			Count++;
		}
	}
	return Count;
}
