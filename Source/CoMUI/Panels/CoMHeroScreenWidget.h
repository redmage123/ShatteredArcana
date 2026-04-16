// Copyright Mythforge Studios. All Rights Reserved.
// CoMHeroScreenWidget.h -- Detailed hero character sheet.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMCore/Units/CoMHeroSubsystem.h"
#include "CoMHeroScreenWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UButton;
class USizeBox;
class UScrollBox;
class UProgressBar;

/**
 * UCoMHeroScreenWidget
 *
 * Full hero character sheet showing portrait, stats, skills, equipment,
 * loyalty meter, and personality traits. Split into left (portrait/identity)
 * and right (stats/skills/items) columns.
 * Built entirely in C++ via RebuildWidget().
 */
UCLASS()
class COMUI_API UCoMHeroScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load and display data for the specified hero unit. */
	UFUNCTION(BlueprintCallable, Category = "CoM|HeroScreen")
	void SetHero(int32 HeroUnitId);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// -- Layout construction ------------------------------------------------

	void BuildHeroLayout();

	/** Build the left column (portrait, name, class, level, loyalty). */
	void BuildLeftColumn(UVerticalBox* LeftBox);

	/** Build the right column (stats, skills, equipment, traits). */
	void BuildRightColumn(UVerticalBox* RightBox);

	/** Helper: create a stat row pair. */
	UHorizontalBox* CreateStatPair(const FString& Label1, const FString& Value1,
	                               const FString& Label2, const FString& Value2,
	                               UVerticalBox* Parent);

	/** Helper: create a styled button. */
	UButton* CreateStyledButton(const FString& Label, UVerticalBox* Parent);

	// -- Button callbacks --------------------------------------------------

	UFUNCTION()
	void OnCloseClicked();

	// Item slot click handlers (2x3 grid = 6 slots).
	UFUNCTION() void OnItem0Clicked();
	UFUNCTION() void OnItem1Clicked();
	UFUNCTION() void OnItem2Clicked();
	UFUNCTION() void OnItem3Clicked();
	UFUNCTION() void OnItem4Clicked();
	UFUNCTION() void OnItem5Clicked();

	// -- Widget references (created in C++) --------------------------------

	UPROPERTY() TObjectPtr<UBorder> OuterBorder;
	UPROPERTY() TObjectPtr<UVerticalBox> MainContentBox;

	// Left column
	UPROPERTY() TObjectPtr<UTextBlock> HeroNameText;
	UPROPERTY() TObjectPtr<UTextBlock> HeroClassText;
	UPROPERTY() TObjectPtr<UBorder> PortraitBorder;
	UPROPERTY() TObjectPtr<UTextBlock> LevelXPText;
	UPROPERTY() TObjectPtr<UProgressBar> XPProgressBar;
	UPROPERTY() TObjectPtr<UTextBlock> LoyaltyLabel;
	UPROPERTY() TObjectPtr<UProgressBar> LoyaltyBar;

	// Right column — stats
	UPROPERTY() TObjectPtr<UTextBlock> AttackValueText;
	UPROPERTY() TObjectPtr<UTextBlock> DefenseValueText;
	UPROPERTY() TObjectPtr<UTextBlock> HPValueText;
	UPROPERTY() TObjectPtr<UTextBlock> MovementValueText;
	UPROPERTY() TObjectPtr<UTextBlock> RangedValueText;
	UPROPERTY() TObjectPtr<UTextBlock> ResistanceValueText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelValueText;
	UPROPERTY() TObjectPtr<UTextBlock> XPValueText;

	// Right column — skills
	UPROPERTY() TObjectPtr<UScrollBox> SkillsScrollBox;

	// Right column — equipment (6 item slots)
	static constexpr int32 MAX_ITEM_SLOTS = 6;
	UPROPERTY() TObjectPtr<UButton> ItemSlotButtons[MAX_ITEM_SLOTS];
	UPROPERTY() TObjectPtr<UTextBlock> ItemSlotLabels[MAX_ITEM_SLOTS];

	// Right column — personality traits
	UPROPERTY() TObjectPtr<UTextBlock> TraitsText;

	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentHeroUnitId = -1;

	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSubsystem;
	TWeakObjectPtr<UCoMHeroSubsystem> CachedHeroSubsystem;
	UCoMUnitSubsystem* GetUnitSubsystem();
	UCoMHeroSubsystem* GetHeroSubsystem();
};
