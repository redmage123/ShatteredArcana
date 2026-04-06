// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMRetortDataAsset

#include "Data/CoMRetortDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMRetortDataAsset::UCoMRetortDataAsset()
{
}

FPrimaryAssetId UCoMRetortDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMRetort"), GetFName());
}
