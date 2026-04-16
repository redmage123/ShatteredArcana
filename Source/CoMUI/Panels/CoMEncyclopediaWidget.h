// Copyright Mythforge Studios. All Rights Reserved.
// CoMEncyclopediaWidget.h -- Codex Arcanum encyclopedia browser.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMEncyclopediaWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UOverlay;
class UEditableTextBox;

/**
 * UCoMEncyclopediaWidget
 *
 * Encyclopedia / codex panel with a category sidebar (Units, Buildings, Spells,
 * Races, Planes, Items, Dragons), a search bar, and a scrollable content area
 * showing entries for the selected category.
 */
UCLASS()
class COMUI_API UCoMEncyclopediaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Switch to a specific category. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Encyclopedia")
	void SelectCategory(const FString& Category);

protected:
	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnUnitsClicked();

	UFUNCTION()
	void OnBuildingsClicked();

	UFUNCTION()
	void OnSpellsClicked();

	UFUNCTION()
	void OnRacesClicked();

	UFUNCTION()
	void OnPlanesClicked();

	UFUNCTION()
	void OnItemsClicked();

	UFUNCTION()
	void OnDragonsClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ContentScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> SearchBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// Category buttons
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> UnitsBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BuildingsBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SpellsBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> RacesBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PlanesBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ItemsBtn;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DragonsBtn;

private:
	void BuildLayout();
	UButton* CreateCategoryButton(const FString& Label, UVerticalBox* Parent);
	UButton* CreateActionButton(const FString& Label, float Width = 140.f);

	/** Add a codex entry to the content scroll box. */
	void AddCodexEntry(const FString& Name, const FString& Description);

	/** Populate placeholder entries for a category. */
	void PopulateCategory(const FString& Category);

	FString CurrentCategory = TEXT("Units");
};
