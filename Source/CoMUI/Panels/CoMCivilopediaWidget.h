// Copyright Shattered Arcana. All Rights Reserved.
// CoMCivilopediaWidget.h -- In-game searchable encyclopedia.
//
// Single screen pulling live data from the spell / unit / building /
// enchantment databases plus handwritten race and mechanics topics.
// Players open it from the HUD or via `com.show_civilopedia`.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCivilopediaWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UImage;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;

UENUM(BlueprintType)
enum class ECoMCivilopediaCategory : uint8
{
	Spells,
	Units,
	Buildings,
	Enchantments,
	Races,
	Mechanics,
};

/**
 * UCoMCivilopediaWidget
 *
 * Three-pane layout:
 *   - Left: category tab buttons
 *   - Middle: filterable list of entries in the active category
 *   - Right: full detail for the selected entry (name, stats, lore, art)
 */
UCLASS()
class COMUI_API UCoMCivilopediaWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Switch active category and rebuild the entry list. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Civilopedia")
	void SetCategory(ECoMCivilopediaCategory NewCategory);

	/** Apply a free-text filter to the current category's list. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Civilopedia")
	void SetFilter(const FString& NewFilter);

	/** Show the detail pane for an entry by ID (FName key in the active category). */
	UFUNCTION(BlueprintCallable, Category = "CoM|Civilopedia")
	void ShowEntry(FName EntryID);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnSpellsClicked();
	UFUNCTION() void OnUnitsClicked();
	UFUNCTION() void OnBuildingsClicked();
	UFUNCTION() void OnEnchantmentsClicked();
	UFUNCTION() void OnRacesClicked();
	UFUNCTION() void OnMechanicsClicked();
	UFUNCTION() void OnFilterChanged(const FText& Text);

private:
	void BuildLayout();
	void RebuildList();
	void RebuildDetail();

	UButton* MakeCategoryButton(const FString& Label, ECoMCivilopediaCategory Cat);

	UPROPERTY() TObjectPtr<UBorder> RootBorder;
	UPROPERTY() TObjectPtr<UButton> CloseButton;
	UPROPERTY() TObjectPtr<UVerticalBox> CategoryColumn;
	UPROPERTY() TObjectPtr<UEditableTextBox> FilterBox;
	UPROPERTY() TObjectPtr<UScrollBox> EntryListScroll;
	UPROPERTY() TObjectPtr<UScrollBox> DetailScroll;
	UPROPERTY() TObjectPtr<UTextBlock> DetailTitle;
	UPROPERTY() TObjectPtr<UTextBlock> DetailSubtitle;
	UPROPERTY() TObjectPtr<UVerticalBox> DetailBody;
	UPROPERTY() TObjectPtr<UImage> DetailArt;

	UPROPERTY() ECoMCivilopediaCategory CurrentCategory = ECoMCivilopediaCategory::Spells;
	FString CurrentFilter;
	FName SelectedEntry;
};
