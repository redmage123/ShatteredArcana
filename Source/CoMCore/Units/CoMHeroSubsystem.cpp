// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMHeroSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Wizards/CoMPlayerState.h"
#include "CoMCore/Turn/CoMTurnSubsystem.h"
#include "Engine/World.h"

// =====================================================================
// Subsystem lifecycle
// =====================================================================

void UCoMHeroSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RngStream.Initialize(0x4865726F);
}

void UCoMHeroSubsystem::Deinitialize()
{
	Personalities.Empty();
	Relationships.Empty();
	HeroTiers.Empty();
	HeroClasses.Empty();
	HeroOwners.Empty();
	PendingOffers.Empty();
	NextOfferID = 1;
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
		Config.StatMultiplier     = FFixed64(1);
		Config.AbilitySlots       = 1;
		Config.MaxLevel           = 6;
		Config.XPMultiplier       = FFixed64(1);
		Config.HireCostMin        = 25;
		Config.HireCostMax        = 50;
		Config.FameRequired       = 0;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired     = false;
		Config.MaxPerWizard       = 4;
		Config.BaseLoyalty        = FFixed64(100);
		break;

	case ECoMHeroTier::Hero:
		Config.StatMultiplier     = FFixed64(1.5f);
		Config.AbilitySlots       = 2;
		Config.MaxLevel           = 9;
		Config.XPMultiplier       = FFixed64(1.5f);
		Config.HireCostMin        = 100;
		Config.HireCostMax        = 200;
		Config.FameRequired       = 5;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired     = false;
		Config.MaxPerWizard       = 3;
		Config.BaseLoyalty        = FFixed64(100);
		break;

	case ECoMHeroTier::Champion:
		Config.StatMultiplier     = FFixed64(2);
		Config.AbilitySlots       = 3;
		Config.MaxLevel           = 12;
		Config.XPMultiplier       = FFixed64(2);
		Config.HireCostMin        = 300;
		Config.HireCostMax        = 500;
		Config.FameRequired       = 15;
		Config.bHirableFromTavern = true;
		Config.bQuestRequired     = false;
		Config.MaxPerWizard       = 2;
		Config.BaseLoyalty        = FFixed64(100);
		break;

	case ECoMHeroTier::Demigod:
		Config.StatMultiplier     = FFixed64(3);
		Config.AbilitySlots       = 5;
		Config.MaxLevel           = 15;
		Config.XPMultiplier       = FFixed64(3);
		Config.HireCostMin        = 0;
		Config.HireCostMax        = 0;
		Config.FameRequired       = 40;
		Config.bHirableFromTavern = false;
		Config.bQuestRequired     = true;
		Config.MaxPerWizard       = 1;
		Config.BaseLoyalty        = FFixed64(80);
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

void UCoMHeroSubsystem::SetHeroOwner(int32 HeroUnitID, int32 WizardIndex)
{
	HeroOwners.Add(HeroUnitID, WizardIndex);
}

int32 UCoMHeroSubsystem::GetHeroOwner(int32 HeroUnitID) const
{
	if (const int32* Found = HeroOwners.Find(HeroUnitID)) { return *Found; }
	return -1;
}

bool UCoMHeroSubsystem::CanRecruitTier(int32 WizardIndex, ECoMHeroTier Tier, int32 WizardFame) const
{
	const FCoMHeroTierConfig Config = GetTierConfig(Tier);

	if (WizardFame < Config.FameRequired)
	{
		return false;
	}

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
		if (Tier < Entry.MinTier || Tier > Entry.MaxTier)
		{
			continue;
		}
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
	const int32 TraitCount = static_cast<int32>(ECoMPersonalityTrait::MAX);
	P.PrimaryTrait = static_cast<ECoMPersonalityTrait>(Rng.RandRange(0, TraitCount - 1));
	P.SecondaryTrait = P.PrimaryTrait;
	while (P.SecondaryTrait == P.PrimaryTrait)
	{
		P.SecondaryTrait = static_cast<ECoMPersonalityTrait>(Rng.RandRange(0, TraitCount - 1));
	}

	// Random realm preference.
	const int32 RealmCount = static_cast<int32>(ECoMSpellRealm::None);
	P.PreferredRealm = static_cast<ECoMSpellRealm>(Rng.RandRange(0, RealmCount - 1));

	// Loyalty is int32 (0-100), starts at 100. Ambition is random [30..90].
	P.Loyalty  = 100;
	P.Ambition = Rng.RandRange(30, 90);

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
		return FFixed64(Found->Loyalty);
	}
	return FFixed64(100);
}

void UCoMHeroSubsystem::ModifyLoyalty(int32 HeroUnitID, FFixed64 Delta)
{
	FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID);
	if (!Found)
	{
		return;
	}

	const int32 NewLoyalty = Found->Loyalty + Delta.ToInt32();
	Found->Loyalty = FMath::Clamp(NewLoyalty, 0, 100);
}

bool UCoMHeroSubsystem::CheckDesertion(int32 HeroUnitID)
{
	const FCoMHeroPersonality* Found = Personalities.Find(HeroUnitID);
	if (!Found)
	{
		return false;
	}

	const int32 Loyalty = Found->Loyalty;

	int32 DesertionChance = 0;
	if (Loyalty < 10)
	{
		DesertionChance = 25;
	}
	else if (Loyalty < 30)
	{
		DesertionChance = 5;
	}
	else
	{
		return false;
	}

	const int32 Roll = RngStream.RandRange(0, 99);
	if (Roll < DesertionChance)
	{
		OnHeroDeserted.Broadcast(HeroUnitID, FFixed64(Loyalty));
		return true;
	}

	return false;
}

// =====================================================================
// Hero Relationships
// =====================================================================

void UCoMHeroSubsystem::UpdateRelationship(int32 HeroA, int32 HeroB, ECoMHeroRelationType Type, FFixed64 StrengthDelta)
{
	if (HeroA > HeroB)
	{
		Swap(HeroA, HeroB);
	}

	for (FCoMHeroRelationship& Rel : Relationships)
	{
		if (Rel.HeroA_UnitID == HeroA && Rel.HeroB_UnitID == HeroB && Rel.Type == Type)
		{
			Rel.RelationStrength = Rel.RelationStrength + StrengthDelta.ToInt32();
			Rel.TurnsSinceLastInteraction = 0;
			return;
		}
	}

	FCoMHeroRelationship NewRel;
	NewRel.HeroA_UnitID             = HeroA;
	NewRel.HeroB_UnitID             = HeroB;
	NewRel.Type                     = Type;
	NewRel.RelationStrength         = StrengthDelta.ToInt32();
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
		Pair.Value.Loyalty = Pair.Value.Loyalty - Decay.ToInt32();
		if (Pair.Value.Loyalty < 0)
		{
			Pair.Value.Loyalty = 0;
		}
	}

	// 2. Check desertion for heroes with low loyalty.
	TArray<int32> DesertedHeroes;
	for (const auto& Pair : Personalities)
	{
		if (Pair.Value.Loyalty < 30)
		{
			if (CheckDesertion(Pair.Key))
			{
				DesertedHeroes.Add(Pair.Key);
			}
		}
	}

	for (const int32 ID : DesertedHeroes)
	{
		Personalities.Remove(ID);
	}

	// 3. Age relationships: increment interaction counter, decay weak ones.
	for (int32 i = Relationships.Num() - 1; i >= 0; --i)
	{
		FCoMHeroRelationship& Rel = Relationships[i];
		Rel.TurnsSinceLastInteraction++;

		if (Rel.RelationStrength > 0)
		{
			Rel.RelationStrength -= 1;
		}

		if (Rel.RelationStrength <= 0 && Rel.TurnsSinceLastInteraction > 20)
		{
			Relationships.RemoveAt(i);
		}
	}

	// 4. Roll tavern offers for every wizard. Resolves CurrentTurn off the
	//    turn subsystem so we don't need it passed in.
	int32 CurrentTurn = 1;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMTurnSubsystem* Turn = GI->GetSubsystem<UCoMTurnSubsystem>())
		{
			CurrentTurn = Turn->GetCurrentTurn();
		}
	}
	RollHeroOffers(CurrentTurn);
}

// =====================================================================
// Tavern offers
// =====================================================================

namespace
{
	// MoM-flavoured starter name pool. Repeats are intentional; randomization
	// pulls between these.
	static const TCHAR* GHeroNames[] = {
		TEXT("Roland"),  TEXT("Mortu"),    TEXT("Torin"),     TEXT("Valana"),
		TEXT("Galen"),   TEXT("Sirena"),   TEXT("Ulric"),     TEXT("Yseult"),
		TEXT("Brand"),   TEXT("Cerys"),    TEXT("Doran"),     TEXT("Elara"),
		TEXT("Fenrik"),  TEXT("Gwyn"),     TEXT("Hadrian"),   TEXT("Inara"),
		TEXT("Jorah"),   TEXT("Kaelen"),   TEXT("Lyra"),      TEXT("Maerwyn"),
		TEXT("Niall"),   TEXT("Oryn"),     TEXT("Perrin"),    TEXT("Quenna"),
		TEXT("Riven"),   TEXT("Selene"),   TEXT("Tariq"),     TEXT("Uthred"),
		TEXT("Vesper"),  TEXT("Wren"),     TEXT("Xanthe"),    TEXT("Ysolde"),
	};
}

void UCoMHeroSubsystem::RollHeroOffers(int32 CurrentTurn)
{
	// Expire old offers first.
	for (int32 i = PendingOffers.Num() - 1; i >= 0; --i)
	{
		if (PendingOffers[i].ExpiresOnTurn <= CurrentTurn)
		{
			PendingOffers.RemoveAt(i);
		}
	}

	for (int32 W = 0; W < CoM::MAX_WIZARDS; ++W)
	{
		// Skip wizards who already have a pending offer.
		bool bHasOffer = false;
		for (const FCoMHeroOffer& O : PendingOffers)
		{
			if (O.WizardIndex == W) { bHasOffer = true; break; }
		}
		if (bHasOffer) continue;

		// Roll: 12% base chance per turn, +1% per turn beyond turn 5 capped at 40%.
		const int32 Bonus = FMath::Clamp(CurrentTurn - 5, 0, 28);
		const int32 Chance = 12 + Bonus;
		if (RngStream.RandRange(0, 99) >= Chance) continue;

		// Tier roll — base Adventurer, escalate by turn.
		ECoMHeroTier Tier = ECoMHeroTier::Adventurer;
		if (CurrentTurn >= 30 && RngStream.RandRange(0, 99) < 30) Tier = ECoMHeroTier::Hero;
		if (CurrentTurn >= 70 && RngStream.RandRange(0, 99) < 12) Tier = ECoMHeroTier::Champion;

		// Class roll — pick a random one from the available pool for the tier.
		const TArray<ECoMHeroClass> Classes = GetAvailableClasses(Tier, ECoMWizardClass::MAX);
		if (Classes.Num() == 0) continue;
		const ECoMHeroClass Class = Classes[RngStream.RandRange(0, Classes.Num() - 1)];

		// Cost: pull from tier config and randomize within its band.
		const FCoMHeroTierConfig Cfg = GetTierConfig(Tier);
		const int32 Cost = (Cfg.HireCostMin >= Cfg.HireCostMax)
			? Cfg.HireCostMin
			: RngStream.RandRange(Cfg.HireCostMin, Cfg.HireCostMax);

		FCoMHeroOffer Offer;
		Offer.OfferID       = NextOfferID++;
		Offer.WizardIndex   = W;
		Offer.HeroName      = GHeroNames[RngStream.RandRange(0, UE_ARRAY_COUNT(GHeroNames) - 1)];
		Offer.Tier          = static_cast<uint8>(Tier);
		Offer.HeroClass     = static_cast<uint8>(Class);
		Offer.GoldCost      = Cost;
		Offer.ExpiresOnTurn = CurrentTurn + 3; // 3-turn window

		PendingOffers.Add(Offer);
		OnHeroOffered.Broadcast(Offer);
	}
}

TArray<FCoMHeroOffer> UCoMHeroSubsystem::GetOffersForWizard(int32 WizardIndex) const
{
	TArray<FCoMHeroOffer> Out;
	for (const FCoMHeroOffer& O : PendingOffers)
	{
		if (O.WizardIndex == WizardIndex) { Out.Add(O); }
	}
	return Out;
}

int32 UCoMHeroSubsystem::AcceptHeroOffer(int32 OfferID)
{
	const int32 Idx = PendingOffers.IndexOfByPredicate(
		[OfferID](const FCoMHeroOffer& O) { return O.OfferID == OfferID; });
	if (Idx == INDEX_NONE) return -1;

	const FCoMHeroOffer Offer = PendingOffers[Idx];

	// Cost & spawn need the player state + unit subsystem from the game instance.
	UGameInstance* GI = GetGameInstance();
	if (!GI) return -1;

	UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!Units) return -1;

	// Locate the wizard's capital from the city subsystem; spawn there.
	ECoMPlane    SpawnPlane = ECoMPlane::Aurelith;
	ECoMMapLayer SpawnLayer = ECoMMapLayer::Surface;
	FIntPoint    SpawnPos   = FIntPoint(80, 50);
	if (UCoMCitySubsystem* Cities = GI->GetSubsystem<UCoMCitySubsystem>())
	{
		const TArray<const FCoMCityData*> Owned = Cities->GetCitiesForWizard(Offer.WizardIndex);
		if (Owned.Num() > 0 && Owned[0])
		{
			SpawnPlane = Owned[0]->Plane;
			SpawnLayer = Owned[0]->Layer;
			SpawnPos   = Owned[0]->Position;
		}
		else
		{
			return -1; // No capital to host the hero.
		}
	}

	// Deduct gold. PlayerState is per-wizard via ACoMGameState.
	if (UWorld* World = GI->GetWorld())
	{
		if (ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState()))
		{
			if (ACoMPlayerState* PS = GS->GetWizardByIndex(Offer.WizardIndex))
			{
				if (PS->Gold < Offer.GoldCost) return -1;
				PS->ModifyGold(-Offer.GoldCost);
			}
		}
	}

	// Pick hero spec by class — fallback to Hero_Fighter for unmapped classes.
	const ECoMHeroClass Cls = static_cast<ECoMHeroClass>(Offer.HeroClass);
	FName HeroSpec = FName(TEXT("Hero_Fighter"));
	if (Cls == ECoMHeroClass::Magician || Cls == ECoMHeroClass::Cleric ||
	    Cls == ECoMHeroClass::Necromancer || Cls == ECoMHeroClass::Druid)
	{
		HeroSpec = FName(TEXT("Hero_Magician"));
	}
	const int32 HeroID = Units->SpawnUnitByName(HeroSpec, SpawnPlane, SpawnLayer, SpawnPos, Offer.WizardIndex);
	if (HeroID <= 0) return -1;

	if (FCoMUnitInstance* Unit = const_cast<FCoMUnitInstance*>(Units->GetUnit(HeroID)))
	{
		Unit->bIsHero = true;
	}

	SetHeroTier(HeroID, static_cast<ECoMHeroTier>(Offer.Tier));
	SetHeroClass(HeroID, static_cast<ECoMHeroClass>(Offer.HeroClass));
	SetHeroOwner(HeroID, Offer.WizardIndex);
	InitializeHeroPersonality(HeroID, RngStream);

	PendingOffers.RemoveAt(Idx);
	UE_LOG(LogTemp, Log, TEXT("[Heroes] Wizard %d hired %s (unit %d)"),
		Offer.WizardIndex, *Offer.HeroName, HeroID);
	return HeroID;
}

void UCoMHeroSubsystem::DeclineHeroOffer(int32 OfferID)
{
	PendingOffers.RemoveAll(
		[OfferID](const FCoMHeroOffer& O) { return O.OfferID == OfferID; });
}

FFixed64 UCoMHeroSubsystem::ComputeLoyaltyDecay(const FCoMHeroPersonality& Personality) const
{
	FFixed64 Decay = FFixed64(1);

	if (Personality.PrimaryTrait == ECoMPersonalityTrait::Ambitious ||
		Personality.SecondaryTrait == ECoMPersonalityTrait::Ambitious)
	{
		Decay = Decay * 2;
	}

	if (Personality.PrimaryTrait == ECoMPersonalityTrait::Loyal ||
		Personality.SecondaryTrait == ECoMPersonalityTrait::Loyal)
	{
		Decay = Decay * FFixed64::Half();
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
			const int32* Owner = HeroOwners.Find(Pair.Key);
			if (Owner && *Owner == WizardIndex)
			{
				Count++;
			}
		}
	}
	return Count;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load Export/Import
// ─────────────────────────────────────────────────────────────────────────────

void UCoMHeroSubsystem::ExportAll(TArray<FCoMHeroPersonality>& OutPersonalities,
                                   TArray<int32>& OutPersonalityUnitIDs,
                                   TArray<FCoMHeroRelationship>& OutRelationships,
                                   TArray<int32>& OutTierUnitIDs, TArray<uint8>& OutTierValues,
                                   TArray<int32>& OutClassUnitIDs, TArray<uint8>& OutClassValues,
                                   TArray<int32>& OutOwnerUnitIDs, TArray<int32>& OutOwnerWizardIDs) const
{
	OutPersonalities.Empty();
	OutPersonalityUnitIDs.Empty();
	for (const auto& Pair : Personalities)
	{
		OutPersonalityUnitIDs.Add(Pair.Key);
		OutPersonalities.Add(Pair.Value);
	}

	OutRelationships = Relationships;

	OutTierUnitIDs.Empty();
	OutTierValues.Empty();
	for (const auto& Pair : HeroTiers)
	{
		OutTierUnitIDs.Add(Pair.Key);
		OutTierValues.Add(static_cast<uint8>(Pair.Value));
	}

	OutClassUnitIDs.Empty();
	OutClassValues.Empty();
	for (const auto& Pair : HeroClasses)
	{
		OutClassUnitIDs.Add(Pair.Key);
		OutClassValues.Add(static_cast<uint8>(Pair.Value));
	}

	OutOwnerUnitIDs.Empty();
	OutOwnerWizardIDs.Empty();
	for (const auto& Pair : HeroOwners)
	{
		OutOwnerUnitIDs.Add(Pair.Key);
		OutOwnerWizardIDs.Add(Pair.Value);
	}
}

void UCoMHeroSubsystem::ImportAll(const TArray<FCoMHeroPersonality>& InPersonalities,
                                   const TArray<int32>& InPersonalityUnitIDs,
                                   const TArray<FCoMHeroRelationship>& InRelationships,
                                   const TArray<int32>& InTierUnitIDs, const TArray<uint8>& InTierValues,
                                   const TArray<int32>& InClassUnitIDs, const TArray<uint8>& InClassValues,
                                   const TArray<int32>& InOwnerUnitIDs, const TArray<int32>& InOwnerWizardIDs)
{
	Personalities.Empty();
	for (int32 i = 0; i < InPersonalities.Num() && i < InPersonalityUnitIDs.Num(); ++i)
	{
		Personalities.Add(InPersonalityUnitIDs[i], InPersonalities[i]);
	}

	Relationships = InRelationships;

	HeroTiers.Empty();
	for (int32 i = 0; i < InTierUnitIDs.Num() && i < InTierValues.Num(); ++i)
	{
		HeroTiers.Add(InTierUnitIDs[i], static_cast<ECoMHeroTier>(InTierValues[i]));
	}

	HeroClasses.Empty();
	for (int32 i = 0; i < InClassUnitIDs.Num() && i < InClassValues.Num(); ++i)
	{
		HeroClasses.Add(InClassUnitIDs[i], static_cast<ECoMHeroClass>(InClassValues[i]));
	}

	HeroOwners.Empty();
	for (int32 i = 0; i < InOwnerUnitIDs.Num() && i < InOwnerWizardIDs.Num(); ++i)
	{
		HeroOwners.Add(InOwnerUnitIDs[i], InOwnerWizardIDs[i]);
	}

	UE_LOG(LogTemp, Log, TEXT("[HeroSubsystem] ImportAll: %d personalities, %d relationships imported."),
	       Personalities.Num(), Relationships.Num());
}
