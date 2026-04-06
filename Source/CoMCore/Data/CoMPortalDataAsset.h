// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMPortalDataAsset.h — Template/prototype for portal type definitions.
// Runtime portal instances use FCoMPortal in the WorldMap subsystem.
// COM-025: DataAsset types — Portal

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMPortalDataAsset.generated.h"

/**
 * Data asset defining a portal TYPE — its visual, cost, and placement rules.
 * Runtime portal instances (FCoMPortal) live in UCoMWorldMapSubsystem.
 * Portals connect two map tiles: same-plane, cross-plane, or surface<->Underdark.
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMPortalDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMPortalDataAsset();

public:
	// ── Identity ──────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FName PortalTypeID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Classification")
	ECoMPortalType PortalType = ECoMPortalType::NaturalGateway;

	// ── Traversal Rules ──────────────────────────────────────────────────

	/** Mana cost per traversal (0 = free). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Traversal")
	int32 ActivationManaCost = 0;

	/** If true, traversal works in both directions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Traversal")
	bool bBidirectional = true;

	/** If true, portal is always open; if false, requires specific weather. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Traversal")
	bool bAlwaysActive = true;

	/** Weather condition required to activate this portal (if !bAlwaysActive). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Traversal",
	          meta = (EditCondition = "!bAlwaysActive"))
	ECoMWeatherType RequiredWeather = ECoMWeatherType::None;

	/** Max army units that can traverse per turn (0 = unlimited). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Traversal")
	int32 Capacity = 0;

	// ── Shifting Portals ─────────────────────────────────────────────────

	/** If true, destination shifts periodically (Aethermist shifting gates, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Shifting")
	bool bShifting = false;

	/** Turns between destination shifts (if bShifting). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Shifting",
	          meta = (EditCondition = "bShifting"))
	int32 ShiftIntervalTurns = 0;

	// ── Placement & Discovery ─────────────────────────────────────────────

	/** Planes on which this portal type can spawn (world gen). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Placement")
	TArray<ECoMPlane> AppearOnPlanes;

	/** Layers on which this portal type can spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Placement")
	TArray<ECoMMapLayer> AppearOnLayers;

	/** Terrain types this portal can occupy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Placement")
	TArray<ECoMTerrain> ValidTerrains;

	/** If true, portal starts hidden (must be explored to reveal). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Discovery")
	bool bStartsHidden = false;

	/** If true, any wizard can build this portal type (not just natural spawns). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Construction")
	bool bPlayerConstructible = false;

	/** Mana cost to construct (if bPlayerConstructible). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Construction",
	          meta = (EditCondition = "bPlayerConstructible"))
	int32 ConstructionManaCost = 0;

	/** Gold cost to construct (if bPlayerConstructible). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Construction",
	          meta = (EditCondition = "bPlayerConstructible"))
	int32 ConstructionGoldCost = 0;

	// ── Presentation ──────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Presentation")
	TSoftObjectPtr<UTexture2D> MapIcon;

	/** Soft path to portal activation VFX (resolved by CoMRendering — keeps CoMCore clean). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Presentation")
	FSoftObjectPath ActivationVFXPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Presentation")
	FSoftObjectPath AmbientVFXPath;

	// ── Tags ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Tags")
	FGameplayTagContainer PortalTags;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("CoMPortal"), GetFName());
	}
};
