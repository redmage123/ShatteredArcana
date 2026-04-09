// Copyright Shattered Arcana. All Rights Reserved.
// CoMPreBattleWidget.h -- Pre-battle popup: Auto-Resolve vs Fight choice.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMPreBattleWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UCoMCombatSubsystem;

/**
 * UCoMPreBattleWidget
 *
 * Shows attacker vs defender army info (name, power, unit count)
 * with predicted odds, and two buttons: "Auto-Resolve" and "Fight".
 */
UCLASS()
class COMUI_API UCoMPreBattleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Populate the widget with the two armies about to fight. */
	UFUNCTION(BlueprintCallable, Category = "CoM|PreBattle")
	void SetBattle(int32 AttackerArmyId, int32 DefenderArmyId);

protected:
	UFUNCTION()
	void OnAutoResolveClicked();

	UFUNCTION()
	void OnFightClicked();

	// -- Widget references (bound from UMG or created in C++) --
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackerInfoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefenderInfoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OddsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AutoResolveButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> FightButton;

private:
	UCoMCombatSubsystem* GetCombatSubsystem();

	int32 CachedAttackerArmyId = INDEX_NONE;
	int32 CachedDefenderArmyId = INDEX_NONE;
};
