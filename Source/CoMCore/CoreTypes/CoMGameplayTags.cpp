// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameplayTags.cpp — Gameplay Tag definitions
// One UE_DEFINE_GAMEPLAY_TAG per UE_DECLARE_GAMEPLAY_TAG_EXTERN in CoMGameplayTags.h

#include "CoMGameplayTags.h"

namespace CoMTags
{
	// ── Planes ───────────────────────────────────────────────────────────────
	namespace Plane
	{
		UE_DEFINE_GAMEPLAY_TAG(Aurelith,   "CoM.Plane.Aurelith")
		UE_DEFINE_GAMEPLAY_TAG(Noctharion, "CoM.Plane.Noctharion")
		UE_DEFINE_GAMEPLAY_TAG(Verdantis,  "CoM.Plane.Verdantis")
		UE_DEFINE_GAMEPLAY_TAG(Infernyx,   "CoM.Plane.Infernyx")
		UE_DEFINE_GAMEPLAY_TAG(Aethermist, "CoM.Plane.Aethermist")
		UE_DEFINE_GAMEPLAY_TAG(Abyssal,    "CoM.Plane.Abyssal")
		UE_DEFINE_GAMEPLAY_TAG(Ethereal,   "CoM.Plane.Ethereal")
		UE_DEFINE_GAMEPLAY_TAG(Feywild,    "CoM.Plane.Feywild")
		// Infernal merged into Infernyx — no tag registered.
	}

	// ── Realms ───────────────────────────────────────────────────────────────
	namespace Realm
	{
		UE_DEFINE_GAMEPLAY_TAG(Life,    "CoM.Realm.Life")
		UE_DEFINE_GAMEPLAY_TAG(Death,   "CoM.Realm.Death")
		UE_DEFINE_GAMEPLAY_TAG(Chaos,   "CoM.Realm.Chaos")
		UE_DEFINE_GAMEPLAY_TAG(Nature,  "CoM.Realm.Nature")
		UE_DEFINE_GAMEPLAY_TAG(Sorcery, "CoM.Realm.Sorcery")
		UE_DEFINE_GAMEPLAY_TAG(Arcane,  "CoM.Realm.Arcane")
		UE_DEFINE_GAMEPLAY_TAG(Binding, "CoM.Realm.Binding")
		UE_DEFINE_GAMEPLAY_TAG(Spirit,  "CoM.Realm.Spirit")
	}

	// ── Races ────────────────────────────────────────────────────────────────
	namespace Race
	{
		UE_DEFINE_GAMEPLAY_TAG(HighMen,     "CoM.Race.HighMen")
		UE_DEFINE_GAMEPLAY_TAG(HighElves,   "CoM.Race.HighElves")
		UE_DEFINE_GAMEPLAY_TAG(Halflings,   "CoM.Race.Halflings")
		UE_DEFINE_GAMEPLAY_TAG(Nomads,      "CoM.Race.Nomads")
		UE_DEFINE_GAMEPLAY_TAG(Orcs,        "CoM.Race.Orcs")
		UE_DEFINE_GAMEPLAY_TAG(Barbarians,  "CoM.Race.Barbarians")

		UE_DEFINE_GAMEPLAY_TAG(DarkElves,   "CoM.Race.DarkElves")
		UE_DEFINE_GAMEPLAY_TAG(Dwarves,     "CoM.Race.Dwarves")
		UE_DEFINE_GAMEPLAY_TAG(Draconians,  "CoM.Race.Draconians")
		UE_DEFINE_GAMEPLAY_TAG(Beastmen,    "CoM.Race.Beastmen")
		UE_DEFINE_GAMEPLAY_TAG(Trolls,      "CoM.Race.Trolls")

		UE_DEFINE_GAMEPLAY_TAG(Chithari,    "CoM.Race.Chithari")
		UE_DEFINE_GAMEPLAY_TAG(Lizardmen,   "CoM.Race.Lizardmen")
		UE_DEFINE_GAMEPLAY_TAG(Gnolls,      "CoM.Race.Gnolls")
		UE_DEFINE_GAMEPLAY_TAG(Treants,     "CoM.Race.Treants")
		UE_DEFINE_GAMEPLAY_TAG(Myconids,    "CoM.Race.Myconids")

		UE_DEFINE_GAMEPLAY_TAG(Salamanders, "CoM.Race.Salamanders")
		UE_DEFINE_GAMEPLAY_TAG(Ashen,       "CoM.Race.Ashen")
		UE_DEFINE_GAMEPLAY_TAG(Obsidians,   "CoM.Race.Obsidians")
		UE_DEFINE_GAMEPLAY_TAG(Demonkin,    "CoM.Race.Demonkin")

		UE_DEFINE_GAMEPLAY_TAG(Sylphs,       "CoM.Race.Sylphs")
		UE_DEFINE_GAMEPLAY_TAG(Crystallines, "CoM.Race.Crystallines")
		UE_DEFINE_GAMEPLAY_TAG(Voidwalkers,  "CoM.Race.Voidwalkers")
		UE_DEFINE_GAMEPLAY_TAG(Dreamweavers, "CoM.Race.Dreamweavers")

		UE_DEFINE_GAMEPLAY_TAG(DeepDwarves,  "CoM.Race.DeepDwarves")
		UE_DEFINE_GAMEPLAY_TAG(Goblins,      "CoM.Race.Goblins")
		UE_DEFINE_GAMEPLAY_TAG(Drow,         "CoM.Race.Drow")
		UE_DEFINE_GAMEPLAY_TAG(Shades,       "CoM.Race.Shades")
		UE_DEFINE_GAMEPLAY_TAG(Myrmidons,    "CoM.Race.Myrmidons")
		UE_DEFINE_GAMEPLAY_TAG(Sporekin,     "CoM.Race.Sporekin")
		UE_DEFINE_GAMEPLAY_TAG(MagmaDwarves, "CoM.Race.MagmaDwarves")
		UE_DEFINE_GAMEPLAY_TAG(ImpSwarms,    "CoM.Race.ImpSwarms")
		UE_DEFINE_GAMEPLAY_TAG(Mindflayers,  "CoM.Race.Mindflayers")
		UE_DEFINE_GAMEPLAY_TAG(EchoFolk,     "CoM.Race.EchoFolk")

		// Abyssal surface
		UE_DEFINE_GAMEPLAY_TAG(Maleficii,     "CoM.Race.Maleficii")
		UE_DEFINE_GAMEPLAY_TAG(Gorethians,    "CoM.Race.Gorethians")
		UE_DEFINE_GAMEPLAY_TAG(Plagueborn,    "CoM.Race.Plagueborn")
		UE_DEFINE_GAMEPLAY_TAG(Voidlings,     "CoM.Race.Voidlings")
		UE_DEFINE_GAMEPLAY_TAG(Ashwalkers,    "CoM.Race.Ashwalkers")

		// Infernal surface
		UE_DEFINE_GAMEPLAY_TAG(Hellbound,     "CoM.Race.Hellbound")
		UE_DEFINE_GAMEPLAY_TAG(Nightbinders,  "CoM.Race.Nightbinders")
		UE_DEFINE_GAMEPLAY_TAG(Ferrovians,    "CoM.Race.Ferrovians")
		UE_DEFINE_GAMEPLAY_TAG(Ashguard,      "CoM.Race.Ashguard")
		UE_DEFINE_GAMEPLAY_TAG(ShadowPact,    "CoM.Race.ShadowPact")

		// Ethereal surface
		UE_DEFINE_GAMEPLAY_TAG(Wraiths,        "CoM.Race.Wraiths")
		UE_DEFINE_GAMEPLAY_TAG(Phantoms,       "CoM.Race.Phantoms")
		UE_DEFINE_GAMEPLAY_TAG(Mistborn,       "CoM.Race.Mistborn")
		UE_DEFINE_GAMEPLAY_TAG(Thoughteaters,  "CoM.Race.Thoughteaters")
		UE_DEFINE_GAMEPLAY_TAG(Spectrels,      "CoM.Race.Spectrels")

		// Abyssal Underdark
		UE_DEFINE_GAMEPLAY_TAG(ChaosSpawn,        "CoM.Race.ChaosSpawn")
		UE_DEFINE_GAMEPLAY_TAG(VoidStalkers,       "CoM.Race.VoidStalkers")

		// Infernal Underdark
		UE_DEFINE_GAMEPLAY_TAG(SoulForgers,        "CoM.Race.SoulForgers")
		UE_DEFINE_GAMEPLAY_TAG(HellmirrorDwarves,  "CoM.Race.HellmirrorDwarves")

		// Ethereal Underdark
		UE_DEFINE_GAMEPLAY_TAG(EchoWraiths,        "CoM.Race.EchoWraiths")
		UE_DEFINE_GAMEPLAY_TAG(MemoryDevourers,    "CoM.Race.MemoryDevourers")

		// Aquatic
		UE_DEFINE_GAMEPLAY_TAG(Merfolk,       "CoM.Race.Merfolk")
		UE_DEFINE_GAMEPLAY_TAG(DeepOnes,      "CoM.Race.DeepOnes")
		UE_DEFINE_GAMEPLAY_TAG(Tideshifters,  "CoM.Race.Tideshifters")
		UE_DEFINE_GAMEPLAY_TAG(LavaNaga,      "CoM.Race.LavaNaga")
		UE_DEFINE_GAMEPLAY_TAG(EtherSwimmers, "CoM.Race.EtherSwimmers")
	}

	// ── Skills ───────────────────────────────────────────────────────────────
	namespace Skill
	{
		UE_DEFINE_GAMEPLAY_TAG(Flying,           "CoM.Skill.Flying")
		UE_DEFINE_GAMEPLAY_TAG(Swimming,         "CoM.Skill.Swimming")
		UE_DEFINE_GAMEPLAY_TAG(Burrowing,        "CoM.Skill.Burrowing")
		UE_DEFINE_GAMEPLAY_TAG(PhaseWalk,        "CoM.Skill.PhaseWalk")
		UE_DEFINE_GAMEPLAY_TAG(Pathfinding,      "CoM.Skill.Pathfinding")
		UE_DEFINE_GAMEPLAY_TAG(Mountaineer,      "CoM.Skill.Mountaineer")
		UE_DEFINE_GAMEPLAY_TAG(Forester,         "CoM.Skill.Forester")
		UE_DEFINE_GAMEPLAY_TAG(PlaneStrider,     "CoM.Skill.PlaneStrider")
		UE_DEFINE_GAMEPLAY_TAG(FirstStrike,      "CoM.Skill.FirstStrike")
		UE_DEFINE_GAMEPLAY_TAG(Armor_Piercing,   "CoM.Skill.ArmorPiercing")
		UE_DEFINE_GAMEPLAY_TAG(Regeneration,     "CoM.Skill.Regeneration")
		UE_DEFINE_GAMEPLAY_TAG(Invisibility,     "CoM.Skill.Invisibility")
		UE_DEFINE_GAMEPLAY_TAG(Illusionary,      "CoM.Skill.Illusionary")
		UE_DEFINE_GAMEPLAY_TAG(NonCorporeal,     "CoM.Skill.NonCorporeal")
		UE_DEFINE_GAMEPLAY_TAG(LifeSteal,        "CoM.Skill.LifeSteal")
		UE_DEFINE_GAMEPLAY_TAG(Poison,           "CoM.Skill.Poison")
		UE_DEFINE_GAMEPLAY_TAG(Petrifying,       "CoM.Skill.Petrifying")
		UE_DEFINE_GAMEPLAY_TAG(Fear,             "CoM.Skill.Fear")
		UE_DEFINE_GAMEPLAY_TAG(DoomBolt,         "CoM.Skill.DoomBolt")
		UE_DEFINE_GAMEPLAY_TAG(Chaos_Touch,      "CoM.Skill.ChaosTouch")
		UE_DEFINE_GAMEPLAY_TAG(MagicImmunity,    "CoM.Skill.MagicImmunity")
		UE_DEFINE_GAMEPLAY_TAG(ColdImmunity,     "CoM.Skill.ColdImmunity")
		UE_DEFINE_GAMEPLAY_TAG(FireImmunity,     "CoM.Skill.FireImmunity")
		UE_DEFINE_GAMEPLAY_TAG(DeathImmunity,    "CoM.Skill.DeathImmunity")
		UE_DEFINE_GAMEPLAY_TAG(PoisonImmunity,   "CoM.Skill.PoisonImmunity")
		UE_DEFINE_GAMEPLAY_TAG(LightningImmunity,"CoM.Skill.LightningImmunity")
		UE_DEFINE_GAMEPLAY_TAG(Leadership,       "CoM.Skill.Leadership")
		UE_DEFINE_GAMEPLAY_TAG(Scouting,         "CoM.Skill.Scouting")
		UE_DEFINE_GAMEPLAY_TAG(Stealth,          "CoM.Skill.Stealth")
		UE_DEFINE_GAMEPLAY_TAG(ArmyBreaker,      "CoM.Skill.ArmyBreaker")
		UE_DEFINE_GAMEPLAY_TAG(Engineering,      "CoM.Skill.Engineering")
		UE_DEFINE_GAMEPLAY_TAG(RuneInscriber,    "CoM.Skill.RuneInscriber")
		UE_DEFINE_GAMEPLAY_TAG(SpellCasting,     "CoM.Skill.SpellCasting")
		UE_DEFINE_GAMEPLAY_TAG(RitualParticipant,"CoM.Skill.RitualParticipant")
		UE_DEFINE_GAMEPLAY_TAG(TunnelVision,     "CoM.Skill.TunnelVision")
		UE_DEFINE_GAMEPLAY_TAG(TunnelBuilder,    "CoM.Skill.TunnelBuilder")
		UE_DEFINE_GAMEPLAY_TAG(Darkvision,       "CoM.Skill.Darkvision")
		UE_DEFINE_GAMEPLAY_TAG(FireBreath,       "CoM.Skill.FireBreath")
		UE_DEFINE_GAMEPLAY_TAG(LightningBreath,  "CoM.Skill.LightningBreath")
		UE_DEFINE_GAMEPLAY_TAG(PoisonBreath,     "CoM.Skill.PoisonBreath")
		UE_DEFINE_GAMEPLAY_TAG(AcidBreath,       "CoM.Skill.AcidBreath")
		UE_DEFINE_GAMEPLAY_TAG(ColdBreath,       "CoM.Skill.ColdBreath")
		UE_DEFINE_GAMEPLAY_TAG(PsychicBreath,    "CoM.Skill.PsychicBreath")
	}

	// ── Spells ───────────────────────────────────────────────────────────────
	namespace Spell
	{
		UE_DEFINE_GAMEPLAY_TAG(Category_Summon,      "CoM.Spell.Category.Summon")
		UE_DEFINE_GAMEPLAY_TAG(Category_Enchantment, "CoM.Spell.Category.Enchantment")
		UE_DEFINE_GAMEPLAY_TAG(Category_Combat,      "CoM.Spell.Category.Combat")
		UE_DEFINE_GAMEPLAY_TAG(Category_City,        "CoM.Spell.Category.City")
		UE_DEFINE_GAMEPLAY_TAG(Category_Global,      "CoM.Spell.Category.Global")
		UE_DEFINE_GAMEPLAY_TAG(Category_Terrain,     "CoM.Spell.Category.Terrain")
		UE_DEFINE_GAMEPLAY_TAG(Category_Planar,      "CoM.Spell.Category.Planar")
		UE_DEFINE_GAMEPLAY_TAG(Category_Trade,       "CoM.Spell.Category.Trade")
		UE_DEFINE_GAMEPLAY_TAG(Category_Ritual,      "CoM.Spell.Category.Ritual")
		UE_DEFINE_GAMEPLAY_TAG(Category_Rune,        "CoM.Spell.Category.Rune")
		UE_DEFINE_GAMEPLAY_TAG(Category_Weather,     "CoM.Spell.Category.Weather")
		UE_DEFINE_GAMEPLAY_TAG(Category_LeyLine,     "CoM.Spell.Category.LeyLine")
		UE_DEFINE_GAMEPLAY_TAG(Category_Diplomatic,  "CoM.Spell.Category.Diplomatic")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Fire,          "CoM.Spell.Effect.Fire")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Cold,          "CoM.Spell.Effect.Cold")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Lightning,     "CoM.Spell.Effect.Lightning")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Poison,        "CoM.Spell.Effect.Poison")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Death,         "CoM.Spell.Effect.Death")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Holy,          "CoM.Spell.Effect.Holy")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Chaos,         "CoM.Spell.Effect.Chaos")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Mind,          "CoM.Spell.Effect.Mind")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Void,          "CoM.Spell.Effect.Void")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Nature,        "CoM.Spell.Effect.Nature")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Healing,       "CoM.Spell.Effect.Healing")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Buff,          "CoM.Spell.Effect.Buff")
		UE_DEFINE_GAMEPLAY_TAG(Effect_Debuff,        "CoM.Spell.Effect.Debuff")
	}

	// ── Buildings ────────────────────────────────────────────────────────────
	namespace Building
	{
		UE_DEFINE_GAMEPLAY_TAG(Barracks,           "CoM.Building.Barracks")
		UE_DEFINE_GAMEPLAY_TAG(Stables,            "CoM.Building.Stables")
		UE_DEFINE_GAMEPLAY_TAG(ArmorersGuild,      "CoM.Building.ArmorersGuild")
		UE_DEFINE_GAMEPLAY_TAG(WeaponsmithsGuild,  "CoM.Building.WeaponsmithsGuild")
		UE_DEFINE_GAMEPLAY_TAG(FightersGuild,      "CoM.Building.FightersGuild")
		UE_DEFINE_GAMEPLAY_TAG(WarCollege,         "CoM.Building.WarCollege")
		UE_DEFINE_GAMEPLAY_TAG(Walls,              "CoM.Building.Walls")
		UE_DEFINE_GAMEPLAY_TAG(Citadel,            "CoM.Building.Citadel")
		UE_DEFINE_GAMEPLAY_TAG(Marketplace,        "CoM.Building.Marketplace")
		UE_DEFINE_GAMEPLAY_TAG(TradePost,          "CoM.Building.TradePost")
		UE_DEFINE_GAMEPLAY_TAG(MerchantGuild_Bld,  "CoM.Building.MerchantGuild")
		UE_DEFINE_GAMEPLAY_TAG(GrandBazaar,        "CoM.Building.GrandBazaar")
		UE_DEFINE_GAMEPLAY_TAG(Sawmill,            "CoM.Building.Sawmill")
		UE_DEFINE_GAMEPLAY_TAG(Miners_Guild,       "CoM.Building.MinersGuild")
		UE_DEFINE_GAMEPLAY_TAG(Bank,               "CoM.Building.Bank")
		UE_DEFINE_GAMEPLAY_TAG(WizardsTower,       "CoM.Building.WizardsTower")
		UE_DEFINE_GAMEPLAY_TAG(Library,            "CoM.Building.Library")
		UE_DEFINE_GAMEPLAY_TAG(AlchemistsLab,      "CoM.Building.AlchemistsLab")
		UE_DEFINE_GAMEPLAY_TAG(SorcerersGuild,     "CoM.Building.SorcerersGuild")
		UE_DEFINE_GAMEPLAY_TAG(MagesGuild,         "CoM.Building.MagesGuild")
		UE_DEFINE_GAMEPLAY_TAG(ManaNode_Bld,       "CoM.Building.ManaNode")
		UE_DEFINE_GAMEPLAY_TAG(Shrine,             "CoM.Building.Shrine")
		UE_DEFINE_GAMEPLAY_TAG(Temple,             "CoM.Building.Temple")
		UE_DEFINE_GAMEPLAY_TAG(Cathedral,          "CoM.Building.Cathedral")
		UE_DEFINE_GAMEPLAY_TAG(Parthenon,          "CoM.Building.Parthenon")
		UE_DEFINE_GAMEPLAY_TAG(GranaryFarm,        "CoM.Building.GranaryFarm")
		UE_DEFINE_GAMEPLAY_TAG(FarmersMarket,      "CoM.Building.FarmersMarket")
		UE_DEFINE_GAMEPLAY_TAG(Irrigated,          "CoM.Building.Irrigated")
		UE_DEFINE_GAMEPLAY_TAG(FungalFarm,         "CoM.Building.FungalFarm")
		UE_DEFINE_GAMEPLAY_TAG(DeepMine_Bld,       "CoM.Building.DeepMine")
		UE_DEFINE_GAMEPLAY_TAG(TunnelNetwork,      "CoM.Building.TunnelNetwork")
		UE_DEFINE_GAMEPLAY_TAG(SubterraneanHarbor, "CoM.Building.SubterraneanHarbor")
		UE_DEFINE_GAMEPLAY_TAG(EchoTower,          "CoM.Building.EchoTower")
		UE_DEFINE_GAMEPLAY_TAG(MagicLightSource,   "CoM.Building.MagicLightSource")
		UE_DEFINE_GAMEPLAY_TAG(ShipwrightsGuild,   "CoM.Building.ShipwrightsGuild")
		UE_DEFINE_GAMEPLAY_TAG(Harbor_Bld,         "CoM.Building.Harbor")
		UE_DEFINE_GAMEPLAY_TAG(TradeFleet_Bld,     "CoM.Building.TradeFleet")
		UE_DEFINE_GAMEPLAY_TAG(CoralPalace,        "CoM.Building.CoralPalace")
		UE_DEFINE_GAMEPLAY_TAG(TidalGenerator,     "CoM.Building.TidalGenerator")
		UE_DEFINE_GAMEPLAY_TAG(PearlFarm,          "CoM.Building.PearlFarm")
		UE_DEFINE_GAMEPLAY_TAG(PressureDome,       "CoM.Building.PressureDome")
	}

	// ── Retorts ──────────────────────────────────────────────────────────────
	namespace Retort
	{
		UE_DEFINE_GAMEPLAY_TAG(Archmage,      "CoM.Retort.Archmage")
		UE_DEFINE_GAMEPLAY_TAG(Channeler,     "CoM.Retort.Channeler")
		UE_DEFINE_GAMEPLAY_TAG(SageMaster,    "CoM.Retort.SageMaster")
		UE_DEFINE_GAMEPLAY_TAG(Famous,        "CoM.Retort.Famous")
		UE_DEFINE_GAMEPLAY_TAG(Myrran,        "CoM.Retort.Myrran")
		UE_DEFINE_GAMEPLAY_TAG(Chaos_Mastery, "CoM.Retort.ChaosMastery")
		UE_DEFINE_GAMEPLAY_TAG(Life_Mastery,  "CoM.Retort.LifeMastery")
		UE_DEFINE_GAMEPLAY_TAG(Death_Mastery, "CoM.Retort.DeathMastery")
		UE_DEFINE_GAMEPLAY_TAG(Nature_Mastery,"CoM.Retort.NatureMastery")
		UE_DEFINE_GAMEPLAY_TAG(Sorcery_Mastery,"CoM.Retort.SorceryMastery")
		UE_DEFINE_GAMEPLAY_TAG(DivinePower,   "CoM.Retort.DivinePower")
		UE_DEFINE_GAMEPLAY_TAG(InfernalPower, "CoM.Retort.InfernalPower")
		UE_DEFINE_GAMEPLAY_TAG(Warlord,       "CoM.Retort.Warlord")
		UE_DEFINE_GAMEPLAY_TAG(Conjurer,      "CoM.Retort.Conjurer")
		UE_DEFINE_GAMEPLAY_TAG(NodeMastery,   "CoM.Retort.NodeMastery")
		UE_DEFINE_GAMEPLAY_TAG(Artificer,     "CoM.Retort.Artificer")
	}

	// ── Terrain ──────────────────────────────────────────────────────────────
	namespace Terrain
	{
		UE_DEFINE_GAMEPLAY_TAG(IsMagical,    "CoM.Terrain.IsMagical")
		UE_DEFINE_GAMEPLAY_TAG(HasRoad,      "CoM.Terrain.HasRoad")
		UE_DEFINE_GAMEPLAY_TAG(HasRiver,     "CoM.Terrain.HasRiver")
		UE_DEFINE_GAMEPLAY_TAG(IsCoastal,    "CoM.Terrain.IsCoastal")
		UE_DEFINE_GAMEPLAY_TAG(IsDeepOcean,  "CoM.Terrain.IsDeepOcean")
		UE_DEFINE_GAMEPLAY_TAG(IsVolcanic,   "CoM.Terrain.IsVolcanic")
		UE_DEFINE_GAMEPLAY_TAG(IsCorrupted,  "CoM.Terrain.IsCorrupted")
		UE_DEFINE_GAMEPLAY_TAG(OnLeyLine,    "CoM.Terrain.OnLeyLine")
		UE_DEFINE_GAMEPLAY_TAG(IsLeyNexus,   "CoM.Terrain.IsLeyNexus")
		UE_DEFINE_GAMEPLAY_TAG(IsUnderdark,  "CoM.Terrain.IsUnderdark")
		UE_DEFINE_GAMEPLAY_TAG(IsUnderwater, "CoM.Terrain.IsUnderwater")
	}

	// ── Resources ────────────────────────────────────────────────────────────
	namespace Resource
	{
		UE_DEFINE_GAMEPLAY_TAG(Iron,        "CoM.Resource.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Coal,        "CoM.Resource.Coal")
		UE_DEFINE_GAMEPLAY_TAG(GoldOre,     "CoM.Resource.GoldOre")
		UE_DEFINE_GAMEPLAY_TAG(Gems,        "CoM.Resource.Gems")
		UE_DEFINE_GAMEPLAY_TAG(Silver,      "CoM.Resource.Silver")
		UE_DEFINE_GAMEPLAY_TAG(Horses,      "CoM.Resource.Horses")
		UE_DEFINE_GAMEPLAY_TAG(Nightshade,  "CoM.Resource.Nightshade")
		UE_DEFINE_GAMEPLAY_TAG(Mithril,     "CoM.Resource.Mithril")
		UE_DEFINE_GAMEPLAY_TAG(Adamantium,  "CoM.Resource.Adamantium")
		UE_DEFINE_GAMEPLAY_TAG(Orichalcon,  "CoM.Resource.Orichalcon")
		UE_DEFINE_GAMEPLAY_TAG(Aetherium,   "CoM.Resource.Aetherium")
		UE_DEFINE_GAMEPLAY_TAG(Lifewood,    "CoM.Resource.Lifewood")
		UE_DEFINE_GAMEPLAY_TAG(Fireglass,   "CoM.Resource.Fireglass")
	}

	// ── Dragons ──────────────────────────────────────────────────────────────
	namespace Dragon
	{
		UE_DEFINE_GAMEPLAY_TAG(Chromatic,       "CoM.Dragon.Chromatic")
		UE_DEFINE_GAMEPLAY_TAG(Metallic,        "CoM.Dragon.Metallic")
		UE_DEFINE_GAMEPLAY_TAG(Planar,          "CoM.Dragon.Planar")
		UE_DEFINE_GAMEPLAY_TAG(Red,             "CoM.Dragon.Red")
		UE_DEFINE_GAMEPLAY_TAG(Blue,            "CoM.Dragon.Blue")
		UE_DEFINE_GAMEPLAY_TAG(Green,           "CoM.Dragon.Green")
		UE_DEFINE_GAMEPLAY_TAG(Black,           "CoM.Dragon.Black")
		UE_DEFINE_GAMEPLAY_TAG(White,           "CoM.Dragon.White")
		UE_DEFINE_GAMEPLAY_TAG(Purple,          "CoM.Dragon.Purple")
		UE_DEFINE_GAMEPLAY_TAG(Orange,          "CoM.Dragon.Orange")
		UE_DEFINE_GAMEPLAY_TAG(Brown,           "CoM.Dragon.Brown")
		UE_DEFINE_GAMEPLAY_TAG(Gold,            "CoM.Dragon.Gold")
		UE_DEFINE_GAMEPLAY_TAG(Silver_Dragon,   "CoM.Dragon.Silver")
		UE_DEFINE_GAMEPLAY_TAG(Bronze,          "CoM.Dragon.Bronze")
		UE_DEFINE_GAMEPLAY_TAG(Copper,          "CoM.Dragon.Copper")
		UE_DEFINE_GAMEPLAY_TAG(Brass,           "CoM.Dragon.Brass")
		UE_DEFINE_GAMEPLAY_TAG(Platinum,        "CoM.Dragon.Platinum")
		UE_DEFINE_GAMEPLAY_TAG(Electrum,        "CoM.Dragon.Electrum")
		UE_DEFINE_GAMEPLAY_TAG(Iron_Dragon,     "CoM.Dragon.Iron")
		UE_DEFINE_GAMEPLAY_TAG(Shadow,          "CoM.Dragon.Shadow")
		UE_DEFINE_GAMEPLAY_TAG(Bone,            "CoM.Dragon.Bone")
		UE_DEFINE_GAMEPLAY_TAG(Void_Dragon,     "CoM.Dragon.Void")
		UE_DEFINE_GAMEPLAY_TAG(Obsidian_Dragon, "CoM.Dragon.Obsidian")
		UE_DEFINE_GAMEPLAY_TAG(Fungal,          "CoM.Dragon.Fungal")
		UE_DEFINE_GAMEPLAY_TAG(Amber,           "CoM.Dragon.Amber")
		UE_DEFINE_GAMEPLAY_TAG(RootWyrm,        "CoM.Dragon.RootWyrm")
		UE_DEFINE_GAMEPLAY_TAG(Bloom,           "CoM.Dragon.Bloom")
		UE_DEFINE_GAMEPLAY_TAG(Magma,           "CoM.Dragon.Magma")
		UE_DEFINE_GAMEPLAY_TAG(Ash,             "CoM.Dragon.Ash")
		UE_DEFINE_GAMEPLAY_TAG(Hellfire,        "CoM.Dragon.Hellfire")
		UE_DEFINE_GAMEPLAY_TAG(Phoenix,         "CoM.Dragon.Phoenix")
		UE_DEFINE_GAMEPLAY_TAG(Crystal_Dragon,  "CoM.Dragon.Crystal")
		UE_DEFINE_GAMEPLAY_TAG(Dream,           "CoM.Dragon.Dream")
		UE_DEFINE_GAMEPLAY_TAG(Temporal,        "CoM.Dragon.Temporal")
		UE_DEFINE_GAMEPLAY_TAG(VoidWyrm,        "CoM.Dragon.VoidWyrm")
	}

	// ── Orgs ─────────────────────────────────────────────────────────────────
	namespace Org
	{
		UE_DEFINE_GAMEPLAY_TAG(SilverConcordat,    "CoM.Org.SilverConcordat")
		UE_DEFINE_GAMEPLAY_TAG(ShadowWeb,          "CoM.Org.ShadowWeb")
		UE_DEFINE_GAMEPLAY_TAG(ArcanCollegium,     "CoM.Org.ArcaneCollegium")
		UE_DEFINE_GAMEPLAY_TAG(RadiantHost,        "CoM.Org.RadiantHost")
		UE_DEFINE_GAMEPLAY_TAG(ObsidianCabal,      "CoM.Org.ObsidianCabal")
		UE_DEFINE_GAMEPLAY_TAG(GreenWardens,       "CoM.Org.GreenWardens")
		UE_DEFINE_GAMEPLAY_TAG(PyroclastLodge,     "CoM.Org.PyroclastLodge")
		UE_DEFINE_GAMEPLAY_TAG(InscriptionMasters, "CoM.Org.InscriptionMasters")
		UE_DEFINE_GAMEPLAY_TAG(IronCompany,        "CoM.Org.IronCompany")
		UE_DEFINE_GAMEPLAY_TAG(Delvers,            "CoM.Org.Delvers")
		UE_DEFINE_GAMEPLAY_TAG(OraclesEye,         "CoM.Org.OraclesEye")
		UE_DEFINE_GAMEPLAY_TAG(MaelstromRaiders,   "CoM.Org.MaelstromRaiders")
	}

	// ── Wizards ──────────────────────────────────────────────────────────────
	namespace Wizard
	{
		UE_DEFINE_GAMEPLAY_TAG(Merlin,     "CoM.Wizard.Merlin")
		UE_DEFINE_GAMEPLAY_TAG(Raven,      "CoM.Wizard.Raven")
		UE_DEFINE_GAMEPLAY_TAG(Sharee,     "CoM.Wizard.Sharee")
		UE_DEFINE_GAMEPLAY_TAG(LoPan,      "CoM.Wizard.LoPan")
		UE_DEFINE_GAMEPLAY_TAG(Jafar,      "CoM.Wizard.Jafar")
		UE_DEFINE_GAMEPLAY_TAG(Oberic,     "CoM.Wizard.Oberic")
		UE_DEFINE_GAMEPLAY_TAG(Rjak,       "CoM.Wizard.Rjak")
		UE_DEFINE_GAMEPLAY_TAG(Sssra,      "CoM.Wizard.Sssra")
		UE_DEFINE_GAMEPLAY_TAG(Tauron,     "CoM.Wizard.Tauron")
		UE_DEFINE_GAMEPLAY_TAG(Freya,      "CoM.Wizard.Freya")
		UE_DEFINE_GAMEPLAY_TAG(Horus,      "CoM.Wizard.Horus")
		UE_DEFINE_GAMEPLAY_TAG(Ariel,      "CoM.Wizard.Ariel")
		UE_DEFINE_GAMEPLAY_TAG(Tlaloc,     "CoM.Wizard.Tlaloc")
		UE_DEFINE_GAMEPLAY_TAG(Kali,       "CoM.Wizard.Kali")
		UE_DEFINE_GAMEPLAY_TAG(Verdana,    "CoM.Wizard.Verdana")
		UE_DEFINE_GAMEPLAY_TAG(Pyraxis,    "CoM.Wizard.Pyraxis")
		UE_DEFINE_GAMEPLAY_TAG(Aethon,     "CoM.Wizard.Aethon")
		UE_DEFINE_GAMEPLAY_TAG(Umbrix,     "CoM.Wizard.Umbrix")
		UE_DEFINE_GAMEPLAY_TAG(Gaia,       "CoM.Wizard.Gaia")
		UE_DEFINE_GAMEPLAY_TAG(Mordecai,   "CoM.Wizard.Mordecai")
		UE_DEFINE_GAMEPLAY_TAG(Seraphiel,  "CoM.Wizard.Seraphiel")
		UE_DEFINE_GAMEPLAY_TAG(Korgath,    "CoM.Wizard.Korgath")
		UE_DEFINE_GAMEPLAY_TAG(Thessaly,   "CoM.Wizard.Thessaly")
		UE_DEFINE_GAMEPLAY_TAG(Vex,        "CoM.Wizard.Vex")
		UE_DEFINE_GAMEPLAY_TAG(Myrkul,     "CoM.Wizard.Myrkul")
		UE_DEFINE_GAMEPLAY_TAG(Crystallia, "CoM.Wizard.Crystallia")
		UE_DEFINE_GAMEPLAY_TAG(Custom,     "CoM.Wizard.Custom")
	}

	// ── Unit types ───────────────────────────────────────────────────────────
	namespace Unit
	{
		UE_DEFINE_GAMEPLAY_TAG(IsHero,       "CoM.Unit.IsHero")
		UE_DEFINE_GAMEPLAY_TAG(IsUndead,     "CoM.Unit.IsUndead")
		UE_DEFINE_GAMEPLAY_TAG(IsDragon,     "CoM.Unit.IsDragon")
		UE_DEFINE_GAMEPLAY_TAG(IsConstruct,  "CoM.Unit.IsConstruct")
		UE_DEFINE_GAMEPLAY_TAG(IsShip,       "CoM.Unit.IsShip")
		UE_DEFINE_GAMEPLAY_TAG(IsSummon,     "CoM.Unit.IsSummon")
		UE_DEFINE_GAMEPLAY_TAG(IsFantastic,  "CoM.Unit.IsFantastic")
		UE_DEFINE_GAMEPLAY_TAG(Role_Infantry,    "CoM.Unit.Role.Infantry")
		UE_DEFINE_GAMEPLAY_TAG(Role_Cavalry,     "CoM.Unit.Role.Cavalry")
		UE_DEFINE_GAMEPLAY_TAG(Role_Ranged,      "CoM.Unit.Role.Ranged")
		UE_DEFINE_GAMEPLAY_TAG(Role_Spellcaster, "CoM.Unit.Role.Spellcaster")
		UE_DEFINE_GAMEPLAY_TAG(Role_Support,     "CoM.Unit.Role.Support")
		UE_DEFINE_GAMEPLAY_TAG(Role_Siege,       "CoM.Unit.Role.Siege")
		UE_DEFINE_GAMEPLAY_TAG(Role_Naval,       "CoM.Unit.Role.Naval")
		UE_DEFINE_GAMEPLAY_TAG(Role_Scout,       "CoM.Unit.Role.Scout")
	}

	// ── Runes ────────────────────────────────────────────────────────────────
	namespace Rune
	{
		UE_DEFINE_GAMEPLAY_TAG(Warding,       "CoM.Rune.Warding")
		UE_DEFINE_GAMEPLAY_TAG(Flame,         "CoM.Rune.Flame")
		UE_DEFINE_GAMEPLAY_TAG(Mending,       "CoM.Rune.Mending")
		UE_DEFINE_GAMEPLAY_TAG(Seeing,        "CoM.Rune.Seeing")
		UE_DEFINE_GAMEPLAY_TAG(Wealth,        "CoM.Rune.Wealth")
		UE_DEFINE_GAMEPLAY_TAG(Binding,       "CoM.Rune.Binding")
		UE_DEFINE_GAMEPLAY_TAG(Swiftness,     "CoM.Rune.Swiftness")
		UE_DEFINE_GAMEPLAY_TAG(Fortification, "CoM.Rune.Fortification")
		UE_DEFINE_GAMEPLAY_TAG(VoidRune,      "CoM.Rune.Void")
	}

	// ── Conditions ───────────────────────────────────────────────────────────
	namespace Condition
	{
		UE_DEFINE_GAMEPLAY_TAG(HeroAlive,                   "CoM.Condition.HeroAlive")
		UE_DEFINE_GAMEPLAY_TAG(ArtifactInPossession,        "CoM.Condition.ArtifactInPossession")
		UE_DEFINE_GAMEPLAY_TAG(DragonAllied,                "CoM.Condition.DragonAllied")
		UE_DEFINE_GAMEPLAY_TAG(OrgMember,                   "CoM.Condition.OrgMember")
		UE_DEFINE_GAMEPLAY_TAG(PlaneConquered,              "CoM.Condition.PlaneConquered")
		UE_DEFINE_GAMEPLAY_TAG(LeyNexusControlled,          "CoM.Condition.LeyNexusControlled")
		UE_DEFINE_GAMEPLAY_TAG(SpellOfMasteryResearched,    "CoM.Condition.SpellOfMasteryResearched")
UE_DEFINE_GAMEPLAY_TAG(TAG_Wizard_DjinnSorcerer, "CoM.Wizard.DjinnSorcerer");
UE_DEFINE_GAMEPLAY_TAG(TAG_Wizard_EfreetiLord, "CoM.Wizard.EfreetiLord");
	}

} // namespace CoMTags
