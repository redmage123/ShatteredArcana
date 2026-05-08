// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMItemVaultWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"

#define LOCTEXT_NAMESPACE "CoMItemVault"

namespace VaultColours
{
	static const FLinearColor BgDark    = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg   = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold      = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor Silver    = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey      = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	static const FLinearColor TileEmpty = FLinearColor(0.10f, 0.08f, 0.18f, 0.85f);
	static const FLinearColor TileEquip = FLinearColor(0.16f, 0.13f, 0.22f, 0.95f);
	static const FLinearColor BtnNormal = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover  = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPress  = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
}

void UCoMVaultItemRow::HandleClick()
{
	if (Owner.IsValid())
	{
		Owner->HandleItemClicked(InstanceID);
	}
}

// =============================================================================
// Lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMItemVaultWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMItemVaultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (CloseButton) { CloseButton->OnClicked.AddDynamic(this, &UCoMItemVaultWidget::OnCloseClicked); }
}

// =============================================================================
// Public API
// =============================================================================

void UCoMItemVaultWidget::Configure(int32 InOwnerWizardIndex)
{
	OwnerWizardIndex = InOwnerWizardIndex;
	RebuildList();
}

void UCoMItemVaultWidget::SetTargetHero(int32 HeroUnitID)
{
	TargetHeroUnitID = HeroUnitID;
	if (SubtitleText)
	{
		SubtitleText->SetText(HeroUnitID == 0
			? LOCTEXT("VaultPreview", "Preview-only — open from a hero screen to equip")
			: FText::Format(LOCTEXT("VaultEquipFor", "Click an item to equip on Hero #{0}"), FText::AsNumber(HeroUnitID)));
	}
	RebuildList();
}

void UCoMItemVaultWidget::HandleItemClicked(int32 InstanceID)
{
	if (TargetHeroUnitID == 0) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	Items->EquipItem(TargetHeroUnitID, InstanceID);
	RebuildList();
}

void UCoMItemVaultWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
		{
			UI->HideAllPanels();
		}
	}
}

// =============================================================================
// List builder
// =============================================================================

void UCoMItemVaultWidget::RebuildList()
{
	if (!ItemListScroll) return;
	ItemListScroll->ClearChildren();
	RowHelpers.Reset();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	const TArray<FCoMItemInstance> Owned = Items->GetItemsForWizard(OwnerWizardIndex);
	if (Owned.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>();
		Empty->SetText(LOCTEXT("VaultEmpty", "Vault is empty. Cast Enchant Item or Create Artifact to forge magical items."));
		Empty->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
		{ FSlateFontInfo F = Empty->GetFont(); F.Size = 14; Empty->SetFont(F); }
		ItemListScroll->AddChild(Empty);
		return;
	}

	for (const FCoMItemInstance& Item : Owned)
	{
		const bool bEquipped = (Item.EquippedByHeroID != 0);

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(bEquipped ? VaultColours::TileEquip : VaultColours::TileEmpty);
		RowBorder->SetPadding(FMargin(8.0f, 8.0f));

		UButton* RowBtn = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = RowBtn->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(FLinearColor(0,0,0,0));
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1,1,1,0.05f));
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0,0,0,0.20f));
		RowBtn->SetStyle(Style);

		UVerticalBox* RowCol = WidgetTree->ConstructWidget<UVerticalBox>();

		// Header: name + slot
		{
			UHorizontalBox* HdrRow = WidgetTree->ConstructWidget<UHorizontalBox>();

			UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
			Name->SetText(Item.DisplayName);
			Name->SetColorAndOpacity(FSlateColor(Item.bArtifact ? VaultColours::Gold : VaultColours::Silver));
			{ FSlateFontInfo F = Name->GetFont(); F.Size = 15; Name->SetFont(F); }
			UHorizontalBoxSlot* HS = HdrRow->AddChildToHorizontalBox(Name);
			if (HS) { HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); HS->SetVerticalAlignment(VAlign_Center); }

			UTextBlock* SlotTag = WidgetTree->ConstructWidget<UTextBlock>();
			SlotTag->SetText(FText::Format(LOCTEXT("VaultSlotFmt", "[{0}]"),
				FText::FromString(StaticEnum<ECoMItemSlot>()->GetDisplayNameTextByValue((int64)Item.Slot).ToString())));
			SlotTag->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
			{ FSlateFontInfo F = SlotTag->GetFont(); F.Size = 12; SlotTag->SetFont(F); }
			UHorizontalBoxSlot* SS = HdrRow->AddChildToHorizontalBox(SlotTag);
			if (SS) { SS->SetVerticalAlignment(VAlign_Center); SS->SetPadding(FMargin(8, 0, 0, 0)); }

			RowCol->AddChildToVerticalBox(HdrRow);
		}

		// Powers list
		for (const FCoMItemPower& P : Item.Powers)
		{
			UTextBlock* Pow = WidgetTree->ConstructWidget<UTextBlock>();
			Pow->SetText(FText::Format(LOCTEXT("VaultPowFmt", "  • {0}"), P.DisplayName));
			Pow->SetColorAndOpacity(FSlateColor(VaultColours::Silver));
			{ FSlateFontInfo F = Pow->GetFont(); F.Size = 12; Pow->SetFont(F); }
			RowCol->AddChildToVerticalBox(Pow);
		}

		// Footer: cost + equipped status
		{
			UTextBlock* Foot = WidgetTree->ConstructWidget<UTextBlock>();
			const FString FootStr = bEquipped
				? FString::Printf(TEXT("%d mana   |   Equipped on Hero #%d"), Item.TotalManaCost, Item.EquippedByHeroID)
				: FString::Printf(TEXT("%d mana   |   In Vault"), Item.TotalManaCost);
			Foot->SetText(FText::FromString(FootStr));
			Foot->SetColorAndOpacity(FSlateColor(VaultColours::Gold));
			{ FSlateFontInfo F = Foot->GetFont(); F.Size = 11; Foot->SetFont(F); }
			RowCol->AddChildToVerticalBox(Foot);
		}

		RowBtn->AddChild(RowCol);
		RowBorder->AddChild(RowBtn);

		// Per-row click helper
		UCoMVaultItemRow* Helper = NewObject<UCoMVaultItemRow>(this);
		Helper->Owner      = this;
		Helper->InstanceID = Item.InstanceID;
		RowHelpers.Add(Helper);
		RowBtn->OnClicked.AddDynamic(Helper, &UCoMVaultItemRow::HandleClick);

		ItemListScroll->AddChild(RowBorder);
	}
}

// =============================================================================
// Layout
// =============================================================================

void UCoMItemVaultWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(VaultColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(Root);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(VaultColours::PanelBg);
	Panel->SetPadding(FMargin(40.0f, 30.0f));
	UOverlaySlot* PSlot = Root->AddChildToOverlay(Panel);
	if (PSlot)
	{
		PSlot->SetHorizontalAlignment(HAlign_Fill);
		PSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Col);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("VaultTitle", "Magical Item Vault"));
	TitleText->SetColorAndOpacity(FSlateColor(VaultColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 28; TitleText->SetFont(F); }
	UVerticalBoxSlot* TS = Col->AddChildToVerticalBox(TitleText);
	if (TS) { TS->SetHorizontalAlignment(HAlign_Center); TS->SetPadding(FMargin(0, 0, 0, 8)); }

	SubtitleText = WidgetTree->ConstructWidget<UTextBlock>();
	SubtitleText->SetText(LOCTEXT("VaultPreview", "Preview-only — open from a hero screen to equip"));
	SubtitleText->SetColorAndOpacity(FSlateColor(VaultColours::Grey));
	SubtitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = SubtitleText->GetFont(); F.Size = 13; SubtitleText->SetFont(F); }
	UVerticalBoxSlot* SS = Col->AddChildToVerticalBox(SubtitleText);
	if (SS) { SS->SetHorizontalAlignment(HAlign_Center); SS->SetPadding(FMargin(0, 0, 0, 16)); }

	ItemListScroll = WidgetTree->ConstructWidget<UScrollBox>();
	ItemListScroll->SetOrientation(Orient_Vertical);
	UVerticalBoxSlot* IS = Col->AddChildToVerticalBox(ItemListScroll);
	if (IS) { IS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

	// Bottom: close
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>();
		Sz->SetWidthOverride(140.0f); Sz->SetHeightOverride(38.0f);

		CloseButton = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = CloseButton->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(VaultColours::BtnNormal);
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(VaultColours::BtnHover);
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(VaultColours::BtnPress);
		CloseButton->SetStyle(Style);

		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>();
		Lbl->SetText(LOCTEXT("VaultClose", "Close"));
		Lbl->SetColorAndOpacity(FSlateColor(VaultColours::Silver));
		Lbl->SetJustification(ETextJustify::Center);
		{ FSlateFontInfo F = Lbl->GetFont(); F.Size = 13; Lbl->SetFont(F); }
		CloseButton->AddChild(Lbl);
		Sz->AddChild(CloseButton);
		Bottom->AddChildToHorizontalBox(Sz);

		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(Bottom);
		if (VS) { VS->SetHorizontalAlignment(HAlign_Right); VS->SetPadding(FMargin(0, 14, 0, 0)); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}
}

#undef LOCTEXT_NAMESPACE
