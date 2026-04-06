// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMLeyLineDataAsset

#include "Data/CoMLeyLineDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMLeyLineDataAsset::UCoMLeyLineDataAsset()
{
}

FPrimaryAssetId UCoMLeyLineDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMLeyLine"), GetFName());
}
