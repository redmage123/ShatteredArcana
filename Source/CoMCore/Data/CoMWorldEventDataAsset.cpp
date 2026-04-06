// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMWorldEventDataAsset

#include "Data/CoMWorldEventDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMWorldEventDataAsset::UCoMWorldEventDataAsset()
{
}

FPrimaryAssetId UCoMWorldEventDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMWorldEvent"), GetFName());
}
