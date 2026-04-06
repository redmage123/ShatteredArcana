#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMWorldEventDataAsset.generated.h"

/**
 * Data asset defining a world event (random event affecting global or plane-wide gameplay)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMWorldEventDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMWorldEventDataAsset();

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FName EventID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Trigger")
	int32 TriggerTurnMin; // Earliest turn this can fire

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Trigger")
	int32 TriggerTurnMax; // -1 = any time after min

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Trigger")
	FFixed64 BaseChancePerTurn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Area")
	TArray<ECoMPlane> AffectedPlanes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Area")
	bool bGlobal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Duration")
	int32 DurationTurns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Effects")
	TArray<FCoMStatModifier> GlobalEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Recurrence")
	bool bCanRecur;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Recurrence")
	int32 RecurCooldownTurns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Notification")
	FText PlayerNotification;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event|Presentation")
	TSoftObjectPtr<UTexture2D> EventIcon;
};
