#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMSkillDataAsset.generated.h"

/**
 * Data asset defining a skill that can be possessed by units, heroes, or granted by items
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMSkillDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	bool bOffensive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	bool bDefensive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	bool bPassive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Type")
	bool bTriggered;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effects")
	TArray<FCoMStatModifier> StatModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Usage")
	int32 UsesPerBattle = -1; // -1 = unlimited

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Magic")
	ECoMSpellRealm Realm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Presentation")
	TSoftObjectPtr<UTexture2D> Icon;
};
