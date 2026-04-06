// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMSpellDataAsset

#include "Data/CoMSpellDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMSpellDataAsset::UCoMSpellDataAsset()
{
}

FPrimaryAssetId UCoMSpellDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMSpell"), GetFName());
}
