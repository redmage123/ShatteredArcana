// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMWeatherDataAsset

#include "Data/CoMWeatherDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMWeatherDataAsset::UCoMWeatherDataAsset()
{
}

FPrimaryAssetId UCoMWeatherDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMWeather"), GetFName());
}
