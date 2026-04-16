// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellTreeWidget.h -- Visual spell research tree.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMSpellTreeWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;
class UCanvasPanel;

/**
 * UCoMSpellTreeWidget
 *
 * Fullscreen spell research tree organized by realm tabs and tiers.
 * Spell nodes are color-coded by state (known, researching, available, locked)
 * with prerequisite lines connecting them. Bottom panel shows selected spell
 * details and a "Research This" button.
 */
UCLASS()
class COMUI_API UCoMSpellTreeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Set which wizard's research tree to display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTree")
	void SetWizardId(int32 WizardId);

	/** Switch to a specific spell realm tab. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellTree")
	void SelectRealm(ECoMSpellRealm Realm);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnResearchThisClicked();

	// Realm tab callbacks
	UFUNCTION() void OnLifeTabClicked();
	UFUNCTION() void OnDeathTabClicked();
	UFUNCTION() void OnChaosTabClicked();
	UFUNCTION() void OnNatureTabClicked();
	UFUNCTION() void OnSorceryTabClicked();
	UFUNCTION() void OnArcaneTabClicked();
	UFUNCTION() void OnBindingTabClicked();
	UFUNCTION() void OnSpiritTabClicked();
	UFUNCTION() void OnGlamourTabClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> RealmTabsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> TreeScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedSpellNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedSpellDescText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedSpellCostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResearchThisButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// Realm tab buttons
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LifeTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeathTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ChaosTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> NatureTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SorceryTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ArcaneTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BindingTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SpiritTab;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> GlamourTab;

private:
	void BuildLayout();
	UButton* CreateRealmTab(const FString& Label, const FLinearColor& Color);
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Create a spell node button with the given state color. */
	UButton* CreateSpellNode(const FString& SpellName, const FLinearColor& NodeColor);

	/** Create a vertical prerequisite line for the tree scroll box. */
	void CreatePrereqLine(UVerticalBox* Parent);

	int32 CurrentWizardId = -1;
	ECoMSpellRealm CurrentRealm = ECoMSpellRealm::Arcane;
	FName SelectedSpellId = NAME_None;
};
