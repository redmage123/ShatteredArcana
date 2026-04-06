// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMHeroDataAsset

#include "Data/CoMHeroDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMHeroDataAsset::UCoMHeroDataAsset()
{
}

FPrimaryAssetId UCoMHeroDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMHero"), GetFName());
}
