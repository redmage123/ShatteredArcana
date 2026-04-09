// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellBookWidget.h -- Spell book / research screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMSpellBookWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UProgressBar;
class UHorizontalBox;
class UVerticalBox;
class USlider;
class UCoMMagicSubsystem;

/**
 * UCoMSpellBookWidget
 *
 * Spell book and research screen. Shows spell realm tabs, spell grids per
 * realm, current research progress, mana allocation slider, and casting
 * controls.
 */
UCLASS()
class COMUI_API UCoMSpellBookWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// -- Wizard Binding --------------------------------------------------------

	/** Load the spell data for a specific wizard. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellBook")
	void SetWizardId(int32 WizardId);

	// -- Realm Tabs ------------------------------------------------------------

	/** Switch to a specific spell realm tab. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellBook")
	void SelectRealm(ECoMSpellRealm Realm);

	// -- Spell Interaction -----------------------------------------------------

	/** Select a spell for research or casting. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellBook")
	void OnSpellClicked(FName SpellId);

	/** Attempt to cast the currently selected spell. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellBook")
	void OnCastClicked();

	/** Begin researching the currently selected spell. */
	UFUNCTION(BlueprintCallable, Category = "CoM|SpellBook")
	void OnResearchClicked();

	/** Get the currently selected spell ID. */
	UFUNCTION(BlueprintPure, Category = "CoM|SpellBook")
	FName GetSelectedSpellId() const { return SelectedSpellId; }

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnManaSliderChanged(float Value);

	UFUNCTION()
	void OnCastButtonClicked();

	UFUNCTION()
	void OnResearchButtonClicked();

	// -- Realm Tab Callbacks ---------------------------------------------------

	UFUNCTION() void OnLifeTabClicked();
	UFUNCTION() void OnDeathTabClicked();
	UFUNCTION() void OnChaosTabClicked();
	UFUNCTION() void OnNatureTabClicked();
	UFUNCTION() void OnSorceryTabClicked();
	UFUNCTION() void OnArcaneTabClicked();
	UFUNCTION() void OnBindingTabClicked();
	UFUNCTION() void OnSpiritTabClicked();
	UFUNCTION() void OnGlamourTabClicked();

	// -- Widget references -----------------------------------------------------

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WizardNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResearchText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedSpellText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentRealmText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ResearchProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> ManaAllocationSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ManaAllocationLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SpellListScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> RealmTabsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CastButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResearchButton;

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
	/** Refresh the spell list for the current realm. */
	void RefreshSpellList();

	/** Resolve the magic subsystem. */
	UCoMMagicSubsystem* GetMagicSubsystem();

	int32 CurrentWizardId = -1;
	ECoMSpellRealm CurrentRealm = ECoMSpellRealm::Arcane;
	FName SelectedSpellId = NAME_None;

	TWeakObjectPtr<UCoMMagicSubsystem> CachedMagicSubsystem;
};
