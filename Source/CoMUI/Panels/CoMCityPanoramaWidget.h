// Copyright Mythforge Studios. All Rights Reserved.
// CoMCityPanoramaWidget.h -- Visual city panorama showing buildings as they are constructed.
// Inspired by Master of Magic's city view.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCityPanoramaWidget.generated.h"

class UImage;
class UOverlay;
class UCanvasPanel;
class UCanvasPanelSlot;
class UTexture2D;

/**
 * UCoMCityPanoramaWidget
 *
 * Visual representation of a city as a panoramic scene.
 * Shows a plane-appropriate background with building sprites
 * overlaid in their construction or completed state.
 *
 * Buildings are positioned in a layered cityscape:
 *   - Background: sky + terrain based on plane
 *   - Back row: large civic buildings (palace, cathedral, war college)
 *   - Middle row: standard buildings (barracks, marketplace, library, etc.)
 *   - Front row: walls, gates, small structures
 *
 * Each building shows either its "under construction" or "completed" sprite
 * based on whether it's in the city's BuildingIDs list or production queue.
 */
UCLASS()
class COMUI_API UCoMCityPanoramaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Update the panorama to reflect the given city's state.
	 * @param BuildingIDs   List of completed building FNames
	 * @param InProductionID  Building currently under construction (NAME_None if none)
	 * @param ProductionProgress  0.0-1.0 progress of current construction
	 * @param Plane         The plane this city is on (determines background)
	 * @param WallLevel     0=none, 1=wood, 2=stone, 3=iron
	 * @param Population    City population (affects visual density)
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|CityPanorama")
	void UpdatePanorama(
		const TArray<FName>& BuildingIDs,
		FName InProductionID,
		float ProductionProgress,
		ECoMPlane Plane,
		int32 WallLevel,
		int32 Population);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

public:
	/** Building position in the panorama. */
	struct FBuildingSlot
	{
		float X;       // Canvas X position (0-960)
		float Y;       // Canvas Y position (0-480)
		float Size;    // Sprite display size
		int32 Row;     // 0=back, 1=mid, 2=front
	};
	static FBuildingSlot GetSlotForBuilding(FName BuildingID);

private:
	/** Clear all building sprites from the canvas. */
	void ClearBuildings();

	/** Place a building sprite at the given canvas position. */
	void PlaceBuildingSprite(const FString& TexturePath, float X, float Y, float Size);

	/** Load a texture from the cityview art directory. */
	UTexture2D* LoadBuildingTexture(const FString& BuildingName, bool bComplete);

	/** Load the background texture for a given plane. */
	UTexture2D* LoadBackgroundTexture(ECoMPlane Plane);

	// -- Widget references -------------------------------------------------------

	UPROPERTY()
	UOverlay* RootOverlay = nullptr;

	UPROPERTY()
	UImage* BackgroundImage = nullptr;

	UPROPERTY()
	UCanvasPanel* BuildingCanvas = nullptr;

	/** Track placed building image widgets for cleanup. */
	UPROPERTY()
	TArray<UImage*> PlacedBuildingImages;
};
