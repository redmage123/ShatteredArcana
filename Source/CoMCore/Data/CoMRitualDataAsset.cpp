// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMRitualDataAsset

#include "Data/CoMRitualDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMRitualDataAsset::UCoMRitualDataAsset()
{
}

FPrimaryAssetId UCoMRitualDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMRitual"), GetFName());
}
