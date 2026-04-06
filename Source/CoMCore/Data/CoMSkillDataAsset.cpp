// Copyright Mythforge Studios 2026. All Rights Reserved.
// COM-025: DataAsset implementation for UCoMSkillDataAsset

#include "Data/CoMSkillDataAsset.h"
#include "UObject/ConstructorHelpers.h"

UCoMSkillDataAsset::UCoMSkillDataAsset()
{
}

FPrimaryAssetId UCoMSkillDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("CoMSkill"), GetFName());
}
