// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityIconActor.h -- Billboard sprite representing a city on the overworld.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCityIconActor.generated.h"

struct FCoMCityData;
class UBillboardComponent;
class UTextRenderComponent;

/**
 * ACoMCityIconActor
 *
 * Placed above the tile grid at a city's world location. Renders as a billboard
 * sprite (always facing camera) with a text label showing the city name and
 * population.
 *
 * The sprite is colour-coded by the owning wizard's colour. A simple shield/
 * tower icon is used as the default until full city art assets are available.
 */
UCLASS(NotBlueprintable)
class COMRENDERING_API ACoMCityIconActor : public AActor
{
	GENERATED_BODY()

public:
	ACoMCityIconActor();

	/**
	 * Configure this icon from city data.
	 * Sets position, name label, population text, and owner colour.
	 */
	void InitFromCityData(const FCoMCityData& CityData);

	/** The city ID this icon represents. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "City")
	int32 CityID = -1;

	/** Owning wizard index. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "City")
	int32 OwnerWizardIndex = -1;

private:
	UPROPERTY(VisibleAnywhere, Category = "City")
	TObjectPtr<UBillboardComponent> IconSprite;

	UPROPERTY(VisibleAnywhere, Category = "City")
	TObjectPtr<UTextRenderComponent> NameLabel;

	UPROPERTY(VisibleAnywhere, Category = "City")
	TObjectPtr<UTextRenderComponent> PopLabel;

	/** Get a colour for a wizard index (simple palette). */
	static FColor GetWizardColor(int32 WizardIndex);
};
