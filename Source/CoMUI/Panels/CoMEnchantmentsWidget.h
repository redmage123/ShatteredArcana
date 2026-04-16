// Copyright Mythforge Studios. All Rights Reserved.
// CoMEnchantmentsWidget.h -- Active global enchantments panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMEnchantmentsWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMEnchantmentsWidget
 *
 * Panel showing active global enchantments with caster info,
 * mana cost per turn, and dispel controls.
 */
UCLASS()
class COMUI_API UCoMEnchantmentsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Set which wizard's enchantments to display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Enchantments")
	void SetWizardId(int32 WizardId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void OnCloseClicked();

private:
	void BuildLayout();
	UButton* CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width = 160.f);

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UScrollBox> EnchantmentListScrollBox;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentWizardId = -1;
};
