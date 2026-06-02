// Copyright Shattered Arcana. All Rights Reserved.
// CoMDataExport.cpp -- Console-driven JSON dump of every static database.
//
// `com.export_databases` writes Saved/Modding/<table>.json files that mirror
// the in-memory data. Modders / external tools can diff against these to
// build their own data packs. A future import path can ingest the same
// format to override the static defaults — the export gives modders a
// stable schema to target.

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "CoMCore/Data/CoMSpellDatabase.h"
#include "CoMCore/Data/CoMUnitDatabase.h"
#include "CoMCore/Data/CoMBuildingDatabase.h"
#include "CoMCore/Data/CoMGlobalEnchantmentData.h"
#include "CoMCore/Scenario/CoMScenarioDatabase.h"

namespace
{
	const TCHAR* RealmTag(ECoMSpellRealm R)
	{
		switch (R)
		{
		case ECoMSpellRealm::Life:    return TEXT("Life");
		case ECoMSpellRealm::Death:   return TEXT("Death");
		case ECoMSpellRealm::Chaos:   return TEXT("Chaos");
		case ECoMSpellRealm::Nature:  return TEXT("Nature");
		case ECoMSpellRealm::Sorcery: return TEXT("Sorcery");
		case ECoMSpellRealm::Arcane:  return TEXT("Arcane");
		case ECoMSpellRealm::Binding: return TEXT("Binding");
		case ECoMSpellRealm::Spirit:  return TEXT("Spirit");
		case ECoMSpellRealm::Glamour: return TEXT("Glamour");
		default:                      return TEXT("None");
		}
	}

	const TCHAR* RarityTag(ECoMSpellRarity R)
	{
		switch (R)
		{
		case ECoMSpellRarity::Common:    return TEXT("Common");
		case ECoMSpellRarity::Uncommon:  return TEXT("Uncommon");
		case ECoMSpellRarity::Rare:      return TEXT("Rare");
		case ECoMSpellRarity::VeryRare:  return TEXT("VeryRare");
		default:                         return TEXT("");
		}
	}

	bool WriteJson(const FString& Filename, const TSharedRef<FJsonObject>& Root)
	{
		FString Out;
		const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Root, W);
		const FString Dir  = FPaths::ProjectSavedDir() / TEXT("Modding");
		IFileManager::Get().MakeDirectory(*Dir, /*tree=*/ true);
		const FString Path = Dir / Filename;
		const bool bOK = FFileHelper::SaveStringToFile(Out, *Path);
		UE_LOG(LogTemp, Log, TEXT("com.export_databases: wrote %s (%d bytes)"),
			*Path, Out.Len());
		return bOK;
	}

	void ExportSpells()
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FName& ID : CoMSpellDatabase::GetAllSpellIDs())
		{
			const FCoMSpellInfo Info = CoMSpellDatabase::GetSpellInfo(ID);
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("id"),           ID.ToString());
			O->SetStringField(TEXT("display_name"), Info.DisplayName.ToString());
			O->SetStringField(TEXT("realm"),        RealmTag(Info.Realm));
			O->SetStringField(TEXT("rarity"),       RarityTag(Info.Rarity));
			O->SetNumberField(TEXT("research_cost"), Info.ResearchCost);
			O->SetNumberField(TEXT("casting_cost"),  Info.CastingCost);
			O->SetNumberField(TEXT("upkeep_mana"),   Info.UpkeepMana);
			O->SetNumberField(TEXT("range"),         Info.Range);
			O->SetBoolField(  TEXT("summon"),        Info.bSummon);
			O->SetBoolField(  TEXT("instant"),       Info.bInstant);
			O->SetBoolField(  TEXT("ongoing"),       Info.bOngoing);
			O->SetBoolField(  TEXT("aoe"),           Info.bAOE);
			O->SetNumberField(TEXT("aoe_radius"),    Info.AOERadius);
			O->SetNumberField(TEXT("damage_base"),   Info.DamageBase);
			O->SetNumberField(TEXT("heal_amount"),   Info.HealAmount);
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		Root->SetArrayField(TEXT("spells"), Arr);
		WriteJson(TEXT("spells.json"), Root);
	}

	void ExportUnits()
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FName& ID : CoMUnitDatabase::GetAllUnitSpecIDs())
		{
			const FCoMUnitSpecInfo& U = CoMUnitDatabase::GetUnitSpec(ID);
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("id"),              ID.ToString());
			O->SetStringField(TEXT("display_name"),    U.DisplayName.ToString());
			O->SetStringField(TEXT("race"),            U.RaceTag);
			O->SetNumberField(TEXT("production_cost"), U.ProductionCost);
			O->SetNumberField(TEXT("hp"),              U.HitPoints);
			O->SetNumberField(TEXT("melee"),           U.MeleeAttack);
			O->SetNumberField(TEXT("ranged"),          U.RangedAttack);
			O->SetNumberField(TEXT("defense"),         U.Defense);
			O->SetNumberField(TEXT("resistance"),      U.Resistance);
			O->SetNumberField(TEXT("movement"),        U.Movement);
			O->SetNumberField(TEXT("figures"),         U.Figures);
			O->SetBoolField(  TEXT("engineer"),        U.bEngineer);
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		Root->SetArrayField(TEXT("units"), Arr);
		WriteJson(TEXT("units.json"), Root);
	}

	void ExportBuildings()
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FName& ID : CoMBuildingDatabase::GetAllBuildingIDs())
		{
			const FCoMBuildingInfo& B = CoMBuildingDatabase::GetBuildingInfo(ID);
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("id"),              ID.ToString());
			O->SetStringField(TEXT("display_name"),    B.DisplayName.ToString());
			O->SetNumberField(TEXT("production_cost"), B.ProductionCost);
			O->SetNumberField(TEXT("upkeep_gold"),     B.UpkeepGold);
			O->SetNumberField(TEXT("food"),            B.FoodBonus);
			O->SetNumberField(TEXT("gold"),            B.GoldBonus);
			O->SetNumberField(TEXT("production"),      B.ProductionBonus);
			O->SetNumberField(TEXT("mana"),            B.ManaBonus);
			O->SetNumberField(TEXT("research"),        B.ResearchBonus);
			O->SetNumberField(TEXT("unrest_reduction"),B.UnrestReduction);
			O->SetNumberField(TEXT("happiness"),       B.HappinessBonus);
			O->SetNumberField(TEXT("pop_cap"),         B.PopCapBonus);
			O->SetNumberField(TEXT("wall_hp"),         B.WallHP);
			O->SetNumberField(TEXT("gold_multiplier_pct"), B.GoldMultiplierPercent);
			O->SetBoolField(  TEXT("coastal_only"),    B.bCoastalOnly);
			O->SetBoolField(  TEXT("enables_heroes"),  B.bEnablesHeroRecruitment);
			O->SetBoolField(  TEXT("enables_summoning"),B.bEnablesSummoning);
			O->SetStringField(TEXT("enables_unit_tag"),B.EnablesUnitTag);
			TArray<TSharedPtr<FJsonValue>> Pre;
			for (const FName& P : B.Prerequisites)
			{
				Pre.Add(MakeShared<FJsonValueString>(P.ToString()));
			}
			O->SetArrayField(TEXT("prerequisites"),    Pre);
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		Root->SetArrayField(TEXT("buildings"), Arr);
		WriteJson(TEXT("buildings.json"), Root);
	}

	void ExportEnchantments()
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FCoMGlobalEnchantmentDef& E : CoMGlobalEnchantmentData::GetAll())
		{
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("spell_id"),      E.SpellID.ToString());
			O->SetStringField(TEXT("display_name"),  E.DisplayName.ToString());
			O->SetStringField(TEXT("realm"),         RealmTag(E.Realm));
			O->SetNumberField(TEXT("magnitude"),     E.Magnitude);
			O->SetStringField(TEXT("card_image"),    E.CardImageSlug);
			O->SetStringField(TEXT("flavor"),        E.FlavorText.ToString());
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		Root->SetArrayField(TEXT("enchantments"), Arr);
		WriteJson(TEXT("enchantments.json"), Root);
	}

	void ExportScenarios()
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FCoMScenarioDef& S : CoMScenarioDatabase::GetAll())
		{
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetStringField(TEXT("id"),            S.ScenarioID.ToString());
			O->SetStringField(TEXT("display_name"),  S.DisplayName.ToString());
			O->SetStringField(TEXT("synopsis"),      S.Synopsis.ToString());
			O->SetNumberField(TEXT("num_wizards"),   S.NumWizards);
			O->SetNumberField(TEXT("seed"),          S.Seed);
			O->SetNumberField(TEXT("max_turns"),     S.MaxTurns);
			TArray<TSharedPtr<FJsonValue>> Bonuses;
			for (const FCoMScenarioWizardBonus& B : S.WizardBonuses)
			{
				TSharedRef<FJsonObject> BJ = MakeShared<FJsonObject>();
				BJ->SetNumberField(TEXT("wizard_index"), B.WizardIndex);
				BJ->SetNumberField(TEXT("extra_cities"), B.ExtraCities);
				BJ->SetNumberField(TEXT("extra_mana"),   B.ExtraMana);
				BJ->SetNumberField(TEXT("extra_gold"),   B.ExtraGold);
				BJ->SetStringField(TEXT("starting_global"), B.StartingGlobal.ToString());
				BJ->SetNumberField(TEXT("aggression_mult"), B.AggressionMultiplier);
				Bonuses.Add(MakeShared<FJsonValueObject>(BJ));
			}
			O->SetArrayField(TEXT("bonuses"), Bonuses);
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		Root->SetArrayField(TEXT("scenarios"), Arr);
		WriteJson(TEXT("scenarios.json"), Root);
	}
} // namespace

// Console: dump every static database to Saved/Modding/*.json.
static FAutoConsoleCommandWithWorldAndArgs GExportDatabasesCmd(
	TEXT("com.export_databases"),
	TEXT("Dump every static database (spells/units/buildings/enchantments/scenarios) as JSON under Saved/Modding/."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& /*Args*/, UWorld* /*World*/)
		{
			ExportSpells();
			ExportUnits();
			ExportBuildings();
			ExportEnchantments();
			ExportScenarios();
		}));
