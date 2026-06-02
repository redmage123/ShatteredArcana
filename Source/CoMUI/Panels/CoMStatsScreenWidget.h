// Copyright Shattered Arcana. All Rights Reserved.
// CoMStatsScreenWidget.h -- Career stats + achievement grid.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMStatsScreenWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

UCLASS()
class COMUI_API UCoMStatsScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	UFUNCTION() void OnCloseClicked();

private:
	void BuildLayout();

	UPROPERTY() TObjectPtr<UBorder>     RootBorder;
	UPROPERTY() TObjectPtr<UButton>     CloseButton;
	UPROPERTY() TObjectPtr<UVerticalBox> StatsColumn;
	UPROPERTY() TObjectPtr<UScrollBox>  AchievementScroll;
};
