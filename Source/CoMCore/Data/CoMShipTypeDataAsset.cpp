// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMShipTypeDataAsset

#include "Data/CoMShipTypeDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMShipTypeDataAsset::UCoMShipTypeDataAsset()
{
}

FPrimaryAssetId UCoMShipTypeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMShipType"), GetFName());
}
