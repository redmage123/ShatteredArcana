// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMItemTemplateDataAsset

#include "Data/CoMItemTemplateDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMItemTemplateDataAsset::UCoMItemTemplateDataAsset()
{
}

FPrimaryAssetId UCoMItemTemplateDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMItem"), GetFName());
}
