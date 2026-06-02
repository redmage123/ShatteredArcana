// Copyright Shattered Arcana. All Rights Reserved.

#include "CoMScenarioDatabase.h"

#define LOCTEXT_NAMESPACE "CoMScenario"

namespace
{
	TArray<FCoMScenarioDef>& MutableTable()
	{
		static TArray<FCoMScenarioDef> Table;
		return Table;
	}

	void Register()
	{
		TArray<FCoMScenarioDef>& T = MutableTable();
		if (T.Num() > 0) return;

		// ── 1. Lone Mage vs Three Empires ──────────────────────────────────
		// Slot 0 is the player: 1 city, no bonuses. The three AI wizards each
		// start with 3 extra cities and are tuned aggressive. The classic
		// "survive the dark tide" setup.
		{
			FCoMScenarioDef S;
			S.ScenarioID  = FName(TEXT("lone_mage"));
			S.DisplayName = LOCTEXT("LoneMageName", "Lone Mage vs Three Empires");
			S.Synopsis    = LOCTEXT("LoneMageSyn",
				"Three rival wizards have carved up the realm. You begin with a "
				"single city and no allies — outpace, outwit, or outlast them all.");
			S.NumWizards  = 4;
			S.Seed        = 1701;
			S.MaxTurns    = 400;
			for (int32 W = 1; W < 4; ++W)
			{
				FCoMScenarioWizardBonus B;
				B.WizardIndex = W;
				B.ExtraCities = 3;
				B.ExtraMana   = 200;
				B.AggressionMultiplier = 1.5f;
				S.WizardBonuses.Add(B);
			}
			T.Add(S);
		}

		// ── 2. Race for Mastery ───────────────────────────────────────────
		// 4 wizards, very short cap, only InstantMagicVictory counts. Whoever
		// finishes Spell of Mastery first wins. Pure research race.
		{
			FCoMScenarioDef S;
			S.ScenarioID  = FName(TEXT("race_for_mastery"));
			S.DisplayName = LOCTEXT("RaceMasteryName", "Race for Mastery");
			S.Synopsis    = LOCTEXT("RaceMasterySyn",
				"The cosmos is unravelling. Be the first to research and cast "
				"the Spell of Mastery — conquest victories do not count here.");
			S.NumWizards  = 4;
			S.Seed        = 9001;
			S.MaxTurns    = 200;
			for (int32 W = 0; W < 4; ++W)
			{
				FCoMScenarioWizardBonus B;
				B.WizardIndex = W;
				B.ExtraMana   = 400;
				B.ExtraGold   = 200;
				S.WizardBonuses.Add(B);
			}
			S.RequiredVictoryType = ECoMVictoryType::Magical;
			T.Add(S);
		}

		// ── 3. Domination Duel ────────────────────────────────────────────
		// Head-to-head 1v1 over a long horizon. Big resources to encourage
		// epic city stacks. Both wizards play scholar/builder by default;
		// the player can swing into aggressor through real choices.
		{
			FCoMScenarioDef S;
			S.ScenarioID  = FName(TEXT("domination_duel"));
			S.DisplayName = LOCTEXT("DomDuelName", "Domination Duel");
			S.Synopsis    = LOCTEXT("DomDuelSyn",
				"A single rival stands between you and total dominion. "
				"No alliances. No truce. Conquer or be conquered.");
			S.NumWizards  = 2;
			S.Seed        = 4242;
			S.MaxTurns    = 500;
			for (int32 W = 0; W < 2; ++W)
			{
				FCoMScenarioWizardBonus B;
				B.WizardIndex = W;
				B.ExtraCities = 1;
				B.ExtraMana   = 300;
				B.ExtraGold   = 500;
				B.AggressionMultiplier = 1.25f;
				S.WizardBonuses.Add(B);
			}
			S.RequiredVictoryType = ECoMVictoryType::Domination;
			T.Add(S);
		}

		// ── 4. The Great Wasting ──────────────────────────────────────────
		// Armageddon-flavoured: all wizards know the Armageddon_Clock global
		// at start. Whoever lets it tick down longest while still standing
		// wins. Short cap because the world is dying.
		{
			FCoMScenarioDef S;
			S.ScenarioID  = FName(TEXT("great_wasting"));
			S.DisplayName = LOCTEXT("GreatWastingName", "The Great Wasting");
			S.Synopsis    = LOCTEXT("GreatWastingSyn",
				"An ancient countdown has begun. Wizards know the doom is "
				"coming and have prepared. Survive the chaos and crown "
				"yourself amid the ruins.");
			S.NumWizards  = 4;
			S.Seed        = 13;
			S.MaxTurns    = 250;
			for (int32 W = 0; W < 4; ++W)
			{
				FCoMScenarioWizardBonus B;
				B.WizardIndex = W;
				B.ExtraMana   = 250;
				B.StartingGlobal = FName(TEXT("Armageddon_Clock"));
				B.AggressionMultiplier = 1.3f;
				S.WizardBonuses.Add(B);
			}
			T.Add(S);
		}

		// ── 5. Allied Foes ────────────────────────────────────────────────
		// The three AI wizards effectively start as a coalition. Player has
		// to break the alliance via diplomacy or out-tempo them.
		{
			FCoMScenarioDef S;
			S.ScenarioID  = FName(TEXT("allied_foes"));
			S.DisplayName = LOCTEXT("AlliedFoesName", "Allied Foes");
			S.Synopsis    = LOCTEXT("AlliedFoesSyn",
				"Three rivals signed a pact against you long ago. Buy them off, "
				"turn them against each other, or break the alliance with steel.");
			S.NumWizards  = 4;
			S.Seed        = 7777;
			S.MaxTurns    = 450;
			// Player keeps default. AIs all start friendly to one another.
			for (int32 W = 1; W < 4; ++W)
			{
				FCoMScenarioWizardBonus B;
				B.WizardIndex = W;
				B.ExtraCities = 2;
				B.ExtraMana   = 150;
				B.AggressionMultiplier = 1.2f;
				S.WizardBonuses.Add(B);
			}
			T.Add(S);
		}
	}
}

const TArray<FCoMScenarioDef>& CoMScenarioDatabase::GetAll()
{
	Register();
	return MutableTable();
}

const FCoMScenarioDef* CoMScenarioDatabase::Find(FName ScenarioID)
{
	Register();
	for (const FCoMScenarioDef& D : MutableTable())
	{
		if (D.ScenarioID == ScenarioID) return &D;
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
