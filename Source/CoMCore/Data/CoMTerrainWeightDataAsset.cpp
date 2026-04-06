// Copyright Mythforge Studios. All Rights Reserved.
// COM-S3-T2: Per-Plane Terrain Distribution DataAssets

#include "Data/CoMTerrainWeightDataAsset.h"

FPrimaryAssetId UCoMTerrainWeightDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMTerrainWeight"), GetFName());
}
