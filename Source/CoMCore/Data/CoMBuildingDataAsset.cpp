// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMBuildingDataAsset

#include "Data/CoMBuildingDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMBuildingDataAsset::UCoMBuildingDataAsset()
{
}

FPrimaryAssetId UCoMBuildingDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMBuilding"), GetFName());
}
