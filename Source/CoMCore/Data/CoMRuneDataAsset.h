#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMRuneDataAsset.generated.h"

/**
 * Data asset defining a rune that can be inscribed on targets for magical effects
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMRuneDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMRuneDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
	FName RuneID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Magic")
	ECoMSpellRealm Realm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Target")
	ECoMRuneTarget TargetType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Cost")
	int32 ManaCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Requirements")
	int32 ResearchRequired; // Skill level to inscribe

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Effects")
	TArray<FCoMStatModifier> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Duration")
	int32 DurationTurns; // -1 = permanent

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Stacking")
	bool bStackable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Stacking")
	int32 MaxStacks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
