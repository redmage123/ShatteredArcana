#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMWeatherDataAsset.generated.h"

/**
 * Data asset defining a weather type with its effects on gameplay and visuals
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMWeatherDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMWeatherDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	ECoMWeatherType WeatherType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Plane")
	TArray<ECoMPlane> OccursOnPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Movement")
	FFixed64 MoveCostModifier; // 1.0 = normal, 1.5 = slowed

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Combat")
	FFixed64 CombatAttackModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Combat")
	FFixed64 CombatDefenseModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Economy")
	FFixed64 ManaIncomeModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Economy")
	FFixed64 FoodModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Vision")
	FFixed64 VisionRangeModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Magic")
	bool bBlocksMagic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Effects|Movement")
	bool bBlocksFlying;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Magic")
	ECoMSpellRealm WeatherAlignment; // Which magic it favors

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Duration")
	int32 DurationTurnsMin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Duration")
	int32 DurationTurnsMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Presentation")
	TSoftObjectPtr<UTexture2D> WeatherIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather|Presentation")
	TSoftObjectPtr<UObject> WeatherVFX;
};
