// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMRuneDataAsset

#include "Data/CoMRuneDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMRuneDataAsset::UCoMRuneDataAsset()
{
}

FPrimaryAssetId UCoMRuneDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMRune"), GetFName());
}
