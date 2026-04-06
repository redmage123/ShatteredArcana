// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMOrganizationDataAsset

#include "Data/CoMOrganizationDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMOrganizationDataAsset::UCoMOrganizationDataAsset()
{
}

FPrimaryAssetId UCoMOrganizationDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMOrganization"), GetFName());
}
