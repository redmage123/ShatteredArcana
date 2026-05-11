// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMSpellCastCinematicWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "FileMediaSource.h"

#include "CoMUI/CoMUISubsystem.h"

namespace CastColours
{
	static const FLinearColor Dim       = FLinearColor(0.0f, 0.0f, 0.0f, 0.78f);
	static const FLinearColor FrameBg   = FLinearColor(0.04f, 0.03f, 0.10f, 0.95f);
	static const FLinearColor TitleGold = FLinearColor(0.9f, 0.7f, 0.18f, 1.0f);
	static const FLinearColor SubSilver = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);

	static FLinearColor RealmGlow(ECoMSpellRealm R)
	{
		switch (R)
		{
		case ECoMSpellRealm::Life:    return FLinearColor(1.0f, 0.95f, 0.55f, 1.0f);
		case ECoMSpellRealm::Death:   return FLinearColor(0.55f, 0.0f, 0.55f, 1.0f);
		case ECoMSpellRealm::Chaos:   return FLinearColor(1.0f, 0.25f, 0.05f, 1.0f);
		case ECoMSpellRealm::Nature:  return FLinearColor(0.10f, 0.85f, 0.10f, 1.0f);
		case ECoMSpellRealm::Sorcery: return FLinearColor(0.25f, 0.45f, 1.0f, 1.0f);
		case ECoMSpellRealm::Arcane:  return FLinearColor(0.9f,  0.65f, 0.10f, 1.0f);
		case ECoMSpellRealm::Binding: return FLinearColor(0.7f,  0.05f, 0.05f, 1.0f);
		case ECoMSpellRealm::Spirit:  return FLinearColor(0.65f, 0.45f, 1.0f, 1.0f);
		case ECoMSpellRealm::Glamour: return FLinearColor(1.0f,  0.45f, 0.85f, 1.0f);
		default:                      return FLinearColor::White;
		}
	}
}

TSharedRef<SWidget> UCoMSpellCastCinematicWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMSpellCastCinematicWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ElapsedSeconds = 0.0f;
}

void UCoMSpellCastCinematicWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);
	ElapsedSeconds += DeltaTime;

	// Fade-out across the last 0.6s.
	if (OuterDimmer)
	{
		const float Alpha = 1.0f - FMath::Clamp((ElapsedSeconds - (TotalSeconds - 0.6f)) / 0.6f, 0.0f, 1.0f);
		FLinearColor C = CastColours::Dim;
		C.A *= Alpha;
		OuterDimmer->SetBrushColor(C);
		SetRenderOpacity(Alpha);
	}

	if (ElapsedSeconds >= TotalSeconds)
	{
		DismissCinematic();
	}
}

void UCoMSpellCastCinematicWidget::DismissCinematic()
{
	if (MediaPlayer)
	{
		MediaPlayer->Close();
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
	RemoveFromParent();
}

void UCoMSpellCastCinematicWidget::Configure(int32 WizardPortraitIndex,
                                              ECoMSpellRealm SpellRealm,
                                              const FString& SpellDisplayName,
                                              bool bIsArtifact)
{
	// Border tint by realm — saturated outer glow, subtle inner.
	if (CenterFrame)
	{
		CenterFrame->SetBrushColor(CastColours::FrameBg);
	}

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(SpellDisplayName));
		TitleText->SetColorAndOpacity(FSlateColor(CastColours::RealmGlow(SpellRealm)));
	}
	if (SubText)
	{
		SubText->SetText(FText::FromString(bIsArtifact
			? TEXT("forges an artifact")
			: TEXT("invokes a great working")));
	}

	// Try the per-wizard animated clip first; fall back to the static portrait.
	bool bVideoStarted = false;
	if (MediaPlayer && WizardPortraitIndex >= 0 && WizardPortraitIndex < 14)
	{
		// FileMediaSource referencing a /Game/Movies/WizardCast/wizard_NN.mp4
		const FString MediaAssetPath = FString::Printf(
			TEXT("/Game/Movies/WizardCast/wizard_cast_%02d.wizard_cast_%02d"),
			WizardPortraitIndex + 1, WizardPortraitIndex + 1);
		if (UMediaSource* Src = LoadObject<UMediaSource>(nullptr, *MediaAssetPath))
		{
			bVideoStarted = MediaPlayer->OpenSource(Src);
		}
	}

	// Static portrait fallback (or behind the video if it loaded).
	if (PortraitImage && WizardPortraitIndex >= 0 && WizardPortraitIndex < 14)
	{
		const FString PortPath = FString::Printf(
			TEXT("/Game/Textures/Wizards/Backgrounds/wizard_bg_%02d.wizard_bg_%02d"),
			WizardPortraitIndex + 1, WizardPortraitIndex + 1);
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *PortPath))
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(Tex);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(960.0f, 540.0f);
			Brush.TintColor = FSlateColor(FLinearColor(0.85f, 0.85f, 0.95f, 1.0f));
			PortraitImage->SetBrush(Brush);
		}
	}

	if (VideoImage)
	{
		VideoImage->SetVisibility(bVideoStarted ? ESlateVisibility::HitTestInvisible
		                                        : ESlateVisibility::Collapsed);
	}
	if (PortraitImage)
	{
		// Keep the still under the video; if no video, still is the show.
		PortraitImage->SetVisibility(bVideoStarted ? ESlateVisibility::Collapsed
		                                           : ESlateVisibility::HitTestInvisible);
	}

	ElapsedSeconds = 0.0f;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.0f);
}

void UCoMSpellCastCinematicWidget::BuildLayout()
{
	OuterDimmer = WidgetTree->ConstructWidget<UBorder>();
	OuterDimmer->SetBrushColor(CastColours::Dim);
	OuterDimmer->SetPadding(FMargin(0));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	OuterDimmer->AddChild(Root);

	// Centred 960x540 stage with a gold frame.
	USizeBox* StageSize = WidgetTree->ConstructWidget<USizeBox>();
	StageSize->SetWidthOverride(960.0f);
	StageSize->SetHeightOverride(620.0f);

	CenterFrame = WidgetTree->ConstructWidget<UBorder>();
	CenterFrame->SetBrushColor(CastColours::FrameBg);
	CenterFrame->SetPadding(FMargin(8.0f));
	StageSize->AddChild(CenterFrame);

	UOverlaySlot* SS = Root->AddChildToOverlay(StageSize);
	if (SS)
	{
		SS->SetHorizontalAlignment(HAlign_Center);
		SS->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* StageCol = WidgetTree->ConstructWidget<UVerticalBox>();
	CenterFrame->AddChild(StageCol);

	// Image stage (video on top, static portrait underneath).
	{
		USizeBox* PixSize = WidgetTree->ConstructWidget<USizeBox>();
		PixSize->SetWidthOverride(944.0f);
		PixSize->SetHeightOverride(530.0f);

		UOverlay* PixOverlay = WidgetTree->ConstructWidget<UOverlay>();
		PixSize->AddChild(PixOverlay);

		PortraitImage = WidgetTree->ConstructWidget<UImage>();
		UOverlaySlot* PSlot = PixOverlay->AddChildToOverlay(PortraitImage);
		if (PSlot) { PSlot->SetHorizontalAlignment(HAlign_Fill); PSlot->SetVerticalAlignment(VAlign_Fill); }

		VideoImage = WidgetTree->ConstructWidget<UImage>();
		VideoImage->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* VSlot = PixOverlay->AddChildToOverlay(VideoImage);
		if (VSlot) { VSlot->SetHorizontalAlignment(HAlign_Fill); VSlot->SetVerticalAlignment(VAlign_Fill); }

		StageCol->AddChildToVerticalBox(PixSize);
	}

	// Title banner.
	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(FText::FromString(TEXT("Spell")));
	TitleText->SetColorAndOpacity(FSlateColor(CastColours::TitleGold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 32; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = StageCol->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 8, 0, 2)); }

	SubText = WidgetTree->ConstructWidget<UTextBlock>();
	SubText->SetText(FText::FromString(TEXT("invokes a great working")));
	SubText->SetColorAndOpacity(FSlateColor(CastColours::SubSilver));
	SubText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = SubText->GetFont(); F.Size = 14; SubText->SetFont(F); }
	UVerticalBoxSlot* US = StageCol->AddChildToVerticalBox(SubText);
	if (US) { US->SetHorizontalAlignment(HAlign_Center); US->SetPadding(FMargin(0, 0, 0, 6)); }

	// Set up media player + texture for video clips. The texture is bound
	// onto the VideoImage brush in Configure() once a clip opens.
	MediaPlayer  = NewObject<UMediaPlayer>(this);
	MediaTexture = NewObject<UMediaTexture>(this);
	if (MediaTexture && MediaPlayer)
	{
		MediaTexture->SetMediaPlayer(MediaPlayer);
		MediaTexture->UpdateResource();
		FSlateBrush VBrush;
		VBrush.SetResourceObject(MediaTexture);
		VBrush.ImageSize = FVector2D(944.0f, 530.0f);
		if (VideoImage) { VideoImage->SetBrush(VBrush); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = OuterDimmer;
	}
}
