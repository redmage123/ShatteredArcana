// Copyright Mythforge Studios. All Rights Reserved.
// CoMPlayerState.cpp — COM-027
#include "Wizards/CoMPlayerState.h"

ACoMPlayerState::ACoMPlayerState()
{
	InitSpellbookAllocation();
}

void ACoMPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ACoMPlayerState::InitSpellbookAllocation()
{
	// One zero-entry per spell realm so indexing by (int32)ECoMSpellRealm is always valid.
	SpellbookAllocation.SetNumZeroed(CoM::NUM_SPELL_REALMS);
}

// ─── Accessors ────────────────────────────────────────────────────────────────

bool ACoMPlayerState::HasRetort(FGameplayTag RetortTag) const
{
	return Retorts.HasTag(RetortTag);
}

int32 ACoMPlayerState::GetSpellbookCountForRealm(ECoMSpellRealm Realm) const
{
	const int32 Idx = static_cast<int32>(Realm);
	if (!SpellbookAllocation.IsValidIndex(Idx))
	{
		return 0;
	}
	return SpellbookAllocation[Idx];
}

bool ACoMPlayerState::HasResearchedSpell(FName SpellID) const
{
	return ResearchedSpells.Contains(SpellID);
}

int32 ACoMPlayerState::ModifyGold(int32 Delta)
{
	Gold = FMath::Max(0, Gold + Delta);
	return Gold;
}

int32 ACoMPlayerState::ModifyMana(int32 Delta)
{
	Mana = FMath::Max(0, Mana + Delta);
	return Mana;
}
