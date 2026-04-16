// Copyright Mythforge Studios. All Rights Reserved.
// CoMTradeWidget.h -- Trade route management panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMTradeWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMTradeWidget
 *
 * Trade route management panel showing active routes, partner wizards,
 * traded goods, income, and controls to propose new trades.
 */
UCLASS()
class COMUI_API UCoMTradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Set which wizard's trade routes to display. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Trade")
	void SetWizardId(int32 WizardId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnProposeTradeClicked();

private:
	void BuildLayout();
	UButton* CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width = 160.f);

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UScrollBox> TradeListScrollBox;
	UPROPERTY() TObjectPtr<UButton> ProposeTradeButton;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentWizardId = -1;
};
