// Copyright Mythforge Studios. All Rights Reserved.
// CoMGameState.cpp — COM-027
#include "Framework/CoMGameState.h"
#include "Wizards/CoMPlayerState.h"

ACoMGameState::ACoMGameState()
{
	// Pre-allocate the indexed roster so every wizard slot is null-safe from the start.
	WizardStates.SetNum(CoM::MAX_WIZARDS);

	// Aurelith is always visible — every game starts there.
	DiscoveredPlanes.Add(ECoMPlane::Aurelith);
}

void ACoMGameState::BeginPlay()
{
	Super::BeginPlay();
}

// ─── PlayerState registration ─────────────────────────────────────────────────

void ACoMGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (ACoMPlayerState* WS = Cast<ACoMPlayerState>(PlayerState))
	{
		const int32 Idx = WS->WizardIndex;
		if (WizardStates.IsValidIndex(Idx))
		{
			WizardStates[Idx] = WS;
		}
	}
}

void ACoMGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	if (ACoMPlayerState* WS = Cast<ACoMPlayerState>(PlayerState))
	{
		const int32 Idx = WS->WizardIndex;
		if (WizardStates.IsValidIndex(Idx))
		{
			WizardStates[Idx] = nullptr;
		}
	}
}

// ─── Accessors ────────────────────────────────────────────────────────────────

ACoMPlayerState* ACoMGameState::GetWizardByIndex(int32 WizardIndex) const
{
	if (!WizardStates.IsValidIndex(WizardIndex))
	{
		return nullptr;
	}
	return WizardStates[WizardIndex].Get();
}

bool ACoMGameState::GetDiplomacyRelation(int32 WizardA, int32 WizardB,
                                          FCoMDiplomacyRelation& OutRelation) const
{
	// Normalise so A < B — relations are stored once per ordered pair.
	if (WizardA > WizardB)
	{
		Swap(WizardA, WizardB);
	}

	for (const FCoMDiplomacyRelation& Rel : DiplomacyMatrix)
	{
		if (Rel.WizardA == WizardA && Rel.WizardB == WizardB)
		{
			OutRelation = Rel;
			return true;
		}
	}
	return false;
}

// ─── World Time ───────────────────────────────────────────────────────────────

void ACoMGameState::TickWorldTime()
{
	++SeasonTurn;
	if (SeasonTurn > CoM::DEFAULT_TURNS_PER_SEASON)
	{
		SeasonTurn = 1;
		++WorldSeasonIndex;
		if (WorldSeasonIndex >= CoM::NUM_SEASONS)
		{
			WorldSeasonIndex = 0;
			++WorldYear;
		}
	}
}
