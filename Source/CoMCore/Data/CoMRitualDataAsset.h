#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMRitualDataAsset.generated.h"

// Forward declarations
class UCoMSpellDataAsset;

/**
 * Data asset defining a ritual (multi-caster spell requiring preparation and execution)
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMRitualDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMRitualDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual")
	FName RitualID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Classification")
	ECoMRitualType RitualType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Requirements")
	ECoMSpellRealm RequiredRealm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Casters")
	int32 MinCasters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Casters")
	int32 MaxCasters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Cost")
	int32 BaseManaCostPerCaster;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Timing")
	int32 TurnsToPrepare;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Timing")
	int32 TurnsToExecute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Effects")
	TArray<FCoMStatModifier> EffectModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Success")
	FFixed64 SuccessChanceBase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Area")
	bool bGlobalEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Area")
	int32 EffectRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Requirements")
	bool bRequiresLeyLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Requirements")
	bool bRequiresNexus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Synergy")
	TArray<TSoftObjectPtr<UCoMSpellDataAsset>> BoostedBy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ritual|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
