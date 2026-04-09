// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityIconActor.cpp -- City billboard icon on the overworld.

#include "Overworld/CoMCityIconActor.h"
#include "CoMCore/Economy/CoMCitySubsystem.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ACoMCityIconActor::ACoMCityIconActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Billboard sprite — visible in game, always faces camera
	IconSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("CityIcon"));
	SetRootComponent(IconSprite);
	IconSprite->SetHiddenInGame(false);
	IconSprite->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));

	// Use the default editor icon texture as a placeholder.
	// In production, this will be replaced with a proper city icon sprite.
	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultIcon(
		TEXT("/Engine/EditorResources/S_Note.S_Note"));
	if (DefaultIcon.Succeeded())
	{
		IconSprite->SetSprite(DefaultIcon.Object);
	}

	// City name text
	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(IconSprite);
	NameLabel->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetVerticalAlignment(EVRTA_TextCenter);
	NameLabel->SetWorldSize(24.0f);
	NameLabel->SetTextRenderColor(FColor::White);

	// Population text (below city name)
	PopLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PopLabel"));
	PopLabel->SetupAttachment(IconSprite);
	PopLabel->SetRelativeLocation(FVector(0.0f, 0.0f, -60.0f));
	PopLabel->SetHorizontalAlignment(EHTA_Center);
	PopLabel->SetVerticalAlignment(EVRTA_TextCenter);
	PopLabel->SetWorldSize(18.0f);
	PopLabel->SetTextRenderColor(FColor(200, 200, 200));
}

void ACoMCityIconActor::InitFromCityData(const FCoMCityData& CityData)
{
	CityID = CityData.CityID;
	OwnerWizardIndex = CityData.OwnerWizardIndex;

	// Set the city name
	const FString DisplayName = CityData.CityName.IsEmpty()
		? FString::Printf(TEXT("City %d"), CityID)
		: CityData.CityName.ToString();
	NameLabel->SetText(FText::FromString(DisplayName));

	// Population display
	PopLabel->SetText(FText::FromString(
		FString::Printf(TEXT("Pop: %d"), CityData.Population)));

	// Colour the icon by wizard ownership
	const FColor WizColor = GetWizardColor(OwnerWizardIndex);
	// UBillboardComponent uses material tinting; color is applied via NameLabel instead
	NameLabel->SetTextRenderColor(WizColor);
}

FColor ACoMCityIconActor::GetWizardColor(int32 WizardIndex)
{
	// 14-colour palette for up to MAX_WIZARDS
	static const FColor WizardPalette[] = {
		FColor(  50, 120, 255),  // 0: Blue (human player default)
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
	return FColor(180, 180, 180); // Neutral/unowned
}
