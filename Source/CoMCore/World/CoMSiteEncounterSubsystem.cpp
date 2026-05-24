// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMSiteEncounterSubsystem.h"

#include "CoMCore/Framework/CoMGameState.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Wizards/CoMPlayerState.h"
#include "CoMCore/World/CoMWorldMapSubsystem.h"
#include "Engine/World.h"

void UCoMSiteEncounterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoMSiteEncounterSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// =============================================================================
// Power evaluation
// =============================================================================

float UCoMSiteEncounterSubsystem::ComputeArmyPower(int32 ArmyID) const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return 0.0f;

	UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!Units) return 0.0f;

	const FCoMArmyGroup* Army = Units->GetArmy(ArmyID);
	if (!Army) return 0.0f;

	float Power = 0.0f;
	for (int32 UID : Army->UnitIDs)
	{
		const FCoMUnitInstance* Unit = Units->GetUnit(UID);
		if (!Unit) continue;
		float P = static_cast<float>(FMath::Max(1, Unit->Level)) * static_cast<float>(FMath::Max(1, Unit->CurrentHP));
		if (Unit->bIsHero) P *= 2.0f;
		Power += P;
	}
	return Power;
}

bool UCoMSiteEncounterSubsystem::ResolveAutoCombat(int32 ArmyID, int32 GuardPower)
{
	const float Ours   = ComputeArmyPower(ArmyID);
	const float Theirs = static_cast<float>(FMath::Max(1, GuardPower));
	const float Ratio  = Ours / Theirs;

	UGameInstance* GI = GetGameInstance();
	UCoMUnitSubsystem* Units = GI ? GI->GetSubsystem<UCoMUnitSubsystem>() : nullptr;

	// If our army has no realistic chance (< 40% of guard power), the AI
	// should bail rather than throw lives away. From the caller's POV this
	// counts as a "loss" so the cooldown fires and the army won't retry.
	const bool bNoChance = (Ratio < 0.40f);
	const bool bWin      = !bNoChance && (Ratio >= 1.05f);

	if (Units && !bNoChance)
	{
		// Damage scales with the guard's relative strength, but capped per unit
		// so even a lost encounter doesn't instantly wipe a stack. Starter
		// armies should be able to flee with HP to spare.
		const FCoMArmyGroup* Army = Units->GetArmy(ArmyID);
		if (Army)
		{
			const TArray<int32> UnitsCopy = Army->UnitIDs;
			const float DamageFactor = bWin
				? FMath::Clamp(Theirs / FMath::Max(Ours, 1.0f), 0.05f, 0.20f)
				: FMath::Clamp(Theirs / FMath::Max(Ours, 1.0f), 0.20f, 0.40f);

			for (int32 UID : UnitsCopy)
			{
				const FCoMUnitInstance* U = Units->GetUnit(UID);
				if (!U) continue;
				// Cap damage at MaxHP - 1 so a unit never dies in a single
				// encounter; multiple lost encounters can still wear it down.
				int32 Damage = FMath::RoundToInt(U->MaxHP * DamageFactor);
				Damage = FMath::Clamp(Damage, 1, FMath::Max(1, U->CurrentHP - 1));
				Units->ApplyDamage(UID, Damage);
			}
		}
	}

	return bWin;
}

// =============================================================================
// Reward payout
// =============================================================================

namespace
{
	/** Find the player state for a given wizard index. */
	ACoMPlayerState* FindWizardPlayerState(UWorld* World, int32 WizardIndex)
	{
		if (!World) return nullptr;
		ACoMGameState* GS = Cast<ACoMGameState>(World->GetGameState());
		if (!GS) return nullptr;
		return GS->GetWizardByIndex(WizardIndex);
	}
}

// =============================================================================
// Encounter detection
// =============================================================================

bool UCoMSiteEncounterSubsystem::TryResolveEncounterForArmy(int32 ArmyID)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return false;

	UCoMUnitSubsystem*     Units = GI->GetSubsystem<UCoMUnitSubsystem>();
	UCoMWorldMapSubsystem* Map   = GI->GetSubsystem<UCoMWorldMapSubsystem>();
	UCoMMagicSubsystem*    Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
	if (!Units || !Map) return false;

	const FCoMArmyGroup* Army = Units->GetArmy(ArmyID);
	if (!Army || Army->UnitIDs.Num() == 0) return false;
	if (Army->bInCombat) return false;
	if (Army->EncounterCooldown > 0) return false;

	// Skip encounters when the army contains a settler — settlers can't fight
	// and walking them into site guards is suicide. The AI's other armies do
	// the dungeon-crawling once cities are established.
	for (int32 UID : Army->UnitIDs)
	{
		const FCoMUnitInstance* U = Units->GetUnit(UID);
		if (U && U->bIsSettler) { return false; }
	}

	const FCoMTileData* Tile = Map->GetTile(Army->Plane, Army->Layer, Army->Position.X, Army->Position.Y);
	if (!Tile) return false;

	const int32 WizardIndex = Army->OwnerWizardIndex;

	// ── Site encounter ────────────────────────────────────────────────
	if (Tile->SiteID >= 0)
	{
		const FCoMSite* Site = Map->GetSite(Tile->SiteID);
		if (Site && !Site->bCleared)
		{
			const bool bWin = ResolveAutoCombat(ArmyID, Site->GuardPower);
			if (bWin)
			{
				FCoMSite* Mut = Map->GetSiteMutable(Tile->SiteID);
				if (Mut)
				{
					Mut->bCleared        = true;
					Mut->ClearedByWizard = WizardIndex;
				}

				// Pay rewards.
				if (ACoMPlayerState* PS = FindWizardPlayerState(GetWorld(), WizardIndex))
				{
					if (Site->GoldReward > 0) { PS->ModifyGold(Site->GoldReward); }
					if (Site->ManaReward > 0) { PS->ModifyMana(Site->ManaReward); }
				}
				if (Magic && !Site->SpellReward.IsNone())
				{
					FCoMWizardMagicState& State = Magic->GetWizardMagic(WizardIndex);
					State.KnownSpells.AddUnique(Site->SpellReward);
				}

				UE_LOG(LogTemp, Log,
					TEXT("[SiteEncounter] Wizard %d cleared site %d (%s) at (%d,%d): +%d gold, +%d mana"),
					WizardIndex, Site->SiteID,
					*StaticEnum<ECoMSiteType>()->GetDisplayNameTextByValue((int64)Site->Type).ToString(),
					Site->Position.X, Site->Position.Y, Site->GoldReward, Site->ManaReward);

				OnSiteCleared.Broadcast(Site->SiteID, WizardIndex, Site->GoldReward, Site->ManaReward);
				return true;
			}
			else
			{
				// Loss: skip future site encounters for this army for 2 turns
				// so a damaged army can heal or relocate instead of fighting
				// the same guards to death.
				if (FCoMArmyGroup* Mut = Units->GetArmyMutable(ArmyID))
				{
					Mut->EncounterCooldown = 2;
				}
				UE_LOG(LogTemp, Log,
					TEXT("[SiteEncounter] Wizard %d failed to clear site %d at (%d,%d) — army repulsed"),
					WizardIndex, Site->SiteID, Site->Position.X, Site->Position.Y);
				return true;
			}
		}
	}

	// ── Guarded mana node ────────────────────────────────────────────
	if (Tile->bHasManaNode && Tile->bNodeGuarded)
	{
		// Stock guard power scales with node output.
		const int32 GuardPower = 60 + Tile->NodeManaOutput * 8;
		const bool bWin = ResolveAutoCombat(ArmyID, GuardPower);
		if (bWin)
		{
			FCoMTileData* Mut = Map->GetTileMutable(Army->Plane, Army->Layer, Army->Position.X, Army->Position.Y);
			if (Mut) { Mut->bNodeGuarded = false; }

			UE_LOG(LogTemp, Log,
				TEXT("[SiteEncounter] Wizard %d defeated mana node guard at (%d,%d) — node now meldable"),
				WizardIndex, Army->Position.X, Army->Position.Y);

			OnNodeGuardDefeated.Broadcast(Army->Position, WizardIndex);
			return true;
		}
	}

	return false;
}

void UCoMSiteEncounterSubsystem::ProcessArmyArrivals()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UCoMUnitSubsystem* Units = GI->GetSubsystem<UCoMUnitSubsystem>();
	if (!Units) return;

	// Snapshot army IDs first — TryResolveEncounterForArmy may despawn units
	// which can rebuild army membership underneath us.
	TArray<int32> ArmyIDs;
	for (int32 i = 0; i < CoM::MAX_WIZARDS; ++i)
	{
		const TArray<const FCoMArmyGroup*> Armies = Units->GetArmiesForWizard(i);
		ArmyIDs.Reserve(ArmyIDs.Num() + Armies.Num());
		for (const FCoMArmyGroup* A : Armies)
		{
			if (A) { ArmyIDs.Add(A->ArmyGroupID); }
		}
	}

	for (int32 AID : ArmyIDs)
	{
		TryResolveEncounterForArmy(AID);
	}
}
