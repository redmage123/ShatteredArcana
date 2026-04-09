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
		FCoMSpellInfo S;
		S.SpellID = FName(TEXT("Nature_T3_Summon_Earth_Elemental"));
		S.DisplayName = FText::FromString(TEXT("Summon Earth Elemental"));
		S.Realm = ECoMSpellRealm::Nature; S.Rarity = ECoMSpellRarity::Rare;
		S.Scope = ECoMSpellScope::Combat;
		S.TargetType = ECoMSpellTarget::TileTarget; S.EffectType = ECoMSpellEffect::Summon;
		S.ResearchCost = 400; S.CastingCost = 40; S.Range = 8;
		S.bSummon = true;
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
}
