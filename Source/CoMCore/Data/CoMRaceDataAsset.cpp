// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMRaceDataAsset

#include "Data/CoMRaceDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMRaceDataAsset::UCoMRaceDataAsset()
{
}

FPrimaryAssetId UCoMRaceDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMRace"), GetFName());
}
