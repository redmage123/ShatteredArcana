// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMTerrainDataAsset

#include "Data/CoMTerrainDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMTerrainDataAsset::UCoMTerrainDataAsset()
{
}

FPrimaryAssetId UCoMTerrainDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMTerrain"), GetFName());
}
