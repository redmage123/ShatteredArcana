// Copyright Mythforge Studios. All Rights Reserved.
// CoMSiteIconActor.h -- Billboard icon for explorable sites and mana nodes
// on the overworld.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMSiteIconActor.generated.h"

struct FCoMSite;
class UBillboardComponent;
class UTextRenderComponent;

/**
 * ACoMSiteIconActor
 *
 * Renders an explorable site (lair, ruin, tower) or a mana node as a
 * billboard sprite + label on the overworld.
 *
 * Two configuration paths:
 *   InitFromSite()    -- registered FCoMSite (lairs, ruins, towers)
 *   InitFromManaNode()-- guarded mana node (tile.bHasManaNode + NodeRealm)
 */
UCLASS(NotBlueprintable)
class COMRENDERING_API ACoMSiteIconActor : public AActor
{
	GENERATED_BODY()

public:
	ACoMSiteIconActor();

	void InitFromSite(const FCoMSite& Site);
	void InitFromManaNode(FIntPoint Position, ECoMSpellRealm Realm,
	                      int32 OwnerWizardIndex, bool bGuarded);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Site")
	int32 SiteID = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Site")
	bool bIsManaNode = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "Site")
	TObjectPtr<UBillboardComponent> IconSprite;

	UPROPERTY(VisibleAnywhere, Category = "Site")
	TObjectPtr<UTextRenderComponent> Label;
};
