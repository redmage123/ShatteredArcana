// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMResourceNodeDataAsset

#include "Data/CoMResourceNodeDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMResourceNodeDataAsset::UCoMResourceNodeDataAsset()
{
}

FPrimaryAssetId UCoMResourceNodeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMResourceNode"), GetFName());
}
