// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMPlaneDataAsset

#include "Data/CoMPlaneDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMPlaneDataAsset::UCoMPlaneDataAsset()
{
}

FPrimaryAssetId UCoMPlaneDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMPlane"), GetFName());
}
