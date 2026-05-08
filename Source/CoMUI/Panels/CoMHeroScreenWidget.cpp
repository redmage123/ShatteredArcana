// Copyright Mythforge Studios. All Rights Reserved.
// CoMHeroScreenWidget.cpp -- Hero character sheet showing base + item-folded
// stats and the equipped magical items.

#include "CoMHeroScreenWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"
#include "CoMCore/CoreTypes/CoMItemTypes.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Data/CoMUnitDatabase.h"

#define LOCTEXT_NAMESPACE "CoMHeroScreen"

namespace HeroColours
{
	static const FLinearColor BgDark    = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg   = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold      = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver    = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey      = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	static const FLinearColor TileBg    = FLinearColor(0.10f, 0.08f, 0.18f, 0.85f);
	static const FLinearColor SlotEmpty = FLinearColor(0.06f, 0.05f, 0.10f, 0.7f);
	static const FLinearColor SlotFull  = FLinearColor(0.18f, 0.13f, 0.06f, 0.95f);
	static const FLinearColor BtnNormal = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover  = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPress  = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
	static const FLinearColor BonusGood = FLinearColor(0.50f, 0.85f, 0.40f, 1.0f);
}

// =============================================================================
// Subsystem accessors
// =============================================================================

UCoMUnitSubsystem* UCoMHeroScreenWidget::GetUnitSubsystem()
{
	if (CachedUnitSubsystem.IsValid()) { return CachedUnitSubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedUnitSubsystem = GI->GetSubsystem<UCoMUnitSubsystem>();
		return CachedUnitSubsystem.Get();
	}
	return nullptr;
}

UCoMHeroSubsystem* UCoMHeroScreenWidget::GetHeroSubsystem()
{
	if (CachedHeroSubsystem.IsValid()) { return CachedHeroSubsystem.Get(); }
	if (UGameInstance* GI = GetGameInstance())
	{
		CachedHeroSubsystem = GI->GetSubsystem<UCoMHeroSubsystem>();
		return CachedHeroSubsystem.Get();
	}
	return nullptr;
}

// =============================================================================
// Lifecycle
// =============================================================================

void UCoMHeroScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
}

TSharedRef<SWidget> UCoMHeroScreenWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildHeroLayout();
	}
	return Super::RebuildWidget();
}

// =============================================================================
// Stat helpers
// =============================================================================

namespace
{
	struct FBaseStats
	{
		int32 Attack    = 0;
		int32 Ranged    = 0;
		int32 Defense   = 0;
		int32 HP        = 0;
		int32 MaxHP     = 0;
		int32 Movement  = 0;
		int32 Resist    = 0;
	};

	struct FItemBonuses
	{
		int32 Attack    = 0;
		int32 Defense   = 0;
		int32 HP        = 0;
		int32 Mana      = 0;
		int32 Movement  = 0;
		int32 Resist    = 0;
		int32 SpellSave = 0;
		TArray<FName> Skills;
	};

	FItemBonuses GatherItemBonuses(UCoMItemSubsystem* Items, int32 HeroUnitId)
	{
		FItemBonuses Out;
		if (!Items || HeroUnitId <= 0) return Out;
		const TArray<FCoMItemInstance> Equipped = Items->GetHeroEquipment(HeroUnitId);
		for (const FCoMItemInstance& Item : Equipped)
		{
			for (const FCoMItemPower& P : Item.Powers)
			{
				if (P.Type == ECoMItemPowerType::StatBonus)
				{
					if      (P.Key == TEXT("Attack"))     Out.Attack    += P.Magnitude;
					else if (P.Key == TEXT("Defense"))    Out.Defense   += P.Magnitude;
					else if (P.Key == TEXT("HP"))         Out.HP        += P.Magnitude;
					else if (P.Key == TEXT("Mana"))       Out.Mana      += P.Magnitude;
					else if (P.Key == TEXT("Movement"))   Out.Movement  += P.Magnitude;
					else if (P.Key == TEXT("Resistance")) Out.Resist    += P.Magnitude;
					else if (P.Key == TEXT("SpellSave"))  Out.SpellSave += P.Magnitude;
				}
				else if (P.Type == ECoMItemPowerType::GrantSkill)
				{
					Out.Skills.AddUnique(P.Key);
				}
			}
		}
		return Out;
	}
}

// =============================================================================
// Public API
// =============================================================================

void UCoMHeroScreenWidget::SetHero(int32 HeroUnitId)
{
	CurrentHeroUnitId = HeroUnitId;

	UCoMUnitSubsystem* UnitSub = GetUnitSubsystem();
	UCoMHeroSubsystem* HeroSub = GetHeroSubsystem();
	if (!UnitSub) return;

	const FCoMUnitInstance* Unit = UnitSub->GetUnit(HeroUnitId);
	if (!Unit) return;

	// Pull base stats from the unit database.
	FBaseStats Base;
	if (CoMUnitDatabase::Contains(Unit->SpecID))
	{
		const FCoMUnitSpecInfo& Spec = CoMUnitDatabase::GetUnitSpec(Unit->SpecID);
		Base.Attack   = Spec.MeleeAttack;
		Base.Ranged   = Spec.RangedAttack;
		Base.Defense  = Spec.Defense;
		Base.HP       = Unit->CurrentHP;
		Base.MaxHP    = Spec.HitPoints;
		Base.Movement = Spec.Movement;
		Base.Resist   = Spec.Resistance;
	}
	else
	{
		Base.HP    = Unit->CurrentHP;
		Base.MaxHP = Unit->MaxHP;
	}

	// Item bonuses (per the item subsystem).
	UCoMItemSubsystem* ItemSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCoMItemSubsystem>() : nullptr;
	const FItemBonuses Bonus = GatherItemBonuses(ItemSub, HeroUnitId);

	// -- Identity ------------------------------------------------------------
	if (HeroNameText)
	{
		HeroNameText->SetText(FText::Format(LOCTEXT("HeroNameFmt", "Hero #{0}"), FText::AsNumber(HeroUnitId)));
	}
	if (HeroClassText && HeroSub)
	{
		const ECoMHeroClass HClass = HeroSub->GetHeroClass(HeroUnitId);
		const ECoMHeroTier  HTier  = HeroSub->GetHeroTier(HeroUnitId);
		const FString Line = FString::Printf(TEXT("%s — %s"),
			*StaticEnum<ECoMHeroTier>()->GetDisplayNameTextByValue((int64)HTier).ToString(),
			*StaticEnum<ECoMHeroClass>()->GetDisplayNameTextByValue((int64)HClass).ToString());
		HeroClassText->SetText(FText::FromString(Line));
	}
	if (LevelXPText)
	{
		LevelXPText->SetText(FText::Format(LOCTEXT("LevelXPFmt", "Level {0}   ({1} XP)"),
			FText::AsNumber(Unit->Level), FText::AsNumber(Unit->Experience)));
	}
	if (LoyaltyBar && HeroSub)
	{
		const float Loyalty = (float)HeroSub->GetLoyalty(HeroUnitId).ToInt32();
		LoyaltyBar->SetPercent(FMath::Clamp(Loyalty / 100.f, 0.f, 1.f));
	}

	// -- Stat values: base (white) + bonus (gold) ----------------------------
	auto StatLine = [](int32 BaseV, int32 BonusV) -> FText
	{
		if (BonusV == 0) return FText::AsNumber(BaseV);
		return FText::FromString(FString::Printf(TEXT("%d  (+%d)"), BaseV + BonusV, BonusV));
	};
	auto StatColor = [](int32 BonusV) -> FSlateColor
	{
		return FSlateColor(BonusV > 0 ? HeroColours::BonusGood : HeroColours::Silver);
	};

	if (AttackValueText)
	{
		AttackValueText->SetText(StatLine(Base.Attack, Bonus.Attack));
		AttackValueText->SetColorAndOpacity(StatColor(Bonus.Attack));
	}
	if (RangedValueText)
	{
		// Ranged units share the +Attack bonus.
		const int32 RangedBonus = (Base.Ranged > 0) ? Bonus.Attack : 0;
		RangedValueText->SetText(StatLine(Base.Ranged, RangedBonus));
		RangedValueText->SetColorAndOpacity(StatColor(RangedBonus));
	}
	if (DefenseValueText)
	{
		DefenseValueText->SetText(StatLine(Base.Defense, Bonus.Defense));
		DefenseValueText->SetColorAndOpacity(StatColor(Bonus.Defense));
	}
	if (ResistanceValueText)
	{
		ResistanceValueText->SetText(StatLine(Base.Resist, Bonus.Resist));
		ResistanceValueText->SetColorAndOpacity(StatColor(Bonus.Resist));
	}
	if (HPValueText)
	{
		const int32 EffMax = Base.MaxHP + Bonus.HP;
		HPValueText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d%s"), Base.HP, EffMax,
			Bonus.HP > 0 ? *FString::Printf(TEXT("  (+%d)"), Bonus.HP) : TEXT(""))));
		HPValueText->SetColorAndOpacity(StatColor(Bonus.HP));
	}
	if (MovementValueText)
	{
		MovementValueText->SetText(StatLine(Base.Movement, Bonus.Movement));
		MovementValueText->SetColorAndOpacity(StatColor(Bonus.Movement));
	}

	// -- Equipment slot grid: show item names per slot -----------------------
	if (ItemSub)
	{
		const ECoMItemSlot SlotForButton[MAX_ITEM_SLOTS] = {
			ECoMItemSlot::Weapon, ECoMItemSlot::Offhand, ECoMItemSlot::Armor,
			ECoMItemSlot::Helm,   ECoMItemSlot::Boots,   ECoMItemSlot::Amulet
		};
		for (int32 i = 0; i < MAX_ITEM_SLOTS; ++i)
		{
			if (!ItemSlotLabels[i]) continue;
			FCoMItemInstance Equipped;
			if (ItemSub->GetHeroEquippedAt(HeroUnitId, SlotForButton[i], Equipped))
			{
				ItemSlotLabels[i]->SetText(Equipped.DisplayName);
				ItemSlotLabels[i]->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
			}
			else
			{
				const FString SlotName = StaticEnum<ECoMItemSlot>()
					->GetDisplayNameTextByValue((int64)SlotForButton[i]).ToString();
				ItemSlotLabels[i]->SetText(FText::FromString(FString::Printf(TEXT("[%s]"), *SlotName)));
				ItemSlotLabels[i]->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
			}
		}
	}

	// -- Skills granted by items --------------------------------------------
	if (SkillsScrollBox)
	{
		SkillsScrollBox->ClearChildren();
		if (Bonus.Skills.Num() == 0)
		{
			UTextBlock* None = WidgetTree->ConstructWidget<UTextBlock>();
			None->SetText(LOCTEXT("HeroNoSkills", "  (no item-granted skills)"));
			None->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
			{ FSlateFontInfo F = None->GetFont(); F.Size = 12; None->SetFont(F); }
			SkillsScrollBox->AddChild(None);
		}
		else
		{
			for (const FName& Skill : Bonus.Skills)
			{
				UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>();
				Line->SetText(FText::FromString(FString::Printf(TEXT("  • %s"),
					*FName::NameToDisplayString(Skill.ToString(), false))));
				Line->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
				{ FSlateFontInfo F = Line->GetFont(); F.Size = 12; Line->SetFont(F); }
				SkillsScrollBox->AddChild(Line);
			}
		}
	}
}

// =============================================================================
// Layout helpers
// =============================================================================

UButton* UCoMHeroScreenWidget::CreateStyledButton(const FString& Label, UVerticalBox* Parent)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(HeroColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(HeroColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(HeroColours::BtnPress);
	Button->SetStyle(Style);

	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Label));
	T->SetColorAndOpacity(FSlateColor(HeroColours::Silver));
	T->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = T->GetFont(); F.Size = 13; T->SetFont(F); }
	Button->AddChild(T);

	if (Parent)
	{
		Parent->AddChildToVerticalBox(Button);
	}
	return Button;
}

UHorizontalBox* UCoMHeroScreenWidget::CreateStatPair(const FString& Label1, const FString& Value1,
                                                      const FString& Label2, const FString& Value2,
                                                      UVerticalBox* Parent)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	auto AddCell = [this, Row](const FString& Lbl) -> UTextBlock*
	{
		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(FText::FromString(Lbl));
		L->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
		{ FSlateFontInfo F = L->GetFont(); F.Size = 12; L->SetFont(F); }
		UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(L);
		if (LS) { LS->SetVerticalAlignment(VAlign_Center); LS->SetPadding(FMargin(0, 0, 6, 0)); }

		UTextBlock* V = WidgetTree->ConstructWidget<UTextBlock>();
		V->SetColorAndOpacity(FSlateColor(HeroColours::Silver));
		{ FSlateFontInfo F = V->GetFont(); F.Size = 13; V->SetFont(F); }
		UHorizontalBoxSlot* VS = Row->AddChildToHorizontalBox(V);
		if (VS) { VS->SetVerticalAlignment(VAlign_Center); VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		return V;
	};

	UTextBlock* V1 = AddCell(Label1);
	V1->SetText(FText::FromString(Value1));
	UTextBlock* V2 = AddCell(Label2);
	V2->SetText(FText::FromString(Value2));

	if (Parent)
	{
		UVerticalBoxSlot* PS = Parent->AddChildToVerticalBox(Row);
		if (PS) { PS->SetPadding(FMargin(0, 2)); }
	}
	(void)V1; (void)V2;
	return Row;
}

// =============================================================================
// Layout: build the full hero screen
// =============================================================================

void UCoMHeroScreenWidget::BuildHeroLayout()
{
	OuterBorder = WidgetTree->ConstructWidget<UBorder>();
	OuterBorder->SetBrushColor(HeroColours::BgDark);
	OuterBorder->SetPadding(FMargin(0));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	OuterBorder->AddChild(Root);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(HeroColours::PanelBg);
	Panel->SetPadding(FMargin(36, 24));
	UOverlaySlot* PSlot = Root->AddChildToOverlay(Panel);
	if (PSlot) { PSlot->SetHorizontalAlignment(HAlign_Fill); PSlot->SetVerticalAlignment(VAlign_Fill); }

	UVerticalBox* OuterCol = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(OuterCol);

	// Title row
	{
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
		HeroNameText = WidgetTree->ConstructWidget<UTextBlock>();
		HeroNameText->SetText(LOCTEXT("HeroDefault", "Hero"));
		HeroNameText->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
		{ FSlateFontInfo F = HeroNameText->GetFont(); F.Size = 26; HeroNameText->SetFont(F); }
		UHorizontalBoxSlot* HS = Top->AddChildToHorizontalBox(HeroNameText);
		if (HS) { HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); HS->SetVerticalAlignment(VAlign_Center); }

		USizeBox* CSize = WidgetTree->ConstructWidget<USizeBox>();
		CSize->SetWidthOverride(120); CSize->SetHeightOverride(34);
		CloseButton = CreateStyledButton(TEXT("Close"), nullptr);
		CSize->AddChild(CloseButton);
		Top->AddChildToHorizontalBox(CSize);

		UVerticalBoxSlot* TS = OuterCol->AddChildToVerticalBox(Top);
		if (TS) { TS->SetPadding(FMargin(0, 0, 0, 8)); }
	}

	HeroClassText = WidgetTree->ConstructWidget<UTextBlock>();
	HeroClassText->SetText(LOCTEXT("HeroClassDefault", ""));
	HeroClassText->SetColorAndOpacity(FSlateColor(HeroColours::Silver));
	{ FSlateFontInfo F = HeroClassText->GetFont(); F.Size = 14; HeroClassText->SetFont(F); }
	OuterCol->AddChildToVerticalBox(HeroClassText);

	// Two-column body
	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
	{
		UVerticalBoxSlot* BS = OuterCol->AddChildToVerticalBox(Body);
		if (BS) { BS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); BS->SetPadding(FMargin(0, 16, 0, 0)); }
	}

	// LEFT — identity / level / loyalty
	{
		UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();

		LevelXPText = WidgetTree->ConstructWidget<UTextBlock>();
		LevelXPText->SetText(LOCTEXT("LevelDefault", "Level 1"));
		LevelXPText->SetColorAndOpacity(FSlateColor(HeroColours::Silver));
		{ FSlateFontInfo F = LevelXPText->GetFont(); F.Size = 14; LevelXPText->SetFont(F); }
		Left->AddChildToVerticalBox(LevelXPText);

		LoyaltyLabel = WidgetTree->ConstructWidget<UTextBlock>();
		LoyaltyLabel->SetText(LOCTEXT("LoyaltyLbl", "Loyalty"));
		LoyaltyLabel->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
		{ FSlateFontInfo F = LoyaltyLabel->GetFont(); F.Size = 12; LoyaltyLabel->SetFont(F); }
		UVerticalBoxSlot* LS = Left->AddChildToVerticalBox(LoyaltyLabel);
		if (LS) { LS->SetPadding(FMargin(0, 8, 0, 2)); }

		LoyaltyBar = WidgetTree->ConstructWidget<UProgressBar>();
		LoyaltyBar->SetPercent(1.0f);
		LoyaltyBar->SetFillColorAndOpacity(HeroColours::BonusGood);
		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
		BarSize->SetHeightOverride(8.0f);
		BarSize->AddChild(LoyaltyBar);
		Left->AddChildToVerticalBox(BarSize);

		// Skills granted by items
		UTextBlock* Hdr = WidgetTree->ConstructWidget<UTextBlock>();
		Hdr->SetText(LOCTEXT("HeroSkillsHdr", "Item-Granted Skills"));
		Hdr->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
		{ FSlateFontInfo F = Hdr->GetFont(); F.Size = 14; Hdr->SetFont(F); }
		UVerticalBoxSlot* HS = Left->AddChildToVerticalBox(Hdr);
		if (HS) { HS->SetPadding(FMargin(0, 16, 0, 4)); }

		SkillsScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
		SkillsScrollBox->SetOrientation(Orient_Vertical);
		UVerticalBoxSlot* SS = Left->AddChildToVerticalBox(SkillsScrollBox);
		if (SS) { SS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		UHorizontalBoxSlot* HBS = Body->AddChildToHorizontalBox(Left);
		if (HBS) { HBS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); HBS->SetPadding(FMargin(0, 0, 24, 0)); }
	}

	// RIGHT — stats + equipment
	{
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();

		// Stats header
		UTextBlock* StatsHdr = WidgetTree->ConstructWidget<UTextBlock>();
		StatsHdr->SetText(LOCTEXT("HeroStatsHdr", "Combat Stats"));
		StatsHdr->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
		{ FSlateFontInfo F = StatsHdr->GetFont(); F.Size = 14; StatsHdr->SetFont(F); }
		UVerticalBoxSlot* SH = Right->AddChildToVerticalBox(StatsHdr);
		if (SH) { SH->SetPadding(FMargin(0, 0, 0, 6)); }

		// Helper: stat row [label][value] — returns the value text-block so the
		// caller can store it on the widget.
		auto StatRow = [this, Right](const FText& Label) -> UTextBlock*
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
			L->SetText(Label);
			L->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
			{ FSlateFontInfo F = L->GetFont(); F.Size = 12; L->SetFont(F); }
			UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(L);
			if (LS) { LS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

			UTextBlock* V = WidgetTree->ConstructWidget<UTextBlock>();
			V->SetText(FText::FromString(TEXT("-")));
			V->SetColorAndOpacity(FSlateColor(HeroColours::Silver));
			{ FSlateFontInfo F = V->GetFont(); F.Size = 13; V->SetFont(F); }
			UHorizontalBoxSlot* VS = Row->AddChildToHorizontalBox(V);
			if (VS) { VS->SetPadding(FMargin(8, 0, 0, 0)); }

			UVerticalBoxSlot* RS = Right->AddChildToVerticalBox(Row);
			if (RS) { RS->SetPadding(FMargin(0, 1)); }
			return V;
		};

		AttackValueText     = StatRow(LOCTEXT("HeroAtk",  "Melee Attack"));
		RangedValueText     = StatRow(LOCTEXT("HeroRng",  "Ranged Attack"));
		DefenseValueText    = StatRow(LOCTEXT("HeroDef",  "Defense"));
		ResistanceValueText = StatRow(LOCTEXT("HeroRes",  "Resistance"));
		HPValueText         = StatRow(LOCTEXT("HeroHP",   "Hit Points"));
		MovementValueText   = StatRow(LOCTEXT("HeroMov",  "Movement"));

		// Equipment header
		UTextBlock* EquipHdr = WidgetTree->ConstructWidget<UTextBlock>();
		EquipHdr->SetText(LOCTEXT("HeroEquipHdr", "Equipment"));
		EquipHdr->SetColorAndOpacity(FSlateColor(HeroColours::Gold));
		{ FSlateFontInfo F = EquipHdr->GetFont(); F.Size = 14; EquipHdr->SetFont(F); }
		UVerticalBoxSlot* EH = Right->AddChildToVerticalBox(EquipHdr);
		if (EH) { EH->SetPadding(FMargin(0, 16, 0, 6)); }

		// 6 slot buttons in a 2x3 grid (built as 2 rows of 3).
		{
			for (int32 RowIdx = 0; RowIdx < 2; ++RowIdx)
			{
				UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>();
				for (int32 ColIdx = 0; ColIdx < 3; ++ColIdx)
				{
					const int32 SlotIdx = RowIdx * 3 + ColIdx;
					if (SlotIdx >= MAX_ITEM_SLOTS) break;

					USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>();
					Sz->SetWidthOverride(170); Sz->SetHeightOverride(36);

					ItemSlotButtons[SlotIdx] = WidgetTree->ConstructWidget<UButton>();
					FButtonStyle Style = ItemSlotButtons[SlotIdx]->GetStyle();
					Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
					Style.Normal.TintColor = FSlateColor(HeroColours::SlotEmpty);
					Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
					Style.Hovered.TintColor = FSlateColor(HeroColours::SlotFull);
					Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
					Style.Pressed.TintColor = FSlateColor(HeroColours::BtnPress);
					ItemSlotButtons[SlotIdx]->SetStyle(Style);

					ItemSlotLabels[SlotIdx] = WidgetTree->ConstructWidget<UTextBlock>();
					ItemSlotLabels[SlotIdx]->SetText(FText::FromString(TEXT("[empty]")));
					ItemSlotLabels[SlotIdx]->SetColorAndOpacity(FSlateColor(HeroColours::Grey));
					ItemSlotLabels[SlotIdx]->SetJustification(ETextJustify::Center);
					{ FSlateFontInfo F = ItemSlotLabels[SlotIdx]->GetFont(); F.Size = 12; ItemSlotLabels[SlotIdx]->SetFont(F); }
					ItemSlotButtons[SlotIdx]->AddChild(ItemSlotLabels[SlotIdx]);
					Sz->AddChild(ItemSlotButtons[SlotIdx]);

					UHorizontalBoxSlot* CS = RowBox->AddChildToHorizontalBox(Sz);
					if (CS) { CS->SetPadding(FMargin(2)); }
				}
				UVerticalBoxSlot* RS2 = Right->AddChildToVerticalBox(RowBox);
				if (RS2) { RS2->SetPadding(FMargin(0, 2)); }
			}
		}

		// Bind slot click handlers
		if (ItemSlotButtons[0]) ItemSlotButtons[0]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem0Clicked);
		if (ItemSlotButtons[1]) ItemSlotButtons[1]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem1Clicked);
		if (ItemSlotButtons[2]) ItemSlotButtons[2]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem2Clicked);
		if (ItemSlotButtons[3]) ItemSlotButtons[3]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem3Clicked);
		if (ItemSlotButtons[4]) ItemSlotButtons[4]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem4Clicked);
		if (ItemSlotButtons[5]) ItemSlotButtons[5]->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnItem5Clicked);

		UHorizontalBoxSlot* HBS = Body->AddChildToHorizontalBox(Right);
		if (HBS) { HBS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
	}

	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMHeroScreenWidget::OnCloseClicked); }

	if (WidgetTree)
	{
		WidgetTree->RootWidget = OuterBorder;
	}
}

void UCoMHeroScreenWidget::BuildLeftColumn(UVerticalBox* /*LeftBox*/) {}
void UCoMHeroScreenWidget::BuildRightColumn(UVerticalBox* /*RightBox*/) {}

// =============================================================================
// Button callbacks
// =============================================================================

void UCoMHeroScreenWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideHeroScreen();
		}
	}
}

// All six slot clicks open the vault for the current hero.
static void OpenVaultForCurrentHero(UCoMHeroScreenWidget* Self, int32 HeroUnitId)
{
	if (HeroUnitId <= 0) return;
	UGameInstance* GI = Self->GetGameInstance();
	if (!GI) return;

	int32 OwnerWizard = 0;
	if (UCoMHeroSubsystem* Heroes = GI->GetSubsystem<UCoMHeroSubsystem>())
	{
		const int32 Found = Heroes->GetHeroOwner(HeroUnitId);
		if (Found >= 0) OwnerWizard = Found;
	}
	if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
	{
		UI->ShowItemVault(OwnerWizard, HeroUnitId);
	}
}

void UCoMHeroScreenWidget::OnItem0Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }
void UCoMHeroScreenWidget::OnItem1Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }
void UCoMHeroScreenWidget::OnItem2Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }
void UCoMHeroScreenWidget::OnItem3Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }
void UCoMHeroScreenWidget::OnItem4Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }
void UCoMHeroScreenWidget::OnItem5Clicked() { OpenVaultForCurrentHero(this, CurrentHeroUnitId); }

#undef LOCTEXT_NAMESPACE
