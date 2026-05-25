// Copyright Mythforge Studios. All Rights Reserved.
// CoMEnchantmentPanelWidget.h -- Caster-of-Magic style "Enchantments" screen.
// Lists every global enchantment currently in play (all wizards) with its
// tarot-card art, owner, upkeep and flavor. The viewing wizard can cancel
// their own enchantments to stop the upkeep drain.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMEnchantmentPanelWidget.generated.h"

class UVerticalBox;
class UCoMEnchantmentPanelWidget;

/** Per-row helper so each Cancel button knows which enchantment it owns. */
UCLASS()
class UCoMEnchantRow : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UCoMEnchantmentPanelWidget> Owner;
	UPROPERTY() int32 OwnerWizard = -1;
	UPROPERTY() FName SpellID;

	UFUNCTION()
	void OnCancelClicked();
};

UCLASS()
class COMUI_API UCoMEnchantmentPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Point the panel at a viewing wizard (controls which Cancel buttons show). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Enchantments")
	void Configure(int32 InViewerWizardIndex);

	/** Rebuild the active-enchantment list from the magic subsystem. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Enchantments")
	void Refresh();

	/** Cancel one wizard's enchantment, then refresh. Called by row helpers. */
	void CancelEnchantment(int32 OwnerWizard, FName SpellID);

protected:
	UFUNCTION()
	void OnCloseClicked();

private:
	void BuildLayout();
	void BuildRows();

	UPROPERTY() UVerticalBox* ListBox = nullptr;
	UPROPERTY() TArray<UCoMEnchantRow*> Rows;

	/** Wizard whose viewpoint this panel represents (0 = local human). */
	int32 ViewerWizardIndex = 0;
};
