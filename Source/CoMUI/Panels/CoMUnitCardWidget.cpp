// Copyright Mythforge Studios. All Rights Reserved.
// CoMUnitCardWidget.cpp -- Trading-card style unit detail popup implementation.

#include "CoMUnitCardWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "CoMCore/Units/CoMUnitSubsystem.h"
#include "CoMUI/CoMUISubsystem.h"

// =============================================================================
// Colour palette — dark fantasy card theme
// =============================================================================

namespace UC
{
	static const FLinearColor BgDark        = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);  // #0e0e1a
	static const FLinearColor PanelBg       = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold          = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);   // #daa520
	static const FLinearColor GoldDim       = FLinearColor(0.500f, 0.380f, 0.080f, 0.7f);
	static const FLinearColor Silver        = FLinearColor(0.820f, 0.820f, 0.860f, 1.0f);
	static const FLinearColor White         = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	static const FLinearColor Grey          = FLinearColor(0.400f, 0.400f, 0.450f, 1.0f);
	static const FLinearColor PortraitDefault = FLinearColor(0.15f, 0.12f, 0.20f, 1.0f);

	static const FLinearColor BtnNormal     = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover      = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPressed    = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
	static const FLinearColor BtnBorder     = FLinearColor(0.500f, 0.380f, 0.080f, 0.7f);
}

// =============================================================================
// RebuildWidget
// =============================================================================

TSharedRef<SWidget> UCoMUnitCardWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildCardLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMUnitCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UCoMUnitCardWidget::OnCloseClicked);
	}
}

// =============================================================================
// Subsystem access
// =============================================================================

UCoMUnitSubsystem* UCoMUnitCardWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid())
	{
		return CachedUnitSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI)
	{
		UCoMUnitSubsystem* Subsystem = GI->GetSubsystem<UCoMUnitSubsystem>();
		CachedUnitSubsystem = Subsystem;
		return Subsystem;
	}
	return nullptr;
}

// =============================================================================
// Public API
// =============================================================================

void UCoMUnitCardWidget::SetUnit(int32 UnitId)
{
	CurrentUnitId = UnitId;

	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	if (!UnitSub)
	{
		return;
	}

	const FCoMUnitInstance* Unit = UnitSub->GetUnit(UnitId);
	if (!Unit)
	{
		return;
	}

	// Unit name from SpecID
	if (UnitNameText)
	{
		UnitNameText->SetText(FText::FromString(Unit->SpecID.ToString()));
	}

	// Race tag
	if (RaceText)
	{
		RaceText->SetText(FText::FromString(
			Unit->RaceTag.IsValid() ? Unit->RaceTag.ToString() : TEXT("Unknown Race")));
	}

	// Portrait colour based on race hash (fallback backdrop).
	if (PortraitBorder)
	{
		const uint32 Hash = GetTypeHash(Unit->RaceTag.GetTagName());
		FLinearColor RaceColor(
			0.1f + FMath::Frac(Hash * 0.00137f) * 0.3f,
			0.1f + FMath::Frac(Hash * 0.00251f) * 0.3f,
			0.15f + FMath::Frac(Hash * 0.00419f) * 0.3f,
			1.0f);
		PortraitBorder->SetBrushColor(RaceColor);
	}

	// Portrait art by SpecID, if one has been authored (/Game/UI/Units/<SpecID>).
	if (PortraitImage)
	{
		const FString SpecStr = Unit->SpecID.ToString();
		const FString TexPath = FString::Printf(TEXT("/Game/UI/Units/%s.%s"), *SpecStr, *SpecStr);
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *TexPath))
		{
			PortraitImage->SetBrushFromTexture(Tex);
			PortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Stats — from the unit instance (HP, Level, XP) and spec data would normally
	// provide Attack/Defense/etc. We show what the unit instance has and defaults.
	if (HPValueText)
	{
		HPValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), Unit->CurrentHP, Unit->MaxHP)));
	}
	if (LevelValueText)
	{
		LevelValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), Unit->Level)));
	}
	if (XPValueText)
	{
		XPValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), Unit->Experience)));
	}

	// Movement type
	if (MovementValueText)
	{
		const UEnum* MoveEnum = StaticEnum<ECoMMovementType>();
		FString MoveStr = MoveEnum
			? MoveEnum->GetDisplayNameTextByValue(static_cast<int64>(Unit->MovementType)).ToString()
			: TEXT("?");
		MovementValueText->SetText(FText::FromString(MoveStr));
	}

	// Skills
	if (SkillsScrollBox)
	{
		SkillsScrollBox->ClearChildren();
		for (const FCoMSkillEntry& Skill : Unit->Skills)
		{
			UTextBlock* SkillText = NewObject<UTextBlock>(this);
			if (SkillText)
			{
				SkillText->SetText(FText::FromString(
					FString::Printf(TEXT("  %s (Lv %d)"), *Skill.SkillID.ToString(), Skill.Level)));
				SkillText->SetColorAndOpacity(FSlateColor(UC::Silver));
				FSlateFontInfo Font = SkillText->GetFont();
				Font.Size = 10;
				SkillText->SetFont(Font);
				SkillsScrollBox->AddChild(SkillText);
			}
		}
		if (Unit->Skills.Num() == 0)
		{
			UTextBlock* NoSkills = NewObject<UTextBlock>(this);
			if (NoSkills)
			{
				NoSkills->SetText(FText::FromString(TEXT("  (None)")));
				NoSkills->SetColorAndOpacity(FSlateColor(UC::Grey));
				FSlateFontInfo Font = NoSkills->GetFont();
				Font.Size = 10;
				NoSkills->SetFont(Font);
				SkillsScrollBox->AddChild(NoSkills);
			}
		}
	}

	// Enchantments
	if (EnchantmentsScrollBox)
	{
		EnchantmentsScrollBox->ClearChildren();
		for (const FCoMEnchantmentInstance& Ench : Unit->Enchantments)
		{
			UTextBlock* EnchText = NewObject<UTextBlock>(this);
			if (EnchText)
			{
				FString Duration = (Ench.TurnsRemaining < 0)
					? TEXT("permanent")
					: FString::Printf(TEXT("%d turns"), Ench.TurnsRemaining);
				EnchText->SetText(FText::FromString(
					FString::Printf(TEXT("  %s (%s)"), *Ench.SpellID.ToString(), *Duration)));
				EnchText->SetColorAndOpacity(FSlateColor(UC::Silver));
				FSlateFontInfo Font = EnchText->GetFont();
				Font.Size = 10;
				EnchText->SetFont(Font);
				EnchantmentsScrollBox->AddChild(EnchText);
			}
		}
		if (Unit->Enchantments.Num() == 0)
		{
			UTextBlock* NoEnch = NewObject<UTextBlock>(this);
			if (NoEnch)
			{
				NoEnch->SetText(FText::FromString(TEXT("  (None)")));
				NoEnch->SetColorAndOpacity(FSlateColor(UC::Grey));
				FSlateFontInfo Font = NoEnch->GetFont();
				Font.Size = 10;
				NoEnch->SetFont(Font);
				EnchantmentsScrollBox->AddChild(NoEnch);
			}
		}
	}
}

void UCoMUnitCardWidget::SetUnitData(const FText& Name, const FText& Race,
                                     int32 Attack, int32 Defense, int32 HP, int32 MaxHP,
                                     int32 Movement, int32 Ranged, int32 Resistance,
                                     int32 Level, int32 XP)
{
	if (UnitNameText)   { UnitNameText->SetText(Name); }
	if (RaceText)       { RaceText->SetText(Race); }
	if (AttackValueText)    { AttackValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Attack))); }
	if (DefenseValueText)   { DefenseValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Defense))); }
	if (HPValueText)        { HPValueText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), HP, MaxHP))); }
	if (MovementValueText)  { MovementValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Movement))); }
	if (RangedValueText)    { RangedValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Ranged))); }
	if (ResistanceValueText){ ResistanceValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Resistance))); }
	if (LevelValueText)     { LevelValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Level))); }
	if (XPValueText)        { XPValueText->SetText(FText::FromString(FString::Printf(TEXT("%d"), XP))); }
}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMUnitCardWidget::OnCloseClicked()
{
	if (auto* GI = GetGameInstance())
	{
		if (auto* UISS = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UISS->HideUnitCard();
		}
	}
}

// =============================================================================
// Layout
// =============================================================================

void UCoMUnitCardWidget::BuildCardLayout()
{
	// -- Card size constraint --------------------------------------------------
	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
	CardSize->SetWidthOverride(350.0f);
	CardSize->SetHeightOverride(500.0f);
	WidgetTree->RootWidget = CardSize;

	// -- Gold outer border -----------------------------------------------------
	OuterBorder = WidgetTree->ConstructWidget<UBorder>();
	OuterBorder->SetBrushColor(UC::Gold);
	OuterBorder->SetPadding(FMargin(2.0f));
	CardSize->AddChild(OuterBorder);

	// -- Dark inner panel ------------------------------------------------------
	UBorder* InnerBorder = WidgetTree->ConstructWidget<UBorder>();
	InnerBorder->SetBrushColor(UC::BgDark);
	InnerBorder->SetPadding(FMargin(10.0f, 8.0f));
	OuterBorder->AddChild(InnerBorder);

	// -- Content column --------------------------------------------------------
	ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
	InnerBorder->AddChild(ContentBox);

	// -- Unit name (gold, bold, size 20, centered) -----------------------------
	{
		UnitNameText = WidgetTree->ConstructWidget<UTextBlock>();
		UnitNameText->SetText(FText::FromString(TEXT("Unit Name")));
		UnitNameText->SetColorAndOpacity(FSlateColor(UC::Gold));
		UnitNameText->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = UnitNameText->GetFont();
		Font.Size = 20;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		UnitNameText->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(UnitNameText);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 2)); }
	}

	// -- Race tag line (silver, size 12, italic) -------------------------------
	{
		RaceText = WidgetTree->ConstructWidget<UTextBlock>();
		RaceText->SetText(FText::FromString(TEXT("Race")));
		RaceText->SetColorAndOpacity(FSlateColor(UC::Silver));
		RaceText->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = RaceText->GetFont();
		Font.Size = 12;
		Font.TypefaceFontName = FName(TEXT("Italic"));
		RaceText->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(RaceText);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 0, 0, 4)); }
	}

	// -- Horizontal divider (gold, 1px) ----------------------------------------
	{
		UBorder* Divider = WidgetTree->ConstructWidget<UBorder>();
		Divider->SetBrushColor(UC::Gold);
		USizeBox* DivSize = WidgetTree->ConstructWidget<USizeBox>();
		DivSize->SetHeightOverride(1.0f);
		DivSize->AddChild(Divider);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(DivSize);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Fill); SlotRef->SetPadding(FMargin(4, 4, 4, 4)); }
	}

	// -- Portrait area (placeholder: colored border 280x200) --------------------
	{
		USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>();
		PortraitSize->SetWidthOverride(280.0f);
		PortraitSize->SetHeightOverride(200.0f);

		PortraitBorder = WidgetTree->ConstructWidget<UBorder>();
		PortraitBorder->SetBrushColor(UC::PortraitDefault);
		PortraitBorder->SetPadding(FMargin(0.0f));
		PortraitSize->AddChild(PortraitBorder);

		// Portrait art (filled in per-unit if a texture exists; otherwise the
		// coloured border shows through).
		PortraitImage = WidgetTree->ConstructWidget<UImage>();
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		PortraitBorder->SetContent(PortraitImage);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(PortraitSize);
		if (SlotRef) { SlotRef->SetHorizontalAlignment(HAlign_Center); SlotRef->SetPadding(FMargin(0, 2, 0, 4)); }
	}

	// -- Stats grid (2 columns x 4 rows) ---------------------------------------
	{
		UHorizontalBox* Row1 = CreateStatPair(TEXT("Attack"), TEXT("--"), TEXT("Defense"), TEXT("--"));
		AttackValueText = Cast<UTextBlock>(Row1->GetChildAt(1));
		DefenseValueText = Cast<UTextBlock>(Row1->GetChildAt(3));

		UHorizontalBox* Row2 = CreateStatPair(TEXT("HP"), TEXT("--/--"), TEXT("Movement"), TEXT("--"));
		HPValueText = Cast<UTextBlock>(Row2->GetChildAt(1));
		MovementValueText = Cast<UTextBlock>(Row2->GetChildAt(3));

		UHorizontalBox* Row3 = CreateStatPair(TEXT("Ranged"), TEXT("--"), TEXT("Resistance"), TEXT("--"));
		RangedValueText = Cast<UTextBlock>(Row3->GetChildAt(1));
		ResistanceValueText = Cast<UTextBlock>(Row3->GetChildAt(3));

		UHorizontalBox* Row4 = CreateStatPair(TEXT("Level"), TEXT("--"), TEXT("XP"), TEXT("--"));
		LevelValueText = Cast<UTextBlock>(Row4->GetChildAt(1));
		XPValueText = Cast<UTextBlock>(Row4->GetChildAt(3));
	}

	// -- Skills section --------------------------------------------------------
	{
		UTextBlock* SkillsHeader = WidgetTree->ConstructWidget<UTextBlock>();
		SkillsHeader->SetText(FText::FromString(TEXT("Skills")));
		SkillsHeader->SetColorAndOpacity(FSlateColor(UC::Gold));
		FSlateFontInfo Font = SkillsHeader->GetFont();
		Font.Size = 12;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		SkillsHeader->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(SkillsHeader);
		if (SlotRef) { SlotRef->SetPadding(FMargin(4, 4, 0, 1)); }

		SkillsScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
		USizeBox* SkillsSize = WidgetTree->ConstructWidget<USizeBox>();
		SkillsSize->SetMaxDesiredHeight(50.0f);
		SkillsSize->AddChild(SkillsScrollBox);

		SlotRef = ContentBox->AddChildToVerticalBox(SkillsSize);
		if (SlotRef) { SlotRef->SetPadding(FMargin(4, 0, 4, 2)); }
	}

	// -- Enchantments section --------------------------------------------------
	{
		UTextBlock* EnchHeader = WidgetTree->ConstructWidget<UTextBlock>();
		EnchHeader->SetText(FText::FromString(TEXT("Enchantments")));
		EnchHeader->SetColorAndOpacity(FSlateColor(UC::Gold));
		FSlateFontInfo Font = EnchHeader->GetFont();
		Font.Size = 12;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		EnchHeader->SetFont(Font);

		UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(EnchHeader);
		if (SlotRef) { SlotRef->SetPadding(FMargin(4, 2, 0, 1)); }

		EnchantmentsScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
		USizeBox* EnchSize = WidgetTree->ConstructWidget<USizeBox>();
		EnchSize->SetMaxDesiredHeight(50.0f);
		EnchSize->AddChild(EnchantmentsScrollBox);

		SlotRef = ContentBox->AddChildToVerticalBox(EnchSize);
		if (SlotRef) { SlotRef->SetPadding(FMargin(4, 0, 4, 4)); }
	}

	// -- Close button ----------------------------------------------------------
	CloseButton = CreateStyledButton(TEXT("Close"));
}

// =============================================================================
// Helpers
// =============================================================================

UHorizontalBox* UCoMUnitCardWidget::CreateStatPair(const FString& Label1, const FString& Value1,
                                                    const FString& Label2, const FString& Value2)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	auto AddLabel = [&](const FString& Text)
	{
		UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>();
		TB->SetText(FText::FromString(Text + TEXT(": ")));
		TB->SetColorAndOpacity(FSlateColor(UC::Gold));
		FSlateFontInfo Font = TB->GetFont();
		Font.Size = 11;
		TB->SetFont(Font);
		UHorizontalBoxSlot* SlotRef = Row->AddChildToHorizontalBox(TB);
		if (SlotRef) { SlotRef->SetPadding(FMargin(4, 1, 0, 1)); }
	};

	auto AddValue = [&](const FString& Text)
	{
		UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>();
		TB->SetText(FText::FromString(Text));
		TB->SetColorAndOpacity(FSlateColor(UC::White));
		FSlateFontInfo Font = TB->GetFont();
		Font.Size = 11;
		TB->SetFont(Font);
		UHorizontalBoxSlot* SlotRef = Row->AddChildToHorizontalBox(TB);
		if (SlotRef) { SlotRef->SetPadding(FMargin(0, 1, 12, 1)); }
	};

	AddLabel(Label1);
	AddValue(Value1);
	AddLabel(Label2);
	AddValue(Value2);

	UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(Row);
	if (SlotRef) { SlotRef->SetPadding(FMargin(4, 0)); }

	return Row;
}

UButton* UCoMUnitCardWidget::CreateStyledButton(const FString& Label)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SizeBox->SetWidthOverride(200.0f);
	SizeBox->SetHeightOverride(36.0f);

	UBorder* BtnBorder = WidgetTree->ConstructWidget<UBorder>();
	BtnBorder->SetBrushColor(UC::BtnBorder);
	BtnBorder->SetPadding(FMargin(1.0f));

	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(UC::BtnNormal);
	Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(UC::BtnHover);
	Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(UC::BtnPressed);
	Button->SetStyle(Style);

	UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetColorAndOpacity(FSlateColor(UC::Silver));
	BtnLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = BtnLabel->GetFont();
	Font.Size = 14;
	BtnLabel->SetFont(Font);

	Button->AddChild(BtnLabel);
	BtnBorder->AddChild(Button);
	SizeBox->AddChild(BtnBorder);

	UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(SizeBox);
	if (SlotRef)
	{
		SlotRef->SetHorizontalAlignment(HAlign_Center);
		SlotRef->SetPadding(FMargin(0, 4, 0, 2));
	}

	return Button;
}
