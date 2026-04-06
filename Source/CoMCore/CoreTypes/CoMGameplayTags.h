// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameplayTags.h — All Gameplay Tag declarations for Shattered Arcana
// Sourced from game-design/master-plan.md
// All tags must also be registered in DefaultGameplayTags.ini
// Platform-agnostic: no OS/platform-specific headers

#pragma once
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ─────────────────────────────────────────────────────────────────────────────
// USAGE
// ─────────────────────────────────────────────────────────────────────────────
//
// 1. In CoMGameplayTags.cpp, define each tag with UE_DEFINE_GAMEPLAY_TAG.
// 2. In DefaultGameplayTags.ini, list every tag under [/Script/GameplayTags.GameplayTagsList].
// 3. Reference tags via CoMTags::Race::Aurelith::HighMen etc.
//
// Naming convention: CoM.<Category>.<SubCategory>.<Leaf>
// ─────────────────────────────────────────────────────────────────────────────

namespace CoMTags
{
	// ─────────────────────────────────────────────────────────────────────────
	// PLANES
	// ─────────────────────────────────────────────────────────────────────────

	namespace Plane
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aurelith);    // CoM.Plane.Aurelith
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noctharion);  // CoM.Plane.Noctharion
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Verdantis);   // CoM.Plane.Verdantis
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Infernyx);    // CoM.Plane.Infernyx
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aethermist);  // CoM.Plane.Aethermist
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Abyssal);     // CoM.Plane.Abyssal
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ethereal);    // CoM.Plane.Ethereal
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Feywild);     // CoM.Plane.Feywild — ever-shifting fey realm, Glamour school
		// NOTE: Infernal plane merged into Infernyx (2026-04-04). No CoM.Plane.Infernal tag.
	}

	// ─────────────────────────────────────────────────────────────────────────
	// SPELL REALMS
	// ─────────────────────────────────────────────────────────────────────────

	namespace Realm
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Life);     // CoM.Realm.Life
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);    // CoM.Realm.Death
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chaos);    // CoM.Realm.Chaos
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nature);   // CoM.Realm.Nature
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sorcery);  // CoM.Realm.Sorcery
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Arcane);   // CoM.Realm.Arcane
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Binding);  // CoM.Realm.Binding — soul contracts, domination
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spirit);   // CoM.Realm.Spirit  — ghosts, dreams, thought
	}

	// ─────────────────────────────────────────────────────────────────────────
	// RACES — 54+ total (24 original surface + 15 new plane surface, 10 original + 6 new Underdark, 5 aquatic)
	// ─────────────────────────────────────────────────────────────────────────

	namespace Race
	{
		// ── Aurelith Surface ──────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HighMen);    // CoM.Race.HighMen
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HighElves);  // CoM.Race.HighElves
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Halflings);  // CoM.Race.Halflings
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nomads);     // CoM.Race.Nomads
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Orcs);       // CoM.Race.Orcs
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Barbarians); // CoM.Race.Barbarians

		// ── Noctharion Surface ───────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DarkElves);  // CoM.Race.DarkElves
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dwarves);    // CoM.Race.Dwarves
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Draconians); // CoM.Race.Draconians
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Beastmen);   // CoM.Race.Beastmen
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trolls);     // CoM.Race.Trolls

		// ── Verdantis Surface ────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chithari);   // CoM.Race.Chithari
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lizardmen);  // CoM.Race.Lizardmen
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gnolls);     // CoM.Race.Gnolls
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Treants);    // CoM.Race.Treants
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Myconids);   // CoM.Race.Myconids

		// ── Infernyx Surface ─────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Salamanders); // CoM.Race.Salamanders
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ashen);       // CoM.Race.Ashen
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Obsidians);   // CoM.Race.Obsidians
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Demonkin);    // CoM.Race.Demonkin

		// ── Aethermist Surface ───────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sylphs);        // CoM.Race.Sylphs
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crystallines);  // CoM.Race.Crystallines
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Voidwalkers);   // CoM.Race.Voidwalkers
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dreamweavers);  // CoM.Race.Dreamweavers

		// ── Aurelith Underdark ────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeepDwarves); // CoM.Race.DeepDwarves
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Goblins);     // CoM.Race.Goblins

		// ── Noctharion Underdark ─────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drow);   // CoM.Race.Drow
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shades); // CoM.Race.Shades

		// ── Verdantis Underdark ──────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Myrmidons); // CoM.Race.Myrmidons
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sporekin);  // CoM.Race.Sporekin

		// ── Infernyx Underdark ───────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagmaDwarves); // CoM.Race.MagmaDwarves
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ImpSwarms);    // CoM.Race.ImpSwarms

		// ── Aethermist Underdark ─────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mindflayers); // CoM.Race.Mindflayers
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(EchoFolk);    // CoM.Race.EchoFolk

		// ── Abyssal Surface ──────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Maleficii);    // CoM.Race.Maleficii    — demon-blooded casters
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gorethians);   // CoM.Race.Gorethians   — demon-ogre brutes
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Plagueborn);   // CoM.Race.Plagueborn   — infectious swarmers
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Voidlings);    // CoM.Race.Voidlings    — formless devourers
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ashwalkers);   // CoM.Race.Ashwalkers   — demon-tainted nomads

		// ── Infernal Surface ─────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hellbound);    // CoM.Race.Hellbound    — organized devil legions
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nightbinders); // CoM.Race.Nightbinders — soul-weaving mages
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ferrovians);   // CoM.Race.Ferrovians   — iron-forged warriors
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ashguard);     // CoM.Race.Ashguard     — elite fire-immune caste
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ShadowPact);   // CoM.Race.ShadowPact   — infernal assassins

		// ── Ethereal Surface ─────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Wraiths);      // CoM.Race.Wraiths      — phasing spirit warriors
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phantoms);     // CoM.Race.Phantoms     — pure energy beings
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mistborn);     // CoM.Race.Mistborn     — mist shapeshifters
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Thoughteaters);// CoM.Race.Thoughteaters— psychic predators
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spectrels);    // CoM.Race.Spectrels    — spectral archers

		// ── Abyssal Underdark ────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChaosSpawn);   // CoM.Race.ChaosSpawn
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(VoidStalkers); // CoM.Race.VoidStalkers

		// ── Infernal Underdark ───────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SoulForgers);       // CoM.Race.SoulForgers
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HellmirrorDwarves); // CoM.Race.HellmirrorDwarves

		// ── Ethereal Underdark ───────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(EchoWraiths);       // CoM.Race.EchoWraiths
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MemoryDevourers);   // CoM.Race.MemoryDevourers

		// ── Aquatic Races ────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Merfolk);        // CoM.Race.Merfolk
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeepOnes);       // CoM.Race.DeepOnes
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tideshifters);   // CoM.Race.Tideshifters
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LavaNaga);       // CoM.Race.LavaNaga
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(EtherSwimmers);  // CoM.Race.EtherSwimmers
	}

	// ─────────────────────────────────────────────────────────────────────────
	// UNIT SKILLS
	// ─────────────────────────────────────────────────────────────────────────

	namespace Skill
	{
		// ── Movement ─────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flying);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Swimming);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Burrowing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PhaseWalk);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pathfinding);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mountaineer);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Forester);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlaneStrider);  // Free movement between adjacent planes

		// ── Combat ───────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FirstStrike);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Piercing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Regeneration);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Invisibility);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Illusionary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(NonCorporeal);  // Phase through terrain
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LifeSteal);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Poison);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Petrifying);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fear);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DoomBolt);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chaos_Touch);   // Random chaos effect on hit
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicImmunity);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ColdImmunity);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FireImmunity);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathImmunity);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PoisonImmunity);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightningImmunity);

		// ── Support ──────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Leadership);        // Adjacent army buff
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scouting);          // Extended sight range
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stealth);           // Invisible to enemies on overworld
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ArmyBreaker);       // Disrupts enemy formations
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Engineering);       // Build roads, mines, fortifications
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(RuneInscriber);     // Required for rune inscription
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpellCasting);      // Unit can cast spells in combat
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(RitualParticipant); // Can participate in rituals

		// ── Underdark ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TunnelVision);      // Reduced penalties underground
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TunnelBuilder);     // Dig through Collapsed Tunnel
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Darkvision);        // No light penalty in Underdark

		// ── Breath Weapons ───────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FireBreath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LightningBreath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PoisonBreath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AcidBreath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ColdBreath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PsychicBreath);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// SPELLS
	// ─────────────────────────────────────────────────────────────────────────

	namespace Spell
	{
		// Spell category tags (used for filtering, AI prioritization)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Summon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Enchantment);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Combat);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_City);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Global);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Terrain);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Planar);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Trade);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Ritual);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Rune);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Weather);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_LeyLine);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Category_Diplomatic);

		// Effect tags (what the spell does, for resistance/immunity lookups)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Fire);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Cold);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Lightning);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Poison);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Death);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Holy);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Chaos);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Mind);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Void);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Nature);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Healing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Buff);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Debuff);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// BUILDINGS
	// ─────────────────────────────────────────────────────────────────────────

	namespace Building
	{
		// ── Military ─────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Barracks);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stables);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ArmorersGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponsmithsGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FightersGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(WarCollege);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Walls);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Citadel);

		// ── Economy ──────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Marketplace);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TradePost);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MerchantGuild_Bld);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GrandBazaar);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sawmill);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Miners_Guild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bank);

		// ── Magic ────────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(WizardsTower);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Library);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AlchemistsLab);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SorcerersGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagesGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ManaNode_Bld);  // Nexus (ley line control)

		// ── Religion / Culture ────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shrine);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Temple);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cathedral);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parthenon);

		// ── Food / Farming ────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GranaryFarm);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FarmersMarket);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Irrigated);

		// ── Underdark-specific ────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FungalFarm);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeepMine_Bld);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TunnelNetwork);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SubterraneanHarbor);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(EchoTower);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicLightSource);

		// ── Naval ─────────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ShipwrightsGuild);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Harbor_Bld);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TradeFleet_Bld);

		// ── Underwater ───────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CoralPalace);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TidalGenerator);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PearlFarm);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PressureDome);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// RETORTS (wizard perks)
	// ─────────────────────────────────────────────────────────────────────────

	namespace Retort
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Archmage);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Channeler);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SageMaster);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Famous);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Myrran);           // Starts on Noctharion
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chaos_Mastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Life_Mastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death_Mastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nature_Mastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sorcery_Mastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DivinePower);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InfernalPower);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Warlord);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Conjurer);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(NodeMastery);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Artificer);        // Crafting focus
	}

	// ─────────────────────────────────────────────────────────────────────────
	// TERRAIN FEATURES
	// ─────────────────────────────────────────────────────────────────────────

	namespace Terrain
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsMagical);        // Enchanted/magical terrain
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HasRoad);          // Road network present
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HasRiver);         // River crossing
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsCoastal);        // Adjacent to ocean
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsDeepOcean);      // Deep water (no fishing boat)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsVolcanic);       // Active volcanic terrain
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsCorrupted);      // Any corruption type present
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(OnLeyLine);        // Tile on a ley line
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsLeyNexus);       // Ley intersection
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsUnderdark);      // Underdark layer tile
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsUnderwater);     // Underwater zone tile
	}

	// ─────────────────────────────────────────────────────────────────────────
	// RESOURCES
	// ─────────────────────────────────────────────────────────────────────────

	namespace Resource
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Iron);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Coal);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GoldOre);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gems);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Silver);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Horses);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nightshade);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mithril);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Adamantium);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Orichalcon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aetherium);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lifewood);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fireglass);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// DRAGON TYPES (for filtering / data asset lookup)
	// ─────────────────────────────────────────────────────────────────────────

	namespace Dragon
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Chromatic);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Metallic);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Planar);

		// ── Chromatic ────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blue);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Green);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Black);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(White);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Purple);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Orange);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brown);

		// ── Metallic ─────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gold);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Silver_Dragon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bronze);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Copper);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brass);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Platinum);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrum);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Iron_Dragon);

		// ── Planar ───────────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shadow);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bone);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Void_Dragon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Obsidian_Dragon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fungal);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Amber);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(RootWyrm);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bloom);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magma);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ash);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hellfire);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phoenix);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crystal_Dragon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dream);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Temporal);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(VoidWyrm);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// ORGANIZATIONS
	// ─────────────────────────────────────────────────────────────────────────

	namespace Org
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SilverConcordat);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ShadowWeb);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ArcanCollegium);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(RadiantHost);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObsidianCabal);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GreenWardens);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PyroclastLodge);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InscriptionMasters);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronCompany);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Delvers);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(OraclesEye);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaelstromRaiders);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// WIZARD IDENTITY TAGS (25+ wizards)
	// ─────────────────────────────────────────────────────────────────────────

	namespace Wizard
	{
		// ── Original 14 ──────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Merlin);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Raven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sharee);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LoPan);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Jafar);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Oberic);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rjak);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sssra);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tauron);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Freya);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Horus);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ariel);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tlaloc);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Kali);

		// ── New Wizards ───────────────────────────────────────────────────────
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Verdana);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pyraxis);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aethon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Umbrix);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gaia);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mordecai);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Seraphiel);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Korgath);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Thessaly);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vex);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Myrkul);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crystallia);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Custom); // Human player custom wizard
	}

	// ─────────────────────────────────────────────────────────────────────────
	// UNIT TYPE CLASSIFICATION
	// ─────────────────────────────────────────────────────────────────────────

	namespace Unit
	{
		// Broad type
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsHero);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsUndead);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsDragon);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsConstruct);  // Golem, siege engine
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsShip);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsSummon);     // Conjured, not trained
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsFantastic);  // Magical creature

		// Combat role
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Infantry);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Cavalry);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Ranged);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Spellcaster);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Support);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Siege);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Naval);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Role_Scout);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// RUNE CATEGORIES
	// ─────────────────────────────────────────────────────────────────────────

	namespace Rune
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Warding);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Flame);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mending);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Seeing);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Wealth);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Binding);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Swiftness);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fortification);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(VoidRune);
	}

	// ─────────────────────────────────────────────────────────────────────────
	// ACHIEVEMENT / CONDITION FLAGS (used in quest and event tracking)
	// ─────────────────────────────────────────────────────────────────────────

	namespace Condition
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeroAlive);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ArtifactInPossession);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DragonAllied);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(OrgMember);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlaneConquered);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeyNexusControlled);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpellOfMasteryResearched);
	}

} // namespace CoMTags
