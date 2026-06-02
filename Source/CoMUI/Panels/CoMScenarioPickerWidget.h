// Copyright Shattered Arcana. All Rights Reserved.
// CoMScenarioPickerWidget.h -- New-game scenario picker.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMScenarioPickerWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class COMUI_API UCoMScenarioPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnStartClicked();

private:
	void BuildLayout();
	void RebuildScenarioList();
	void SelectScenario(FName ScenarioID);

	UPROPERTY() TObjectPtr<UBorder>     RootBorder;
	UPROPERTY() TObjectPtr<UScrollBox>  ScenarioListScroll;
	UPROPERTY() TObjectPtr<UTextBlock>  DetailNameText;
	UPROPERTY() TObjectPtr<UTextBlock>  DetailSubText;
	UPROPERTY() TObjectPtr<UTextBlock>  DetailBodyText;
	UPROPERTY() TObjectPtr<UButton>     StartButton;
	UPROPERTY() TObjectPtr<UButton>     CloseButton;

	FName SelectedScenarioID;
};
