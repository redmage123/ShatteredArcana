#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMOrganizationDataAsset.generated.h"

// Forward declarations
class UCoMSpellDataAsset;

/**
 * Data asset defining an organization (faction) with membership benefits and missions
 */
UCLASS(BlueprintType)
class COMCORE_API UCoMOrganizationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
	UCoMOrganizationDataAsset();
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization")
	FName OrgID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Classification")
	ECoMOrgType OrgType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Territory")
	ECoMPlane PrimaryPlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Territory")
	bool bHasUnderdarkPresence;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Missions")
	TArray<ECoMAgentMission> AvailableMissions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Membership")
	int32 MembershipCost; // Gold/mana

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Membership")
	int32 MembershipRepRequired;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Benefits")
	TArray<FCoMStatModifier> MemberBonuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Benefits")
	TArray<TSoftObjectPtr<UCoMSpellDataAsset>> MemberSpells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Diplomacy")
	FText PrimaryAgenda;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Diplomacy")
	TArray<FName> RivalOrgIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Diplomacy")
	TArray<FName> AlliedOrgIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Organization|Presentation")
	TSoftObjectPtr<UTexture2D> Emblem;
};
