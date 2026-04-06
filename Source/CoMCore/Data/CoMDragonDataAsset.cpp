// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMDragonDataAsset

#include "Data/CoMDragonDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMDragonDataAsset::UCoMDragonDataAsset()
{
}

FPrimaryAssetId UCoMDragonDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMDragon"), GetFName());
}
