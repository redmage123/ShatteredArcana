// Copyright Mythforge Studios. All Rights Reserved.
// CoMArmyIconActor.h -- Billboard sprite representing an army stack on the overworld.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMArmyIconActor.generated.h"

struct FCoMArmyGroup;
class UBillboardComponent;
class UTextRenderComponent;

/**
 * ACoMArmyIconActor
 *
 * Placed above the tile grid at an army's world location. Shows a billboard
 * sprite with a coloured team indicator and a unit-count badge.
 *
 * Enemy armies are only visible when the local player has current vision on
 * the tile; the renderer handles this filtering before spawning.
 */
UCLASS(NotBlueprintable)
class COMRENDERING_API ACoMArmyIconActor : public AActor
{
	GENERATED_BODY()

public:
	ACoMArmyIconActor();

	/** Configure this icon from army group data. */
	void InitFromArmyData(const FCoMArmyGroup& ArmyData);

	/** Update world position from the army's current tile coordinate. */
	void UpdatePosition(FIntPoint TilePos);

	/** The army group ID this icon represents. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Army")
	int32 ArmyID = -1;

	/** Owning wizard index. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Army")
	int32 OwnerWizardIndex = -1;

private:
	UPROPERTY(VisibleAnywhere, Category = "Army")
	TObjectPtr<UBillboardComponent> IconSprite;

	UPROPERTY(VisibleAnywhere, Category = "Army")
	TObjectPtr<UTextRenderComponent> CountLabel;

	/** Get a colour for a wizard index (shared palette with city icons). */
	static FColor GetWizardColor(int32 WizardIndex);
};
