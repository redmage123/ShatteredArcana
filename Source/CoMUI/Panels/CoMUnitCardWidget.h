// Copyright Mythforge Studios. All Rights Reserved.
// CoMUnitCardWidget.h -- Trading-card style unit detail popup.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMUnitCardWidget.generated.h"

class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UButton;
class USizeBox;
class UScrollBox;

/**
 * UCoMUnitCardWidget
 *
 * A trading-card style popup displaying a single unit's full details:
 * name, race, portrait placeholder, stat grid, skills, and enchantments.
 * Built entirely in C++ via RebuildWidget().
 */
UCLASS()
class COMUI_API UCoMUnitCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Load and display data for the specified unit. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UnitCard")
	void SetUnit(int32 UnitId);

	/** Direct-set for preview / testing without subsystem lookup. */
	UFUNCTION(BlueprintCallable, Category = "CoM|UnitCard")
	void SetUnitData(const FText& Name, const FText& Race,
	                 int32 Attack, int32 Defense, int32 HP, int32 MaxHP,
	                 int32 Movement, int32 Ranged, int32 Resistance,
	                 int32 Level, int32 XP);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	// -- Layout construction ------------------------------------------------

	void BuildCardLayout();

	/** Helper: create a stat row (Label: Value) pair inside a horizontal box. */
	UHorizontalBox* CreateStatPair(const FString& Label1, const FString& Value1,
	                               const FString& Label2, const FString& Value2);

	/** Helper: create a styled button matching dark fantasy theme. */
	UButton* CreateStyledButton(const FString& Label);

	// -- Button callbacks --------------------------------------------------

	UFUNCTION()
	void OnCloseClicked();

	// -- Widget references (created in C++) --------------------------------

	UPROPERTY() TObjectPtr<UBorder> OuterBorder;
	UPROPERTY() TObjectPtr<UVerticalBox> ContentBox;
	UPROPERTY() TObjectPtr<UTextBlock> UnitNameText;
	UPROPERTY() TObjectPtr<UTextBlock> RaceText;
	UPROPERTY() TObjectPtr<UBorder> PortraitBorder;
	UPROPERTY() TObjectPtr<class UImage> PortraitImage;

	// Stat value text blocks
	UPROPERTY() TObjectPtr<UTextBlock> AttackValueText;
	UPROPERTY() TObjectPtr<UTextBlock> DefenseValueText;
	UPROPERTY() TObjectPtr<UTextBlock> HPValueText;
	UPROPERTY() TObjectPtr<UTextBlock> MovementValueText;
	UPROPERTY() TObjectPtr<UTextBlock> RangedValueText;
	UPROPERTY() TObjectPtr<UTextBlock> ResistanceValueText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelValueText;
	UPROPERTY() TObjectPtr<UTextBlock> XPValueText;

	UPROPERTY() TObjectPtr<UScrollBox> SkillsScrollBox;
	UPROPERTY() TObjectPtr<UScrollBox> EnchantmentsScrollBox;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentUnitId = -1;

	TWeakObjectPtr<UCoMUnitSubsystem> CachedUnitSubsystem;
	UCoMUnitSubsystem* GetUnitSubsystem();
};
