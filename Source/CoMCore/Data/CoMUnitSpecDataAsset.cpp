// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMUnitSpecDataAsset

#include "Data/CoMUnitSpecDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMUnitSpecDataAsset::UCoMUnitSpecDataAsset()
{
}

FPrimaryAssetId UCoMUnitSpecDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMUnitSpec"), GetFName());
}
