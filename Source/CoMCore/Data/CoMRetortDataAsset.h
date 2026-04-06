#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMRetortDataAsset.generated.h"

/**
 * Data asset defining a wizard retort (special ability/modifier for wizard creation)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMRetortDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMRetortDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort")
	FName RetortID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Cost")
	int32 SpellBookCost; // Number of spell books consumed

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Effects")
	TArray<FCoMStatModifier> WizardBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Effects")
	TArray<FCoMStatModifier> UnitBonuses; // Applied to all units

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Effects")
	TArray<FCoMStatModifier> CityBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Requirements")
	TArray<ECoMSpellRealm> RequiredRealms;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Requirements")
	TArray<FName> IncompatibleRetortIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Special")
	bool bUnlockSpecialMechanic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Special")
	FText MechanicDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retort|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
