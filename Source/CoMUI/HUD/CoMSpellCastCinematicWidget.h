// Copyright Mythforge Studios. All Rights Reserved.
// CoMSpellCastCinematicWidget.h -- Fullscreen cast cinematic shown for
// "big" spells: artifact creation, summons, global enchantments, rituals.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMSpellCastCinematicWidget.generated.h"

class UBorder;
class UImage;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class UTextBlock;

/**
 * UCoMSpellCastCinematicWidget
 *
 * MoM-style cinematic cast overlay. Plays a per-wizard 3-second clip with
 * a name banner ("Wizard Solarius casts Just Cause"), then auto-dismisses.
 *
 * Falls back gracefully when no animated clip is available — uses the
 * static wizard portrait + spell name + glow border instead.
 */
UCLASS()
class COMUI_API UCoMSpellCastCinematicWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * Show the cinematic.
	 *   WizardPortraitIndex -- 0..13 to pick the wizard portrait + cast clip.
	 *   SpellRealm          -- decides the glow color.
	 *   SpellDisplayName    -- shown on the title banner ("Just Cause").
	 *   bIsArtifact         -- swaps banner to "forges an artifact" wording.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoM|Cast")
	void Configure(int32 WizardPortraitIndex, ECoMSpellRealm SpellRealm,
	               const FString& SpellDisplayName, bool bIsArtifact);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();
	void DismissCinematic();

	UPROPERTY() TObjectPtr<UBorder>     OuterDimmer;
	UPROPERTY() TObjectPtr<UBorder>     CenterFrame;
	UPROPERTY() TObjectPtr<UImage>      PortraitImage;
	UPROPERTY() TObjectPtr<UImage>      VideoImage;
	UPROPERTY() TObjectPtr<UTextBlock>  TitleText;
	UPROPERTY() TObjectPtr<UTextBlock>  SubText;

	UPROPERTY() TObjectPtr<UMediaPlayer>  MediaPlayer;
	UPROPERTY() TObjectPtr<UMediaTexture> MediaTexture;

	float ElapsedSeconds = 0.0f;
	static constexpr float TotalSeconds = 3.0f;
};
