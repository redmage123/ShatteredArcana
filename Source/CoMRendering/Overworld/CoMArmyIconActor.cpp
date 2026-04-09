// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyIconActor.cpp -- Army billboard icon on the overworld.

#include "Overworld/CoMArmyIconActor.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ACoMArmyIconActor::ACoMArmyIconActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Billboard sprite
	IconSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("ArmyIcon"));
	SetRootComponent(IconSprite);
	IconSprite->SetHiddenInGame(false);
	IconSprite->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));

	// Use a different engine icon as placeholder to distinguish from cities
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultIcon(
		TEXT("/Engine/EditorResources/S_Actor.S_Actor"));
	if (DefaultIcon.Succeeded())
	{
		IconSprite->SetSprite(DefaultIcon.Object);
	}

	// Unit count badge
	CountLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CountLabel"));
	CountLabel->SetupAttachment(IconSprite);
	CountLabel->SetRelativeLocation(FVector(0.0f, 0.0f, -30.0f));
	CountLabel->SetHorizontalAlignment(EHTA_Center);
	CountLabel->SetVerticalAlignment(EVRTA_TextCenter);
	CountLabel->SetWorldSize(20.0f);
	CountLabel->SetTextRenderColor(FColor::White);
}

void ACoMArmyIconActor::InitFromArmyData(const FCoMArmyGroup& ArmyData)
{
	ArmyID = ArmyData.ArmyGroupID;
	OwnerWizardIndex = ArmyData.OwnerWizardIndex;

	// Show unit count in the army stack
	const int32 UnitCount = ArmyData.UnitIDs.Num();
	CountLabel->SetText(FText::FromString(
		FString::Printf(TEXT("%d/%d"), UnitCount, CoM::MAX_ARMY_SIZE)));

	// Colour by wizard
	const FColor WizColor = GetWizardColor(OwnerWizardIndex);
	// UBillboardComponent uses material tinting; color is applied via CountLabel instead
	CountLabel->SetTextRenderColor(WizColor);
}

void ACoMArmyIconActor::UpdatePosition(FIntPoint TilePos)
{
	const float WorldX = (static_cast<float>(TilePos.X) + 0.5f) * CoM::TILE_WORLD_SIZE_CM;
	const float WorldY = (static_cast<float>(TilePos.Y) + 0.5f) * CoM::TILE_WORLD_SIZE_CM;
	SetActorLocation(FVector(WorldX, WorldY, 15.0f));
}

FColor ACoMArmyIconActor::GetWizardColor(int32 WizardIndex)
{
	// Same palette as CoMCityIconActor for consistency
	static const FColor WizardPalette[] = {
		FColor(  50, 120, 255),  // 0: Blue
		FColor( 220,  40,  40),  // 1: Red
		FColor(  40, 200,  40),  // 2: Green
		FColor( 220, 200,  40),  // 3: Yellow
		FColor( 200,  60, 200),  // 4: Purple
		FColor(  40, 200, 200),  // 5: Cyan
		FColor( 220, 140,  40),  // 6: Orange
		FColor( 200, 200, 200),  // 7: Silver
		FColor( 140,  80,  40),  // 8: Brown
		FColor( 255, 180, 200),  // 9: Pink
		FColor(  80, 200, 120),  // 10: Sea green
		FColor( 160, 160, 220),  // 11: Lavender
		FColor( 200, 100, 100),  // 12: Salmon
		FColor( 100, 100, 100),  // 13: Grey
	};

	if (WizardIndex >= 0 && WizardIndex < 14)
	{
		return WizardPalette[WizardIndex];
	}
	return FColor(180, 180, 180);
}
