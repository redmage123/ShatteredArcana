// Copyright Shattered Arcana. All Rights Reserved.
// CoMSpellDatabase.cpp — Tier-based spell cost system for 600 spells
// 10 realms x 4 tiers x 15 spells = 600 total
// Values inspired by Master of Magic / Caster of Magic

#include "CoMSpellDatabase.h"

TMap<FName, FCoMSpellInfo> CoMSpellDatabase::SpellTable;
bool CoMSpellDatabase::bInitialized = false;

void CoMSpellDatabase::RegisterSpell(FCoMSpellInfo&& Info)
{
	SpellTable.Add(Info.SpellID, MoveTemp(Info));
}

void CoMSpellDatabase::EnsureInitialized()
{
	if (!bInitialized)
	{
		InitializeDatabase();
		bInitialized = true;
	}
}

// =====================================================================
// Tier Cost Tables
// =====================================================================

int32 CoMSpellDatabase::GetTierResearchCost(ECoMSpellRarity Rarity)
{
	switch (Rarity)
	{
	case ECoMSpellRarity::Common:    return 50;
	case ECoMSpellRarity::Uncommon:  return 150;
	case ECoMSpellRarity::Rare:      return 400;
	case ECoMSpellRarity::VeryRare:  return 1000;
	default:                         return 100;
	}
}

int32 CoMSpellDatabase::GetTierCastingCost(ECoMSpellRarity Rarity)
{
	switch (Rarity)
	{
	case ECoMSpellRarity::Common:    return 5;
	case ECoMSpellRarity::Uncommon:  return 15;
	case ECoMSpellRarity::Rare:      return 40;
	case ECoMSpellRarity::VeryRare:  return 100;
	default:                         return 10;
	}
}

int32 CoMSpellDatabase::GetTierRange(ECoMSpellRarity Rarity)
{
	switch (Rarity)
	{
	case ECoMSpellRarity::Common:    return 4;
	case ECoMSpellRarity::Uncommon:  return 6;
	case ECoMSpellRarity::Rare:      return 8;
	case ECoMSpellRarity::VeryRare:  return 10;
	default:                         return 4;
	}
}

// =====================================================================
// Realm/Rarity Parsers (from spell ID naming convention)
// =====================================================================

ECoMSpellRealm CoMSpellDatabase::ParseRealm(const FString& SpellIDStr)
{
	if (SpellIDStr.StartsWith(TEXT("Life")))    return ECoMSpellRealm::Life;
	if (SpellIDStr.StartsWith(TEXT("Death")))   return ECoMSpellRealm::Death;
	if (SpellIDStr.StartsWith(TEXT("Chaos")))   return ECoMSpellRealm::Chaos;
	if (SpellIDStr.StartsWith(TEXT("Nature")))  return ECoMSpellRealm::Nature;
	if (SpellIDStr.StartsWith(TEXT("Sorcery"))) return ECoMSpellRealm::Sorcery;
	if (SpellIDStr.StartsWith(TEXT("Arcane")))  return ECoMSpellRealm::Arcane;
	if (SpellIDStr.StartsWith(TEXT("Binding"))) return ECoMSpellRealm::Binding;
	if (SpellIDStr.StartsWith(TEXT("Spirit")))  return ECoMSpellRealm::Spirit;
	if (SpellIDStr.StartsWith(TEXT("Glamour"))) return ECoMSpellRealm::Glamour;
	return ECoMSpellRealm::Arcane;
}

ECoMSpellRarity CoMSpellDatabase::ParseRarity(const FString& SpellIDStr)
{
	if (SpellIDStr.Contains(TEXT("_T1_"))) return ECoMSpellRarity::Common;
	if (SpellIDStr.Contains(TEXT("_T2_"))) return ECoMSpellRarity::Uncommon;
	if (SpellIDStr.Contains(TEXT("_T3_"))) return ECoMSpellRarity::Rare;
	if (SpellIDStr.Contains(TEXT("_T4_"))) return ECoMSpellRarity::VeryRare;
	// Fallback: estimate from name length / position in realm
	return ECoMSpellRarity::Common;
}

ECoMSpellEffect CoMSpellDatabase::ParseEffect(const FString& SpellIDStr)
{
	FString Lower = SpellIDStr.ToLower();
	if (Lower.Contains(TEXT("bolt")) || Lower.Contains(TEXT("blast")) ||
	    Lower.Contains(TEXT("fire")) || Lower.Contains(TEXT("lightning")) ||
	    Lower.Contains(TEXT("doom")) || Lower.Contains(TEXT("death")) ||
	    Lower.Contains(TEXT("ice")) || Lower.Contains(TEXT("warp")))
	{
		return ECoMSpellEffect::Damage;
	}
	if (Lower.Contains(TEXT("heal")) || Lower.Contains(TEXT("cure")) ||
	    Lower.Contains(TEXT("restore")) || Lower.Contains(TEXT("rejuv")))
	{
		return ECoMSpellEffect::Healing;
	}
	if (Lower.Contains(TEXT("enchant")) || Lower.Contains(TEXT("bless")) ||
	    Lower.Contains(TEXT("shield")) || Lower.Contains(TEXT("resist")) ||
	    Lower.Contains(TEXT("haste")) || Lower.Contains(TEXT("might")))
	{
		return ECoMSpellEffect::UnitBuff;
	}
	if (Lower.Contains(TEXT("curse")) || Lower.Contains(TEXT("weaken")) ||
	    Lower.Contains(TEXT("poison")) || Lower.Contains(TEXT("slow")))
	{
		return ECoMSpellEffect::UnitDebuff;
	}
	if (Lower.Contains(TEXT("summon")) || Lower.Contains(TEXT("conjure")) ||
	    Lower.Contains(TEXT("call")))
	{
		return ECoMSpellEffect::Summon;
	}
	if (Lower.Contains(TEXT("wall")) || Lower.Contains(TEXT("terrain")) ||
	    Lower.Contains(TEXT("growth")) || Lower.Contains(TEXT("corrupt")))
	{
		return ECoMSpellEffect::Terrain;
	}
	if (Lower.Contains(TEXT("global")) || Lower.Contains(TEXT("aura")) ||
	    Lower.Contains(TEXT("eternal")))
	{
		return ECoMSpellEffect::GlobalEnchantment;
	}
	if (Lower.Contains(TEXT("dispel")) || Lower.Contains(TEXT("counter")) ||
	    Lower.Contains(TEXT("disjunct")))
	{
		return ECoMSpellEffect::Dispel;
	}
	if (Lower.Contains(TEXT("city")) || Lower.Contains(TEXT("prosperity")) ||
	    Lower.Contains(TEXT("ward")))
	{
		return ECoMSpellEffect::CityBuff;
	}
	if (Lower.Contains(TEXT("reveal")) || Lower.Contains(TEXT("vision")) ||
	    Lower.Contains(TEXT("detect")))
	{
		return ECoMSpellEffect::Divination;
	}
	if (Lower.Contains(TEXT("diplomat")) || Lower.Contains(TEXT("treaty")) ||
	    Lower.Contains(TEXT("alliance")) || Lower.Contains(TEXT("pact")) ||
	    Lower.Contains(TEXT("reputation")) || Lower.Contains(TEXT("majesty")))
	{
		return ECoMSpellEffect::Diplomatic;
	}
	return ECoMSpellEffect::Damage; // Default
}

// =====================================================================
// Lookup — falls back to tier-based defaults for unregistered spells
// =====================================================================

FCoMSpellInfo CoMSpellDatabase::GetSpellInfo(FName SpellID)
{
	EnsureInitialized();

	if (const FCoMSpellInfo* Found = SpellTable.Find(SpellID))
	{
		return *Found;
	}

	// Fall back to tier-based defaults parsed from the spell ID
	FString IDStr = SpellID.ToString();
	ECoMSpellRealm Realm = ParseRealm(IDStr);
	ECoMSpellRarity Rarity = ParseRarity(IDStr);
	ECoMSpellEffect Effect = ParseEffect(IDStr);

	FCoMSpellInfo Info;
	Info.SpellID = SpellID;
	Info.DisplayName = FText::FromString(IDStr.Replace(TEXT("_"), TEXT(" ")));
	Info.Realm = Realm;
	Info.Rarity = Rarity;
	Info.ResearchCost = GetTierResearchCost(Rarity);
	Info.CastingCost = GetTierCastingCost(Rarity);
	Info.Range = GetTierRange(Rarity);
	Info.EffectType = Effect;

	// Set target type based on effect
	switch (Effect)
	{
	case ECoMSpellEffect::Damage:
	case ECoMSpellEffect::UnitBuff:
	case ECoMSpellEffect::UnitDebuff:
	case ECoMSpellEffect::Healing:
		Info.TargetType = ECoMSpellTarget::UnitTarget;
		break;
	case ECoMSpellEffect::CityBuff:
	case ECoMSpellEffect::CityDebuff:
		Info.TargetType = ECoMSpellTarget::CityTarget;
		break;
	case ECoMSpellEffect::Terrain:
	case ECoMSpellEffect::Summon:
		Info.TargetType = ECoMSpellTarget::TileTarget;
		break;
	case ECoMSpellEffect::GlobalEnchantment:
	case ECoMSpellEffect::Divination:
	case ECoMSpellEffect::Dispel:
	case ECoMSpellEffect::Diplomatic:
		Info.TargetType = ECoMSpellTarget::NoTarget;
		break;
	}

	// Set scope based on effect
	if (Effect == ECoMSpellEffect::GlobalEnchantment)
	{
		Info.Scope = ECoMSpellScope::Global;
		Info.bOngoing = true;
		Info.bInstant = false;
		Info.UpkeepMana = GetTierCastingCost(Rarity) / 5;
	}
	else if (Effect == ECoMSpellEffect::CityBuff || Effect == ECoMSpellEffect::CityDebuff)
	{
		Info.Scope = ECoMSpellScope::Overworld;
		Info.bOngoing = true;
		Info.bInstant = false;
		Info.UpkeepMana = GetTierCastingCost(Rarity) / 5;
	}

	if (Effect == ECoMSpellEffect::Summon)
	{
		Info.bSummon = true;
	}

	// Damage base scales with tier
	if (Effect == ECoMSpellEffect::Damage)
	{
		Info.DamageBase = GetTierCastingCost(Rarity); // 5/15/40/100
	}
	if (Effect == ECoMSpellEffect::Healing)
	{
		Info.HealAmount = GetTierCastingCost(Rarity); // 5/15/40/100
	}

	return Info;
}

TArray<FName> CoMSpellDatabase::GetAllSpellIDs()
{
	EnsureInitialized();
	TArray<FName> Result;
	SpellTable.GetKeys(Result);
	return Result;
}

TArray<FName> CoMSpellDatabase::GetSpellsForRealm(ECoMSpellRealm Realm)
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : SpellTable)
	{
		if (Pair.Value.Realm == Realm)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> CoMSpellDatabase::GetSpellsByRarity(ECoMSpellRarity Rarity)
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : SpellTable)
	{
		if (Pair.Value.Rarity == Rarity)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool CoMSpellDatabase::Contains(FName SpellID)
{
	EnsureInitialized();
	return SpellTable.Contains(SpellID);
}

TArray<FName> CoMSpellDatabase::GetSpellsForRealmAndRarity(ECoMSpellRealm Realm, ECoMSpellRarity Rarity)
{
	EnsureInitialized();
	TArray<FName> Result;
	for (const auto& Pair : SpellTable)
	{
		if (Pair.Value.Realm == Realm && Pair.Value.Rarity == Rarity)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

void CoMSpellDatabase::GetSpellsForBookCount(ECoMSpellRealm Realm, int32 BookCount,
	TArray<FName>& OutLearnableSpells, TArray<FName>& OutStartingSpells)
{
	OutLearnableSpells.Empty();
	OutStartingSpells.Empty();

	if (BookCount <= 0 || Realm == ECoMSpellRealm::None) return;

	// Determine which tiers are accessible based on book count (MoM rules)
	// 1 book: Common only
	// 2-3 books: Common + Uncommon
	// 4-5 books: Common + Uncommon + Rare
	// 6+ books: Common + Uncommon + Rare + Very Rare
	TArray<ECoMSpellRarity> AccessibleTiers;
	AccessibleTiers.Add(ECoMSpellRarity::Common);
	if (BookCount >= 2) AccessibleTiers.Add(ECoMSpellRarity::Uncommon);
	if (BookCount >= 4) AccessibleTiers.Add(ECoMSpellRarity::Rare);
	if (BookCount >= 6) AccessibleTiers.Add(ECoMSpellRarity::VeryRare);

	// Gather all learnable spells from accessible tiers
	for (ECoMSpellRarity Tier : AccessibleTiers)
	{
		TArray<FName> TierSpells = GetSpellsForRealmAndRarity(Realm, Tier);
		OutLearnableSpells.Append(TierSpells);
	}

	// Starting spells: 1 guaranteed Common spell per 2 books (MoM rule)
	// Plus 1 guaranteed Uncommon at 4+ books, 1 Rare at 8+ books
	TArray<FName> CommonSpells = GetSpellsForRealmAndRarity(Realm, ECoMSpellRarity::Common);
	int32 GuaranteedCommon = FMath::Min(BookCount / 2, CommonSpells.Num());
	for (int32 i = 0; i < GuaranteedCommon; ++i)
	{
		OutStartingSpells.Add(CommonSpells[i]);
	}

	if (BookCount >= 4)
	{
		TArray<FName> UncommonSpells = GetSpellsForRealmAndRarity(Realm, ECoMSpellRarity::Uncommon);
		if (UncommonSpells.Num() > 0)
		{
			OutStartingSpells.Add(UncommonSpells[0]);
		}
	}

	if (BookCount >= 8)
	{
		TArray<FName> RareSpells = GetSpellsForRealmAndRarity(Realm, ECoMSpellRarity::Rare);
		if (RareSpells.Num() > 0)
		{
			OutStartingSpells.Add(RareSpells[0]);
		}
	}
}

// =====================================================================
// Explicit Spell Registrations — representative spells per realm/tier
// The remaining ~550 spells use the tier-based fallback system.
// =====================================================================

void CoMSpellDatabase::InitializeDatabase()
{
	// ─── LIFE REALM ──────────────────────────────────────────────────

	// T1 Common
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T1_Healing_Word"));
		S.DisplayName = FText::FromString(TEXT("Healing Word"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Healing;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.HealAmount = 5; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T1_Bless"));
		S.DisplayName = FText::FromString(TEXT("Bless"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bInstant = false; S.bOngoing = true; S.UpkeepMana = 1;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T1_Star_Fires"));
		S.DisplayName = FText::FromString(TEXT("Star Fires"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Damage;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.DamageBase = 5; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T1_Endurance"));
		S.DisplayName = FText::FromString(TEXT("Endurance"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bOngoing = true; S.UpkeepMana = 1;
		RegisterSpell(MoveTemp(S));
	}

	// T2 Uncommon
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T2_Heroism"));
		S.DisplayName = FText::FromString(TEXT("Heroism"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bOngoing = true; S.UpkeepMana = 3;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T2_Prosperity"));
		S.DisplayName = FText::FromString(TEXT("Prosperity"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bOngoing = true; S.UpkeepMana = 3;
		RegisterSpell(MoveTemp(S));
	}

	// T3 Rare
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T3_High_Prayer"));
		S.DisplayName = FText::FromString(TEXT("High Prayer"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bInstant = false; S.bOngoing = true;
		RegisterSpell(MoveTemp(S));
	}

	// T4 Very Rare
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Life_T4_Consecration"));
		S.DisplayName = FText::FromString(TEXT("Consecration"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 100; S.Range = 10;
		S.bOngoing = true; S.UpkeepMana = 20;
		RegisterSpell(MoveTemp(S));
	}

	// ─── DEATH REALM ─────────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Death_T1_Life_Drain"));
		S.DisplayName = FText::FromString(TEXT("Life Drain"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Damage;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.DamageBase = 4; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Death_T1_Skeletons"));
		S.DisplayName = FText::FromString(TEXT("Summon Skeletons"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Summon;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bSummon = true; S.bInstant = true;
		S.SummonSpecID = FName(TEXT("Summon_Skeleton"));
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Death_T2_Black_Sleep"));
		S.DisplayName = FText::FromString(TEXT("Black Sleep"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitDebuff;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Death_T3_Wraith_Form"));
		S.DisplayName = FText::FromString(TEXT("Wraith Form"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bOngoing = true; S.UpkeepMana = 8;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Death_T4_Death_Wish"));
		S.DisplayName = FText::FromString(TEXT("Death Wish"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 100; S.Range = 10;
		S.bOngoing = true; S.UpkeepMana = 20;
		RegisterSpell(MoveTemp(S));
	}

	// ─── CHAOS REALM ─────────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_T1_Fire_Bolt"));
		S.DisplayName = FText::FromString(TEXT("Fire Bolt"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Damage;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.DamageBase = 6; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_T2_Fireball"));
		S.DisplayName = FText::FromString(TEXT("Fireball"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Damage;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.DamageBase = 15; S.bAOE = true; S.AOERadius = 2; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_T3_Lightning_Bolt"));
		S.DisplayName = FText::FromString(TEXT("Lightning Bolt"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Damage;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.DamageBase = 40; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_T4_Call_The_Void"));
		S.DisplayName = FText::FromString(TEXT("Call the Void"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 100; S.Range = 10;
		S.DamageBase = 50; S.bAOE = true; S.bOngoing = true; S.UpkeepMana = 25;
		RegisterSpell(MoveTemp(S));
	}

	// ─── NATURE REALM ────────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T1_Resist_Elements"));
		S.DisplayName = FText::FromString(TEXT("Resist Elements"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bOngoing = true; S.UpkeepMana = 1;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T1_Summon_War_Bears"));
		S.DisplayName = FText::FromString(TEXT("Summon War Bears"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Summon;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bSummon = true;
		S.SummonSpecID = FName(TEXT("Summon_WarBear"));
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T2_Wild_Growth"));
		S.DisplayName = FText::FromString(TEXT("Wild Growth"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Terrain;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		// Enchant Road: MoM-style global road enchantment. Every built road in
		// the world (RoadLevel 1) is upgraded to an enchanted road (RoadLevel 2)
		// granting 0.25x movement cost instead of 0.5x.
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T2_Enchant_Road"));
		S.DisplayName = FText::FromString(TEXT("Enchant Road"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Terrain;
		S.ResearchCost = 200; S.CastingCost = 50;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T3_Summon_Earth_Elemental"));
		S.DisplayName = FText::FromString(TEXT("Summon Earth Elemental"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Summon;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bSummon = true;
		S.SummonSpecID = FName(TEXT("Summon_EarthElemental"));
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T4_Natures_Wrath"));
		S.DisplayName = FText::FromString(TEXT("Nature's Wrath"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 100; S.Range = 10;
		S.bOngoing = true; S.UpkeepMana = 20;
		RegisterSpell(MoveTemp(S));
	}

	// ─── SORCERY REALM ───────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Sorcery_T1_Confusion"));
		S.DisplayName = FText::FromString(TEXT("Confusion"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitDebuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Sorcery_T2_Invisibility"));
		S.DisplayName = FText::FromString(TEXT("Invisibility"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bOngoing = true; S.UpkeepMana = 3;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Sorcery_T3_Dispel_Magic_True"));
		S.DisplayName = FText::FromString(TEXT("Dispel Magic True"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Dispel;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Sorcery_T4_Spell_Of_Mastery"));
		S.DisplayName = FText::FromString(TEXT("Spell of Mastery"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 100; S.Range = 10;
		S.bOngoing = true; S.UpkeepMana = 25;
		RegisterSpell(MoveTemp(S));
	}

	// ─── ARCANE REALM ────────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Arcane_T1_Magic_Spirit"));
		S.DisplayName = FText::FromString(TEXT("Magic Spirit"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Summon;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bSummon = true;
		S.SummonSpecID = FName(TEXT("Summon_MagicSpirit"));
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Arcane_T1_Detect_Magic"));
		S.DisplayName = FText::FromString(TEXT("Detect Magic"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Divination;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Arcane_T2_Dispel_Magic"));
		S.DisplayName = FText::FromString(TEXT("Dispel Magic"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::Dispel;
		S.ResearchCost = 150; S.CastingCost = 15; S.Range = 6;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Arcane_T3_Disjunction"));
		S.DisplayName = FText::FromString(TEXT("Disjunction"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Dispel;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── BINDING REALM ───────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Binding_T1_Soul_Chain"));
		S.DisplayName = FText::FromString(TEXT("Soul Chain"));
		S.Realm = ECoMSpellRealm::Binding; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitDebuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── SPIRIT REALM ────────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Spirit_T1_Dream_Vision"));
		S.DisplayName = FText::FromString(TEXT("Dream Vision"));
		S.Realm = ECoMSpellRealm::Spirit; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Divination;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── GLAMOUR REALM ───────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Glamour_T1_Fey_Glamour"));
		S.DisplayName = FText::FromString(TEXT("Fey Glamour"));
		S.Realm = ECoMSpellRealm::Glamour; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::UnitTarget; S.EffectType = ECoMSpellEffect::UnitBuff;
		S.ResearchCost = 50; S.CastingCost = 5; S.Range = 4;
		S.bOngoing = true; S.UpkeepMana = 1;
		RegisterSpell(MoveTemp(S));
	}

	// =====================================================================
	// Diplomatic Spells
	// =====================================================================

	// ─── Life Diplomatic ─────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Aura_of_Majesty"));
		S.DisplayName = FText::FromString(TEXT("Aura of Majesty"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 400; S.CastingCost = 120;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Divine_Mandate"));
		S.DisplayName = FText::FromString(TEXT("Divine Mandate"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Holy_Alliance"));
		S.DisplayName = FText::FromString(TEXT("Holy Alliance"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 400; S.CastingCost = 80;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Death Diplomatic ────────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Aura_of_Fear"));
		S.DisplayName = FText::FromString(TEXT("Aura of Fear"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 400; S.CastingCost = 100;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Dark_Intimidation"));
		S.DisplayName = FText::FromString(TEXT("Dark Intimidation"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Soul_Pact"));
		S.DisplayName = FText::FromString(TEXT("Soul Pact"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 1000; S.CastingCost = 100;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Glamour Diplomatic ─────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Charm_of_Persuasion"));
		S.DisplayName = FText::FromString(TEXT("Charm of Persuasion"));
		S.Realm = ECoMSpellRealm::Glamour; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 50; S.CastingCost = 40;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Diplomatic_Illusion"));
		S.DisplayName = FText::FromString(TEXT("Diplomatic Illusion"));
		S.Realm = ECoMSpellRealm::Glamour; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 150; S.CastingCost = 50;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("True_Name_Binding"));
		S.DisplayName = FText::FromString(TEXT("True Name Binding"));
		S.Realm = ECoMSpellRealm::Glamour; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 1000; S.CastingCost = 150;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Binding Diplomatic ─────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Dark_Contract"));
		S.DisplayName = FText::FromString(TEXT("Dark Contract"));
		S.Realm = ECoMSpellRealm::Binding; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 400; S.CastingCost = 80;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Infernal_Ultimatum"));
		S.DisplayName = FText::FromString(TEXT("Infernal Ultimatum"));
		S.Realm = ECoMSpellRealm::Binding; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Diplomatic;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Sorcery Diplomatic ─────────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Word_of_Recall"));
		S.DisplayName = FText::FromString(TEXT("Word of Recall"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Dispel;
		S.ResearchCost = 150; S.CastingCost = 50;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Diplomatic_Scrying"));
		S.DisplayName = FText::FromString(TEXT("Diplomatic Scrying"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Divination;
		S.ResearchCost = 50; S.CastingCost = 30;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// =====================================================================
	// City Enchantments (MoM-style)
	// =====================================================================

	// ─── Life City Enchantments ──────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Heavenly_Light"));
		S.DisplayName = FText::FromString(TEXT("Heavenly Light"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 50; S.CastingCost = 30;
		S.UpkeepMana = 3; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Consecrate_Ground"));
		S.DisplayName = FText::FromString(TEXT("Consecrate Ground"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Prosperity"));
		S.DisplayName = FText::FromString(TEXT("Prosperity"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 400; S.CastingCost = 100;
		S.UpkeepMana = 4; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Guardian_Spirit"));
		S.DisplayName = FText::FromString(TEXT("Guardian Spirit"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 1000; S.CastingCost = 200;
		S.UpkeepMana = 6; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Death City Enchantments ─────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Dark_Ritual"));
		S.DisplayName = FText::FromString(TEXT("Dark Ritual"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 50; S.CastingCost = 25;
		S.UpkeepMana = 3; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Pestilence"));
		S.DisplayName = FText::FromString(TEXT("Pestilence"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityDebuff;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Famine"));
		S.DisplayName = FText::FromString(TEXT("Famine"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityDebuff;
		S.ResearchCost = 400; S.CastingCost = 100;
		S.UpkeepMana = 4; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Cursed_Lands"));
		S.DisplayName = FText::FromString(TEXT("Cursed Lands"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityDebuff;
		S.ResearchCost = 1000; S.CastingCost = 200;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Chaos City Enchantments ─────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Wall_of_Fire"));
		S.DisplayName = FText::FromString(TEXT("Wall of Fire"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 50; S.CastingCost = 30;
		S.UpkeepMana = 3; S.bOngoing = true; S.bInstant = false;
		S.DamageBase = 3;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_Rift"));
		S.DisplayName = FText::FromString(TEXT("Chaos Rift"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Volcano"));
		S.DisplayName = FText::FromString(TEXT("Volcano"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityDebuff;
		S.ResearchCost = 400; S.CastingCost = 200;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Armageddon_Clock"));
		S.DisplayName = FText::FromString(TEXT("Armageddon Clock"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 20; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Nature City Enchantments ────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_Ward"));
		S.DisplayName = FText::FromString(TEXT("Nature Ward"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 50; S.CastingCost = 20;
		S.UpkeepMana = 2; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Gaias_Blessing"));
		S.DisplayName = FText::FromString(TEXT("Gaia's Blessing"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Earth_Gate"));
		S.DisplayName = FText::FromString(TEXT("Earth Gate"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 400; S.CastingCost = 120;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Great_Tree"));
		S.DisplayName = FText::FromString(TEXT("Great Tree"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 1000; S.CastingCost = 250;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Sorcery City Enchantments ───────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Awareness"));
		S.DisplayName = FText::FromString(TEXT("Awareness"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 50; S.CastingCost = 25;
		S.UpkeepMana = 3; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Phantom_Warriors"));
		S.DisplayName = FText::FromString(TEXT("Phantom Warriors"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 150; S.CastingCost = 50;
		S.UpkeepMana = 4; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Spell_Ward"));
		S.DisplayName = FText::FromString(TEXT("Spell Ward"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 400; S.CastingCost = 100;
		S.UpkeepMana = 6; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Flying_Fortress"));
		S.DisplayName = FText::FromString(TEXT("Flying Fortress"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Overworld;
		S.TargetType = ECoMSpellTarget::CityTarget; S.EffectType = ECoMSpellEffect::CityBuff;
		S.ResearchCost = 1000; S.CastingCost = 250;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// =====================================================================
	// Global Enchantments
	// =====================================================================

	// ─── Life Global Enchantments ────────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Crusade"));
		S.DisplayName = FText::FromString(TEXT("Crusade"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 15; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Tranquility"));
		S.DisplayName = FText::FromString(TEXT("Tranquility"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 120;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Just_Cause"));
		S.DisplayName = FText::FromString(TEXT("Just Cause"));
		S.Realm = ECoMSpellRealm::Life; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 120;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Death Global Enchantments ───────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Eternal_Night"));
		S.DisplayName = FText::FromString(TEXT("Eternal Night"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 15; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Zombie_Mastery"));
		S.DisplayName = FText::FromString(TEXT("Zombie Mastery"));
		S.Realm = ECoMSpellRealm::Death; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 150;
		S.UpkeepMana = 12; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Chaos Global Enchantments ───────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Chaos_Surge"));
		S.DisplayName = FText::FromString(TEXT("Chaos Surge"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 150;
		S.UpkeepMana = 12; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Great_Wasting"));
		S.DisplayName = FText::FromString(TEXT("Great Wasting"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 15; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Doom_Mastery"));
		S.DisplayName = FText::FromString(TEXT("Doom Mastery"));
		S.Realm = ECoMSpellRealm::Chaos; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 350;
		S.UpkeepMana = 20; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Nature Global Enchantments ──────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_Awareness"));
		S.DisplayName = FText::FromString(TEXT("Nature Awareness"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 120;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Herb_Mastery"));
		S.DisplayName = FText::FromString(TEXT("Herb Mastery"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Uncommon;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 150; S.CastingCost = 60;
		S.UpkeepMana = 8; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Gaia_Force"));
		S.DisplayName = FText::FromString(TEXT("Gaia Force"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 15; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Sorcery Global Enchantments ─────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Suppress_Magic"));
		S.DisplayName = FText::FromString(TEXT("Suppress Magic"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 300;
		S.UpkeepMana = 15; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Detect_Magic"));
		S.DisplayName = FText::FromString(TEXT("Detect Magic"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Common;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::Divination;
		S.ResearchCost = 50; S.CastingCost = 20;
		S.UpkeepMana = 5; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Aether_Binding"));
		S.DisplayName = FText::FromString(TEXT("Aether Binding"));
		S.Realm = ECoMSpellRealm::Sorcery; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 400; S.CastingCost = 150;
		S.UpkeepMana = 10; S.bOngoing = true; S.bInstant = false;
		RegisterSpell(MoveTemp(S));
	}

	// ─── Arcane Global Enchantments ──────────────────────────────────

	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Spell_of_Mastery"));
		S.DisplayName = FText::FromString(TEXT("Spell of Mastery"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 10000; S.CastingCost = 5000;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}
	{
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Spell_of_Return"));
		S.DisplayName = FText::FromString(TEXT("Spell of Return"));
		S.Realm = ECoMSpellRealm::Arcane; S.Rarity = ECoMSpellRarity::VeryRare;
		S.Scope = ECoMSpellScope::Global;
		S.TargetType = ECoMSpellTarget::NoTarget; S.EffectType = ECoMSpellEffect::GlobalEnchantment;
		S.ResearchCost = 1000; S.CastingCost = 500;
		S.UpkeepMana = 0; S.bOngoing = false; S.bInstant = true;
		RegisterSpell(MoveTemp(S));
	}

	// ──────────────────────────────────────────────────────────────────────
	// Summon roster -- one spell per CoM-style summonable creature, each
	// explicitly bound to its unit SpecID so the resolver doesn't need a
	// hardcoded realm+tier table.
	// ──────────────────────────────────────────────────────────────────────
	auto AddSummon = [](const TCHAR* SpellID, const TCHAR* Display, ECoMSpellRealm Realm,
	                    ECoMSpellRarity Rarity, const TCHAR* SpecID,
	                    int32 ResearchCost, int32 CastingCost)
	{
		FCoMSpellInfo S;
		S.SpellID       = FName(SpellID);
		S.DisplayName   = FText::FromString(Display);
		S.Realm         = Realm;
		S.Rarity        = Rarity;
		S.Scope         = ECoMSpellScope::Overworld;
		S.TargetType    = ECoMSpellTarget::TileTarget;
		S.EffectType    = ECoMSpellEffect::Summon;
		S.ResearchCost  = ResearchCost;
		S.CastingCost   = CastingCost;
		S.Range         = 8;
		S.bSummon       = true;
		S.bInstant      = true;
		S.SummonSpecID  = FName(SpecID);
		RegisterSpell(MoveTemp(S));
	};

	// Life ----------------------------------------------------------------
	AddSummon(TEXT("Life_T2_Summon_Unicorns"),       TEXT("Summon Unicorns"),
	          ECoMSpellRealm::Life,    ECoMSpellRarity::Uncommon, TEXT("Summon_Unicorns"),     150, 15);
	AddSummon(TEXT("Life_T4_Summon_Arch_Angel"),     TEXT("Summon Arch Angel"),
	          ECoMSpellRealm::Life,    ECoMSpellRarity::VeryRare, TEXT("Summon_ArchAngel"),    900, 90);

	// Death ---------------------------------------------------------------
	AddSummon(TEXT("Death_T1_Summon_Ghouls"),        TEXT("Summon Ghouls"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::Common,   TEXT("Summon_Ghouls"),        60,  6);
	AddSummon(TEXT("Death_T1_Summon_Werewolves"),    TEXT("Summon Werewolves"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::Common,   TEXT("Summon_Werewolves"),   120, 12);
	AddSummon(TEXT("Death_T2_Summon_Night_Stalker"), TEXT("Summon Night Stalker"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::Uncommon, TEXT("Summon_NightStalker"), 180, 18);
	AddSummon(TEXT("Death_T2_Summon_Shadow_Demons"), TEXT("Summon Shadow Demons"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::Uncommon, TEXT("Summon_ShadowDemons"), 220, 22);
	AddSummon(TEXT("Death_T4_Summon_Demon_Lord"),    TEXT("Summon Demon Lord"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::VeryRare, TEXT("Summon_DemonLord"),    900, 90);
	AddSummon(TEXT("Death_T4_Summon_Death_Knights"), TEXT("Summon Death Knights"),
	          ECoMSpellRealm::Death,   ECoMSpellRarity::VeryRare, TEXT("Summon_DeathKnights"), 700, 70);

	// Chaos ---------------------------------------------------------------
	AddSummon(TEXT("Chaos_T1_Summon_Fire_Elemental"),TEXT("Summon Fire Elemental"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Common,   TEXT("Summon_FireElemental"), 80,  8);
	AddSummon(TEXT("Chaos_T2_Summon_Fire_Giant"),    TEXT("Summon Fire Giant"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Uncommon, TEXT("Summon_FireGiant"),    220, 22);
	AddSummon(TEXT("Chaos_T2_Summon_Doom_Bat"),      TEXT("Summon Doom Bat"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Uncommon, TEXT("Summon_DoomBat"),      180, 18);
	AddSummon(TEXT("Chaos_T2_Summon_Chimera"),       TEXT("Summon Chimera"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Uncommon, TEXT("Summon_Chimera"),      240, 24);
	AddSummon(TEXT("Chaos_T3_Summon_Efreet"),        TEXT("Summon Efreet"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Rare,     TEXT("Summon_Efreet"),       420, 42);
	AddSummon(TEXT("Chaos_T3_Summon_Hydra"),         TEXT("Summon Hydra"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Rare,     TEXT("Summon_Hydra"),        500, 50);
	AddSummon(TEXT("Chaos_T3_Summon_Chaos_Spawn"),   TEXT("Summon Chaos Spawn"),
	          ECoMSpellRealm::Chaos,   ECoMSpellRarity::Rare,     TEXT("Summon_ChaosSpawn"),   440, 44);

	// Nature --------------------------------------------------------------
	AddSummon(TEXT("Nature_T1_Summon_Sprites"),      TEXT("Summon Sprites"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Common,   TEXT("Summon_Sprites"),       60,  6);
	AddSummon(TEXT("Nature_T2_Summon_Cockatrices"),  TEXT("Summon Cockatrices"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Uncommon, TEXT("Summon_Cockatrices"),  220, 22);
	AddSummon(TEXT("Nature_T2_Summon_Basilisk"),     TEXT("Summon Basilisk"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Uncommon, TEXT("Summon_Basilisk"),     200, 20);
	AddSummon(TEXT("Nature_T2_Summon_Stone_Giant"),  TEXT("Summon Stone Giant"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Uncommon, TEXT("Summon_StoneGiant"),   240, 24);
	// Earth Elemental already had a stub spell; rebind it to the new unit.
	AddSummon(TEXT("Nature_T3_Summon_Gorgons"),      TEXT("Summon Gorgons"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Rare,     TEXT("Summon_Gorgons"),      460, 46);
	AddSummon(TEXT("Nature_T3_Summon_Behemoth"),     TEXT("Summon Behemoth"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Rare,     TEXT("Summon_Behemoth"),     500, 50);
	AddSummon(TEXT("Nature_T3_Summon_Colossus"),     TEXT("Summon Colossus"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::Rare,     TEXT("Summon_Colossus"),     560, 56);
	AddSummon(TEXT("Nature_T4_Summon_Great_Wyrm"),   TEXT("Summon Great Wyrm"),
	          ECoMSpellRealm::Nature,  ECoMSpellRarity::VeryRare, TEXT("Summon_GreatWyrm"),    900, 90);

	// Sorcery -------------------------------------------------------------
	AddSummon(TEXT("Sorcery_T2_Summon_Phantom_Beast"),TEXT("Summon Phantom Beast"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::Uncommon, TEXT("Summon_PhantomBeast"), 180, 18);
	AddSummon(TEXT("Sorcery_T2_Summon_Air_Elemental"),TEXT("Summon Air Elemental"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::Uncommon, TEXT("Summon_AirElemental"), 160, 16);
	AddSummon(TEXT("Sorcery_T2_Summon_Nagas"),       TEXT("Summon Nagas"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::Uncommon, TEXT("Summon_Nagas"),        200, 20);
	AddSummon(TEXT("Sorcery_T3_Summon_Storm_Giant"), TEXT("Summon Storm Giant"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::Rare,     TEXT("Summon_StormGiant"),   500, 50);
	AddSummon(TEXT("Sorcery_T3_Summon_Djinn"),       TEXT("Summon Djinn"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::Rare,     TEXT("Summon_Djinn"),        520, 52);
	AddSummon(TEXT("Sorcery_T4_Summon_Sky_Drake"),   TEXT("Summon Sky Drake"),
	          ECoMSpellRealm::Sorcery, ECoMSpellRarity::VeryRare, TEXT("Summon_SkyDrake"),     900, 90);

	// Arcane --------------------------------------------------------------
	// Magic Spirit's stub spell already exists; rebind via SummonSpecID below
	// during resolve. Floating Island is a new addition.
	AddSummon(TEXT("Arcane_T2_Summon_Floating_Island"), TEXT("Summon Floating Island"),
	          ECoMSpellRealm::Arcane,  ECoMSpellRarity::Uncommon, TEXT("Summon_FloatingIsland"), 200, 20);
}
