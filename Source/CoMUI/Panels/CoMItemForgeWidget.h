// Copyright Mythforge Studios. All Rights Reserved.
// CoMItemForgeWidget.h -- Forge magical items / artifacts. Used by the
// Enchant Item and Create Artifact spell flows.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMItemTypes.h"
#include "CoMItemForgeWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UCoMItemForgeWidget;

/** Per-row helper that owns the Click UFunction so each catalog row can carry its own PowerID. */
UCLASS()
class COMUI_API UCoMForgePowerRow : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UCoMItemForgeWidget> Owner;
	UPROPERTY() FName PowerID;

	UFUNCTION() void HandleClick();
};

/**
 * UCoMItemForgeWidget
 *
 * Two-column forge UI:
 *   Left  -- slot picker, name input, summary (selected powers + total mana cost)
 *   Right -- catalog of available powers; click to toggle in/out of selection
 *
 * Bottom bar -- Cancel / Forge.
 */
UCLASS()
class COMUI_API UCoMItemForgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Configure for either Enchant Item (false) or Create Artifact (true). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Items")
	void Configure(int32 InOwnerWizardIndex, bool bInArtifactMode);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnSlotWeapon();
	UFUNCTION() void OnSlotOffhand();
	UFUNCTION() void OnSlotArmor();
	UFUNCTION() void OnSlotHelm();
	UFUNCTION() void OnSlotBoots();
	UFUNCTION() void OnSlotRing();
	UFUNCTION() void OnSlotAmulet();
	UFUNCTION() void OnSlotRelic();

	UFUNCTION() void OnForgeClicked();
	UFUNCTION() void OnCancelClicked();

public:
	/** Called by UCoMForgePowerRow::HandleClick. Toggles the given power in/out of the selection. */
	void TogglePower(FName PowerID);

private:
	void BuildLayout();
	void SelectSlot(ECoMItemSlot NewSlot);
	void RebuildPowerList();
	void RebuildSummary();
	UButton* MakeBtn(const FString& Label, float Width = 110.f);

	/** Keep per-row helper objects alive while the widget is shown. */
	UPROPERTY() TArray<TObjectPtr<UCoMForgePowerRow>> RowHelpers;

	// State -------------------------------------------------------------------
	int32 OwnerWizardIndex = -1;
	bool  bArtifactMode    = false;
	ECoMItemSlot SelectedSlot = ECoMItemSlot::Weapon;
	TArray<FName> SelectedPowerIDs;

	// Widgets -----------------------------------------------------------------
	UPROPERTY() TObjectPtr<UBorder>           BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock>        TitleText;
	UPROPERTY() TObjectPtr<UEditableTextBox>  NameInput;

	UPROPERTY() TObjectPtr<UButton>           SlotButtons[8];
	UPROPERTY() TObjectPtr<UBorder>           SlotButtonBorders[8];

	UPROPERTY() TObjectPtr<UScrollBox>        PowerListScroll;
	UPROPERTY() TObjectPtr<UScrollBox>        SelectedListScroll;
	UPROPERTY() TObjectPtr<UTextBlock>        TotalCostText;

	UPROPERTY() TObjectPtr<UButton>           ForgeButton;
	UPROPERTY() TObjectPtr<UButton>           CancelButton;
};
