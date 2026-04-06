// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMWizardDataAsset

#include "Data/CoMWizardDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMWizardDataAsset::UCoMWizardDataAsset()
{
}

FPrimaryAssetId UCoMWizardDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMWizard"), GetFName());
}
