#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMFixedPoint.h"
#include "CoMLeyLineDataAsset.generated.h"

/**
 * Data asset defining a ley line type (magical energy conduit connecting power nodes)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMLeyLineDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMLeyLineDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine")
	FName LeyLineTypeID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Magic")
	ECoMSpellRealm Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Bonuses")
	int32 ManaBonusPerNode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Bonuses")
	int32 ManaBonusNexus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Bonuses")
	FFixed64 SpellPowerBonus; // 0.1 = +10%

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Bonuses")
	FFixed64 ResearchBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Vulnerability")
	bool bCanBeCorrupted;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Vulnerability")
	bool bCanBeSevered;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Placement")
	TArray<ECoMPlane> OccursOnPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LeyLine|Presentation")
	TSoftObjectPtr<UTexture2D> LineTexture;
};
