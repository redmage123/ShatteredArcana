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
	static const int32    DomainExpansionInterval = 10;   // expand every 10 turns
}

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMHeroSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMHeroSubsystem::Deinitialize()
{
	Personalities.Empty();
	Relationships.Empty();
	Dragons.Empty();
	Domains.Empty();
	Eggs.Empty();
	Super::Deinitialize();
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
	const int32 Roll = FMath::RandRange(0, 99);
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

// =====================================================================
// Dragons
// =====================================================================

int32 UCoMHeroSubsystem::AllocateDragonID()
{
	return NextDragonID++;
}

int32 UCoMHeroSubsystem::AllocateDomainID()
{
	return NextDomainID++;
}

int32 UCoMHeroSubsystem::AllocateEggID()
{
	return NextEggID++;
}

int32 UCoMHeroSubsystem::SpawnDragon(int32 TypeID, ECoMPlane Plane, FIntPoint LairPosition, FRandomStream& Rng)
{
	const int32 ID = AllocateDragonID();

	FCoMDragonInstance Dragon;
	Dragon.DragonID     = ID;
	Dragon.TypeID       = TypeID;
	Dragon.PersonalName = FName(NAME_None); // to be named by caller or event
	Dragon.Role         = ECoMDragonRole::Wild;
	Dragon.Age          = 0;
	Dragon.UnitID       = -1; // assigned when placed on map via UCoMUnitSubsystem
	Dragon.DomainID     = -1;
	Dragon.HoardGold    = FFixed64(0, 0);
	Dragon.HoardMana    = FFixed64(0, 0);

	// Random personality values.
	Dragon.TerritorialAggression = FFixed64(Rng.RandRange(20, 80), 0);

	Dragons.Add(ID, Dragon);
	return ID;
}

void UCoMHeroSubsystem::CreateDragonDomain(int32 DragonID, int32 InfluenceRadius)
{
	FCoMDragonInstance* Dragon = Dragons.Find(DragonID);
	if (!Dragon)
	{
		return;
	}

	const int32 DomID = AllocateDomainID();

	FCoMDragonDomain Domain;
	Domain.DomainID        = DomID;
	Domain.RulerDragonID   = DragonID;
	Domain.InfluenceRadius = InfluenceRadius;
	Domain.LairPosition    = FIntPoint(0, 0); // caller should set via dragon's spawn position

	// Compute initial claimed tiles.
	Domain.ClaimedTiles = ComputeClaimedTiles(Domain.LairPosition, InfluenceRadius);

	Domains.Add(DomID, Domain);

	// Link the dragon to this domain and promote to DomainRuler.
	Dragon->DomainID = DomID;
	Dragon->Role     = ECoMDragonRole::DomainRuler;
}

const FCoMDragonInstance* UCoMHeroSubsystem::GetDragon(int32 DragonID) const
{
	return Dragons.Find(DragonID);
}

const FCoMDragonDomain* UCoMHeroSubsystem::GetDomain(int32 DomainID) const
{
	return Domains.Find(DomainID);
}

TArray<const FCoMDragonDomain*> UCoMHeroSubsystem::GetDomainsOnPlane(ECoMPlane Plane) const
{
	TArray<const FCoMDragonDomain*> Result;
	for (const auto& Pair : Domains)
	{
		if (Pair.Value.Plane == Plane)
		{
			Result.Add(&Pair.Value);
		}
	}
	return Result;
}

// =====================================================================
// Dragon Eggs
// =====================================================================

int32 UCoMHeroSubsystem::LayDragonEgg(int32 ParentDragonID, int32 PossessorWizard)
{
	const FCoMDragonInstance* Parent = Dragons.Find(ParentDragonID);
	if (!Parent)
	{
		return -1;
	}

	const int32 ID = AllocateEggID();

	FCoMDragonEgg Egg;
	Egg.EggID           = ID;
	Egg.DragonTypeID    = Parent->TypeID;
	Egg.ParentDragonID  = ParentDragonID;
	Egg.PossessorWizard = PossessorWizard;
	Egg.IncubationTurns = 0;
	Egg.HatchingManaCost = FFixed64(50, 0); // base cost; type-specific overrides later
	Egg.bImprints       = false;

	Eggs.Add(Egg);
	return ID;
}

int32 UCoMHeroSubsystem::HatchEgg(int32 EggID)
{
	for (int32 i = 0; i < Eggs.Num(); ++i)
	{
		if (Eggs[i].EggID == EggID)
		{
			FCoMDragonEgg& Egg = Eggs[i];

			// Require a minimum incubation period (10 turns).
			if (Egg.IncubationTurns < 10)
			{
				return -1;
			}

			// Spawn the hatchling.
			FRandomStream HatchRng(EggID * 7919); // deterministic seed from egg ID
			const int32 NewDragonID = SpawnDragon(Egg.DragonTypeID, Egg.HatchingRealm, FIntPoint(0, 0), HatchRng);

			if (NewDragonID >= 0)
			{
				FCoMDragonInstance* Hatchling = Dragons.Find(NewDragonID);
				if (Hatchling)
				{
					// If the egg imprints, the hatchling becomes a companion.
					if (Egg.bImprints)
					{
						Hatchling->Role = ECoMDragonRole::Companion;
					}
					else
					{
						Hatchling->Role = ECoMDragonRole::Summon;
					}
				}

				OnDragonEggHatched.Broadcast(EggID, NewDragonID);
			}

			Eggs.RemoveAt(i);
			return NewDragonID;
		}
	}

	return -1;
}

// =====================================================================
// Dragon Turn Processing
// =====================================================================

void UCoMHeroSubsystem::ProcessDragonTurn()
{
	// 1. Age all dragons.
	for (auto& Pair : Dragons)
	{
		Pair.Value.Age++;
	}

	// 2. Expand domains periodically.
	for (auto& Pair : Domains)
	{
		FCoMDragonDomain& Domain = Pair.Value;
		const FCoMDragonInstance* Ruler = Dragons.Find(Domain.RulerDragonID);
		if (!Ruler)
		{
			continue;
		}

		// Domain rulers expand influence every N turns based on age.
		if (Ruler->Age > 0 && (Ruler->Age % CoMHeroConstants::DomainExpansionInterval) == 0)
		{
			Domain.InfluenceRadius++;
			Domain.ClaimedTiles = ComputeClaimedTiles(Domain.LairPosition, Domain.InfluenceRadius);
			OnDragonDomainExpanded.Broadcast(Domain.DomainID, Domain.InfluenceRadius);
		}
	}

	// 3. Tick egg incubation.
	for (FCoMDragonEgg& Egg : Eggs)
	{
		Egg.IncubationTurns++;
	}
}

// =====================================================================
// Internal helpers
// =====================================================================

TArray<FIntPoint> UCoMHeroSubsystem::ComputeClaimedTiles(FIntPoint Center, int32 Radius)
{
	TArray<FIntPoint> Tiles;
	const int32 RadiusSq = Radius * Radius;

	for (int32 DX = -Radius; DX <= Radius; ++DX)
	{
		for (int32 DY = -Radius; DY <= Radius; ++DY)
		{
			if ((DX * DX + DY * DY) <= RadiusSq)
			{
				Tiles.Add(FIntPoint(Center.X + DX, Center.Y + DY));
			}
		}
	}

	return Tiles;
}


// ═══════════════════════════════════════════════════════════════════════════
// Hero Tiers — CoM-based four-tier system
// ═══════════════════════════════════════════════════════════════════════════

FCoMHeroTierConfig UCoMHeroSubsystem::GetTierConfig(ECoMHeroTier Tier)
{
	FCoMHeroTierConfig Config;
	Config.Tier = Tier;

	switch (Tier)
	{
	case ECoMHeroTier::Adventurer:
		// Tier 1: Common heroes — CoM basic Heroes (Dwarf, Orc Warrior)
		Config.StatMultiplier = FFixed64(1);     // 1.0x base stats
		Config.AbilitySlots = 1;
		Config.MaxLevel = 6;
		Config.XPMultiplier = FFixed64(1);       // Normal XP curve
		Config.HireCostMin = 25;
		Config.HireCostMax = 50;
		Config.FameRequired = 0;                  // No fame needed
		Config.bHirableFromTavern = true;
		Config.bQuestRequired = false;
		Config.MaxPerWizard = 4;
		Config.BaseLoyalty = FFixed64(80);         // Lower starting loyalty
		break;

	case ECoMHeroTier::Hero:
		// Tier 2: Proven heroes — CoM mid-tier (Bard, Priestess, Beastmaster)
		Config.StatMultiplier = FFixed64(1) + FFixed64(1) / FFixed64(2); // 1.5x
		Config.AbilitySlots = 2;
		Config.MaxLevel = 9;
		Config.XPMultiplier = FFixed64(1) + FFixed64(1) / FFixed64(2); // 1.5x XP needed
		Config.HireCostMin = 100;
		Config.HireCostMax = 200;
		Config.FameRequired = 5;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired = false;
		Config.MaxPerWizard = 3;
		Config.BaseLoyalty = FFixed64(100);
		break;

	case ECoMHeroTier::Champion:
		// Tier 3: Legendary — CoM Champions (Roland, Mortu, Torin)
		Config.StatMultiplier = FFixed64(2);      // 2.0x base stats
		Config.AbilitySlots = 4;
		Config.MaxLevel = 12;
		Config.XPMultiplier = FFixed64(2);        // 2x XP needed
		Config.HireCostMin = 300;
		Config.HireCostMax = 500;
		Config.FameRequired = 15;
		Config.bHirableFromTavern = false;         // Must seek you out
		Config.bQuestRequired = false;             // Attracted by Fame, not quest
		Config.MaxPerWizard = 2;
		Config.BaseLoyalty = FFixed64(120);         // Champions are more loyal
		break;

	case ECoMHeroTier::Demigod:
		// Tier 4: Planar entities — NEW for Shattered Arcana
		Config.StatMultiplier = FFixed64(3);      // 3.0x base stats
		Config.AbilitySlots = 5;
		Config.MaxLevel = 15;
		Config.XPMultiplier = FFixed64(3);        // 3x XP needed
		Config.HireCostMin = 0;                   // Cannot be hired with gold
		Config.HireCostMax = 0;
		Config.FameRequired = 30;                 // High fame + quest
		Config.bHirableFromTavern = false;
		Config.bQuestRequired = true;              // Must complete plane-specific quest
		Config.MaxPerWizard = 1;                   // Only one Demigod per wizard
		Config.BaseLoyalty = FFixed64(150);         // Very loyal once earned
		break;

	default:
		break;
	}

	return Config;
}

ECoMHeroTier UCoMHeroSubsystem::GetHeroTier(int32 HeroUnitID) const
{
	const ECoMHeroTier* Tier = HeroTiers.Find(HeroUnitID);
	return Tier ? *Tier : ECoMHeroTier::Adventurer;
}

void UCoMHeroSubsystem::SetHeroTier(int32 HeroUnitID, ECoMHeroTier Tier)
{
	HeroTiers.Add(HeroUnitID, Tier);

	// Apply tier config to loyalty
	FCoMHeroTierConfig Config = GetTierConfig(Tier);
	FCoMHeroPersonality* Personality = Personalities.Find(HeroUnitID);
	if (Personality)
	{
		Personality->Loyalty = Config.BaseLoyalty;
	}
}

bool UCoMHeroSubsystem::CanRecruitTier(int32 WizardIndex, ECoMHeroTier Tier, int32 WizardFame) const
{
	FCoMHeroTierConfig Config = GetTierConfig(Tier);

	// Check fame requirement
	if (WizardFame < Config.FameRequired)
	{
		return false;
	}

	// Check max per wizard
	int32 CurrentCount = CountHeroesOfTier(WizardIndex, Tier);
	if (CurrentCount >= Config.MaxPerWizard)
	{
		return false;
	}

	return true;
}

int32 UCoMHeroSubsystem::CountHeroesOfTier(int32 WizardIndex, ECoMHeroTier Tier) const
{
	// This requires knowing which heroes belong to which wizard.
	// Cross-reference with UCoMUnitSubsystem — for now count from our tier map.
	int32 Count = 0;
	for (const auto& Pair : HeroTiers)
	{
		if (Pair.Value == Tier)
		{
			// TODO: verify this hero belongs to WizardIndex via UCoMUnitSubsystem
			Count++;
		}
	}
	return Count;
}



// ═══════════════════════════════════════════════════════════════════════════
// Hero Classes — Wizard / Psyker / Warlock class archetypes
// ═══════════════════════════════════════════════════════════════════════════

ECoMHeroClass UCoMHeroSubsystem::GetHeroClass(int32 HeroUnitID) const
{
	const ECoMHeroClass* Cls = HeroClasses.Find(HeroUnitID);
	return Cls ? *Cls : ECoMHeroClass::Fighter;
}

void UCoMHeroSubsystem::SetHeroClass(int32 HeroUnitID, ECoMHeroClass HeroClass)
{
	HeroClasses.Add(HeroUnitID, HeroClass);
}

TArray<ECoMHeroClass> UCoMHeroSubsystem::GetAvailableClasses(ECoMHeroTier Tier, ECoMWizardClass WizardClass)
{
	// Class availability matrix:
	// - Tier 1 (Adventurer): Fighter, Bowman, Bard, basic Psyker/Warlock if matching wizard class
	// - Tier 2 (Hero): All base classes + Psyker/Warlock specializations
	// - Tier 3 (Champion): All + DragonKnight, Warlord, PlanarWalker
	// - Tier 4 (Demigod): Ascendant + any Tier 3 class
	
	struct FClassEntry
	{
		ECoMHeroClass Class;
		ECoMHeroTier MinTier;
		ECoMWizardClass RequiredWizardClass; // MAX = any
	};

	static const TArray<FClassEntry> AllEntries = {
		// Wizard-aligned (available to all wizard classes)
		{ ECoMHeroClass::Fighter,      ECoMHeroTier::Adventurer, ECoMWizardClass::MAX },
		{ ECoMHeroClass::Bowman,       ECoMHeroTier::Adventurer, ECoMWizardClass::MAX },
		{ ECoMHeroClass::Bard,         ECoMHeroTier::Adventurer, ECoMWizardClass::MAX },
		{ ECoMHeroClass::Cleric,    ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Magician,     ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Assassin,     ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Beastmaster,  ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Necromancer,  ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Druid,        ECoMHeroTier::Hero,       ECoMWizardClass::MAX },
		{ ECoMHeroClass::Paladin,      ECoMHeroTier::Champion,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::Warlord,      ECoMHeroTier::Champion,   ECoMWizardClass::MAX },

		// Psyker-aligned (require Psyker wizard class)
		{ ECoMHeroClass::Psyker,       ECoMHeroTier::Adventurer, ECoMWizardClass::Psyker },
		{ ECoMHeroClass::Telepath,     ECoMHeroTier::Hero,       ECoMWizardClass::Psyker },
		{ ECoMHeroClass::Dominator,    ECoMHeroTier::Champion,   ECoMWizardClass::Psyker },
		{ ECoMHeroClass::Empath,       ECoMHeroTier::Hero,       ECoMWizardClass::Psyker },

		// Warlock-aligned (require Warlock wizard class)
		{ ECoMHeroClass::Warlock,      ECoMHeroTier::Adventurer, ECoMWizardClass::Warlock },
		{ ECoMHeroClass::WitchHunter,  ECoMHeroTier::Hero,       ECoMWizardClass::Warlock },
		{ ECoMHeroClass::Demonologist, ECoMHeroTier::Champion,   ECoMWizardClass::Warlock },
		{ ECoMHeroClass::Feysworn,     ECoMHeroTier::Hero,       ECoMWizardClass::Warlock },
		{ ECoMHeroClass::SoulReaper,   ECoMHeroTier::Champion,   ECoMWizardClass::Warlock },

		// High-tier (Champion+)
		{ ECoMHeroClass::DragonKnight, ECoMHeroTier::Champion,   ECoMWizardClass::MAX },
		{ ECoMHeroClass::PlanarWalker, ECoMHeroTier::Champion,   ECoMWizardClass::MAX },

		// Demigod only
		{ ECoMHeroClass::Ascendant,    ECoMHeroTier::Demigod,    ECoMWizardClass::MAX },
	};

	TArray<ECoMHeroClass> Result;
	for (const FClassEntry& Entry : AllEntries)
	{
		// Tier check: hero must be at or above the minimum tier for this class
		if (static_cast<uint8>(Tier) < static_cast<uint8>(Entry.MinTier))
		{
			continue;
		}

		// Wizard class check: MAX means any class can use it
		if (Entry.RequiredWizardClass != ECoMWizardClass::MAX && Entry.RequiredWizardClass != WizardClass)
		{
			continue;
		}

		Result.Add(Entry.Class);
	}

	return Result;
}

