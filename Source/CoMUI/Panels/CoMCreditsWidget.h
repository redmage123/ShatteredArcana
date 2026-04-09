// Copyright Mythforge Studios. All Rights Reserved.
// CoMCreditsWidget.h -- Full-screen scrolling credits overlay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCreditsWidget.generated.h"

class UScrollBox;
class UBorder;
class UVerticalBox;

/**
 * UCoMCreditsWidget
 *
 * Full-screen scrolling credits screen styled like film credits.
 * Text auto-scrolls upward. Press Escape or click anywhere to close.
 */
UCLASS()
class COMUI_API UCoMCreditsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// -- Public API ---------------------------------------------------------------

	/** Reset scroll position and begin auto-scrolling. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Credits")
	void StartCredits();

	/** Stop scrolling and remove from viewport. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Credits")
	void StopCredits();

	/** Scroll speed in pixels per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoM|Credits")
	float ScrollSpeed = 30.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** Build the entire credits layout in C++. */
	void BuildCreditsContent();

	/** Helper: add a text line to the content box. */
	void AddTextLine(const FString& Text, const FLinearColor& Color, int32 FontSize, bool bBold = false);

	/** Helper: add a gold horizontal separator. */
	void AddSeparator();

	/** Helper: add vertical spacing. */
	void AddSpacer(float Height);

	// -- Widget references (created in C++) ---------------------------------------

	UPROPERTY()
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY()
	TObjectPtr<UScrollBox> CreditsScrollBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ContentBox;

	/** Whether auto-scroll is active. */
	bool bIsScrolling = false;

	/** Current accumulated scroll offset. */
	float CurrentScrollOffset = 0.0f;
};
