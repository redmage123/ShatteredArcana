// Copyright Mythforge Studios. All Rights Reserved.
// CoMTutorialWidget.h -- First-turn welcome / how-to-play modal.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMTutorialWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMTutorialWidget
 *
 * Lightweight onboarding modal shown to new players on Turn 1.
 * Explains: end-turn cycle, spell book, city queue, hero recruitment,
 * mana nodes, and Spell of Mastery as the win condition.
 * Dismissable via [Got it] or Esc.
 */
UCLASS()
class COMUI_API UCoMTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION() void OnDismissClicked();

private:
	void BuildLayout();

	UPROPERTY() TObjectPtr<UBorder>     BackgroundBorder;
	UPROPERTY() TObjectPtr<UButton>     DismissButton;
	UPROPERTY() TObjectPtr<UTextBlock>  TitleText;
	UPROPERTY() TObjectPtr<UVerticalBox> ContentBox;
};
