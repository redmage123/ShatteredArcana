// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMExplorableSiteDataAsset

#include "Data/CoMExplorableSiteDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMExplorableSiteDataAsset::UCoMExplorableSiteDataAsset()
{
}

FPrimaryAssetId UCoMExplorableSiteDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMExplorableSite"), GetFName());
}
