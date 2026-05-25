// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMGlobalEnchantmentData.h"

namespace
{
	TArray<FCoMGlobalEnchantmentDef> GDefs;
	TMap<FName, int32>               GIndex;   // SpellID -> index into GDefs
	FCoMGlobalEnchantmentDef         GInvalid; // returned for non-enchantments
	bool                             GBuilt = false;

	void Add(const FName& SpellID, const TCHAR* Display, ECoMSpellRealm Realm,
	         ECoMEnchantEffect Effect, ECoMEnchantScope Scope, int32 Magnitude,
	         const TCHAR* Slug, const TCHAR* Flavor)
	{
		FCoMGlobalEnchantmentDef D;
		D.SpellID       = SpellID;
		D.DisplayName   = FText::FromString(Display);
		D.Realm         = Realm;
		D.Effect        = Effect;
		D.Scope         = Scope;
		D.Magnitude     = Magnitude;
		D.CardImageSlug = Slug;
		D.FlavorText    = FText::FromString(Flavor);
		GIndex.Add(SpellID, GDefs.Num());
		GDefs.Add(MoveTemp(D));
	}

	void BuildIfNeeded()
	{
		if (GBuilt) { return; }
		GBuilt = true;

		using EReal = ECoMSpellRealm;
		using EEff  = ECoMEnchantEffect;
		using EScope= ECoMEnchantScope;

		// ── Life ──────────────────────────────────────────────────────────
		Add(TEXT("Just_Cause"), TEXT("Just Cause"), EReal::Life,
			EEff::ReduceOwnUnrest, EScope::Caster, 2, TEXT("just_cause"),
			TEXT("The righteousness of your cause calms unrest in all your cities."));
		Add(TEXT("Crusade"), TEXT("Crusade"), EReal::Life,
			EEff::BuffOwnUnitsCombat, EScope::Caster, 1, TEXT("crusade"),
			TEXT("All your normal units gain +1 to hit and elevated morale."));
		Add(TEXT("Tranquility"), TEXT("Tranquility"), EReal::Life,
			EEff::GlobalPeaceAura, EScope::AllWizards, 1, TEXT("tranquility"),
			TEXT("A pall of peace blunts the world's most destructive enchantments."));
		Add(TEXT("Life_T4_Consecration"), TEXT("Consecration"), EReal::Life,
			EEff::ConsecrateRealm, EScope::Caster, 1, TEXT("consecration"),
			TEXT("Your lands are consecrated, resisting death and corruption."));

		// ── Death ─────────────────────────────────────────────────────────
		Add(TEXT("Eternal_Night"), TEXT("Eternal Night"), EReal::Death,
			EEff::BuffOwnUnitsCombat, EScope::Caster, 1, TEXT("eternal_night"),
			TEXT("Endless darkness empowers your death creatures and blinds rivals."));
		Add(TEXT("Zombie_Mastery"), TEXT("Zombie Mastery"), EReal::Death,
			EEff::RaiseSlainAsZombies, EScope::Caster, 1, TEXT("zombie_mastery"),
			TEXT("The slain rise again under your banner as zombies."));
		Add(TEXT("Death_T4_Death_Wish"), TEXT("Death Wish"), EReal::Death,
			EEff::GlobalDeathToll, EScope::Enemies, 1, TEXT("death_wish"),
			TEXT("A toll of death is exacted from every rival realm."));

		// ── Chaos ─────────────────────────────────────────────────────────
		Add(TEXT("Chaos_Surge"), TEXT("Chaos Surge"), EReal::Chaos,
			EEff::GlobalChaosBoon, EScope::Caster, 1, TEXT("chaos_surge"),
			TEXT("Raw chaos surges through your magic, empowering your spells."));
		Add(TEXT("Great_Wasting"), TEXT("Great Wasting"), EReal::Chaos,
			EEff::GlobalCityDecay, EScope::Enemies, 1, TEXT("great_wasting"),
			TEXT("The land withers; enemy cities lose population each turn."));
		Add(TEXT("Doom_Mastery"), TEXT("Doom Mastery"), EReal::Chaos,
			EEff::GlobalChaosBoon, EScope::Caster, 2, TEXT("doom_mastery"),
			TEXT("Doom answers your call, strengthening your every conjuration."));
		Add(TEXT("Armageddon_Clock"), TEXT("Armageddon Clock"), EReal::Chaos,
			EEff::DoomsdayCountdown, EScope::AllWizards, 15, TEXT("armageddon_clock"),
			TEXT("The doomsday clock turns; chaos floods to its master each turn."));
		Add(TEXT("Chaos_T4_Call_The_Void"), TEXT("Call the Void"), EReal::Chaos,
			EEff::GlobalCityDecay, EScope::Enemies, 1, TEXT("call_the_void"),
			TEXT("The void devours rival realms from within."));

		// ── Nature ────────────────────────────────────────────────────────
		Add(TEXT("Nature_Awareness"), TEXT("Nature Awareness"), EReal::Nature,
			EEff::GlobalMapVision, EScope::Caster, 1, TEXT("nature_awareness"),
			TEXT("Nature's senses reveal the entire world to you."));
		Add(TEXT("Herb_Mastery"), TEXT("Herb Mastery"), EReal::Nature,
			EEff::HealOwnUnits, EScope::Caster, 2, TEXT("herb_mastery"),
			TEXT("Healing herbs mend your units across the realm each turn."));
		Add(TEXT("Gaia_Force"), TEXT("Gaia Force"), EReal::Nature,
			EEff::BoostOwnMana, EScope::Caster, 10, TEXT("gaia_force"),
			TEXT("The living world channels extra mana to you each turn."));
		Add(TEXT("Nature_T4_Natures_Wrath"), TEXT("Nature's Wrath"), EReal::Nature,
			EEff::GlobalCityDecay, EScope::Enemies, 1, TEXT("natures_wrath"),
			TEXT("The wilds turn savage against those who defy you."));

		// ── Sorcery ───────────────────────────────────────────────────────
		Add(TEXT("Suppress_Magic"), TEXT("Suppress Magic"), EReal::Sorcery,
			EEff::RaiseEnemyCastCost, EScope::Enemies, 50, TEXT("suppress_magic"),
			TEXT("Rival spellcasting is suppressed; their spells cost far more."));
		Add(TEXT("Detect_Magic"), TEXT("Detect Magic"), EReal::Sorcery,
			EEff::RevealEnemyMagic, EScope::Caster, 1, TEXT("detect_magic"),
			TEXT("You sense every spell being cast across the world."));
		Add(TEXT("Diplomatic_Scrying"), TEXT("Diplomatic Scrying"), EReal::Sorcery,
			EEff::RevealEnemyPlans, EScope::Caster, 1, TEXT("diplomatic_scrying"),
			TEXT("You scry the courts of your rivals and learn their intentions."));
		Add(TEXT("Aether_Binding"), TEXT("Aether Binding"), EReal::Sorcery,
			EEff::ScryEnchantments, EScope::Caster, 1, TEXT("aether_binding"),
			TEXT("The aether binds; you perceive all active enchantments."));
		Add(TEXT("Sorcery_T4_Spell_Of_Mastery"), TEXT("Spell of Mastery"), EReal::Sorcery,
			EEff::InstantMagicVictory, EScope::Caster, 0, TEXT("spell_of_mastery"),
			TEXT("The ultimate spell. Its completion wins the game."));

		// ── Arcane ────────────────────────────────────────────────────────
		Add(TEXT("Spell_of_Mastery"), TEXT("Spell of Mastery"), EReal::Arcane,
			EEff::InstantMagicVictory, EScope::Caster, 0, TEXT("spell_of_mastery"),
			TEXT("The ultimate spell. Its completion wins the game."));
		Add(TEXT("Spell_of_Return"), TEXT("Spell of Return"), EReal::Arcane,
			EEff::BanishReturn, EScope::Caster, 0, TEXT("spell_of_return"),
			TEXT("A banished wizard returns to the world of the living."));
		Add(TEXT("Arcane_T1_Detect_Magic"), TEXT("Detect Magic"), EReal::Arcane,
			EEff::RevealEnemyMagic, EScope::Caster, 1, TEXT("detect_magic"),
			TEXT("You sense every spell being cast across the world."));

		// ── Spirit ────────────────────────────────────────────────────────
		Add(TEXT("Spirit_T1_Dream_Vision"), TEXT("Dream Vision"), EReal::Spirit,
			EEff::RevealEnemyPlans, EScope::Caster, 1, TEXT("dream_vision"),
			TEXT("Dreams carry visions of distant lands and rival schemes."));

		GInvalid = FCoMGlobalEnchantmentDef();
	}
}

const FCoMGlobalEnchantmentDef& CoMGlobalEnchantmentData::Get(FName SpellID)
{
	BuildIfNeeded();
	if (const int32* Idx = GIndex.Find(SpellID))
	{
		return GDefs[*Idx];
	}
	return GInvalid;
}

bool CoMGlobalEnchantmentData::IsGlobalEnchantment(FName SpellID)
{
	BuildIfNeeded();
	return GIndex.Contains(SpellID);
}

const TArray<FCoMGlobalEnchantmentDef>& CoMGlobalEnchantmentData::GetAll()
{
	BuildIfNeeded();
	return GDefs;
}
