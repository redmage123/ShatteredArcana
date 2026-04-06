// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMWarbandDataAsset

#include "Data/CoMWarbandDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMWarbandDataAsset::UCoMWarbandDataAsset()
{
}

FPrimaryAssetId UCoMWarbandDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMWarband"), GetFName());
}
