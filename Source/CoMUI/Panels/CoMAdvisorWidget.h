// Copyright Mythforge Studios. All Rights Reserved.
// CoMAdvisorWidget.h -- Strategic advisor recommendations panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMAdvisorWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * UCoMAdvisorWidget
 *
 * Royal Advisor panel with strategic recommendations across
 * Military, Economic, Magic, and Diplomatic domains.
 */
UCLASS()
class COMUI_API UCoMAdvisorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Set which wizard to provide advice for. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Advisor")
	void SetWizardId(int32 WizardId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void OnCloseClicked();

private:
	void BuildLayout();
	void AddAdviceSection(UVerticalBox* Parent, const FString& Title, const FLinearColor& AccentColor, const FString& Advice);
	UButton* CreateStyledButton(UVerticalBox* Parent, const FString& Label, float Width = 140.f);

	UPROPERTY() TObjectPtr<UBorder> BackgroundBorder;
	UPROPERTY() TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY() TObjectPtr<UScrollBox> AdviceScrollBox;
	UPROPERTY() TObjectPtr<UButton> CloseButton;

	int32 CurrentWizardId = -1;
};
