// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMItemForgeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"

#include "CoMUI/CoMUISubsystem.h"
#include "CoMCore/Items/CoMItemSubsystem.h"
#include "CoMCore/Magic/CoMMagicSubsystem.h"

#define LOCTEXT_NAMESPACE "CoMItemForge"

void UCoMForgePowerRow::HandleClick()
{
	if (Owner.IsValid())
	{
		Owner->TogglePower(PowerID);
	}
}

void UCoMItemForgeWidget::TogglePower(FName PowerID)
{
	if (PowerID.IsNone()) return;

	const int32 Idx = SelectedPowerIDs.IndexOfByKey(PowerID);
	if (Idx == INDEX_NONE)
	{
		// Adding: only allow if the resulting set is still slot-valid.
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
			{
				TArray<FCoMItemPower> Trial;
				for (const FName& ID : SelectedPowerIDs)
				{
					for (const FCoMItemPower& P : Items->GetPowerCatalog())
					{
						if (P.PowerID == ID) { Trial.Add(P); break; }
					}
				}
				for (const FCoMItemPower& P : Items->GetPowerCatalog())
				{
					if (P.PowerID == PowerID) { Trial.Add(P); break; }
				}
				if (!Items->ArePowersValidForSlot(SelectedSlot, Trial)) return;
			}
		}
		SelectedPowerIDs.Add(PowerID);
	}
	else
	{
		SelectedPowerIDs.RemoveAt(Idx);
	}
	RebuildPowerList();
	RebuildSummary();
}

namespace ForgeColours
{
	static const FLinearColor BgDark    = FLinearColor(0.055f, 0.055f, 0.102f, 1.0f);
	static const FLinearColor PanelBg   = FLinearColor(0.035f, 0.025f, 0.080f, 0.95f);
	static const FLinearColor Gold      = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
	static const FLinearColor GoldDim   = FLinearColor(0.500f, 0.380f, 0.080f, 0.5f);
	static const FLinearColor Silver    = FLinearColor(0.816f, 0.816f, 0.863f, 1.0f);
	static const FLinearColor Grey      = FLinearColor(0.500f, 0.500f, 0.550f, 1.0f);
	static const FLinearColor BtnNormal = FLinearColor(0.055f, 0.040f, 0.120f, 1.0f);
	static const FLinearColor BtnHover  = FLinearColor(0.090f, 0.060f, 0.180f, 1.0f);
	static const FLinearColor BtnPress  = FLinearColor(0.035f, 0.025f, 0.080f, 1.0f);
	static const FLinearColor SlotSel   = FLinearColor(0.220f, 0.150f, 0.030f, 1.0f);
	static const FLinearColor TileEmpty = FLinearColor(0.10f, 0.08f, 0.18f, 0.85f);
	static const FLinearColor TilePicked= FLinearColor(0.18f, 0.14f, 0.06f, 0.95f);
	static const FLinearColor RedDim    = FLinearColor(0.85f, 0.30f, 0.30f, 1.0f);
	static const FLinearColor Green     = FLinearColor(0.55f, 0.85f, 0.40f, 1.0f);
}

// =============================================================================
// Slot metadata
// =============================================================================

namespace
{
	struct FSlotMeta { ECoMItemSlot Slot; const TCHAR* Label; };

	static const FSlotMeta SlotMetaList[] = {
		{ ECoMItemSlot::Weapon,  TEXT("Weapon")  },
		{ ECoMItemSlot::Offhand, TEXT("Offhand") },
		{ ECoMItemSlot::Armor,   TEXT("Armor")   },
		{ ECoMItemSlot::Helm,    TEXT("Helm")    },
		{ ECoMItemSlot::Boots,   TEXT("Boots")   },
		{ ECoMItemSlot::Ring,    TEXT("Ring")    },
		{ ECoMItemSlot::Amulet,  TEXT("Amulet")  },
		{ ECoMItemSlot::Relic,   TEXT("Relic")   },
	};
	static_assert(UE_ARRAY_COUNT(SlotMetaList) == 8, "Slot button array sized wrong");
}

// =============================================================================
// Lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMItemForgeWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UCoMItemForgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);

	if (SlotButtons[0]) SlotButtons[0]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotWeapon);
	if (SlotButtons[1]) SlotButtons[1]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotOffhand);
	if (SlotButtons[2]) SlotButtons[2]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotArmor);
	if (SlotButtons[3]) SlotButtons[3]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotHelm);
	if (SlotButtons[4]) SlotButtons[4]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotBoots);
	if (SlotButtons[5]) SlotButtons[5]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotRing);
	if (SlotButtons[6]) SlotButtons[6]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotAmulet);
	if (SlotButtons[7]) SlotButtons[7]->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnSlotRelic);
	if (ForgeButton)    ForgeButton->OnClicked.AddDynamic(this,  &UCoMItemForgeWidget::OnForgeClicked);
	if (CancelButton)   CancelButton->OnClicked.AddDynamic(this, &UCoMItemForgeWidget::OnCancelClicked);

	SelectSlot(SelectedSlot);
}

// =============================================================================
// Public API
// =============================================================================

void UCoMItemForgeWidget::Configure(int32 InOwnerWizardIndex, bool bInArtifactMode)
{
	OwnerWizardIndex = InOwnerWizardIndex;
	bArtifactMode    = bInArtifactMode;

	if (TitleText)
	{
		TitleText->SetText(bArtifactMode
			? LOCTEXT("TitleArtifact", "Create Artifact")
			: LOCTEXT("TitleEnchant",  "Enchant Item"));
	}
	RebuildSummary();
}

// =============================================================================
// Slot select callbacks
// =============================================================================

void UCoMItemForgeWidget::OnSlotWeapon()  { SelectSlot(ECoMItemSlot::Weapon);  }
void UCoMItemForgeWidget::OnSlotOffhand() { SelectSlot(ECoMItemSlot::Offhand); }
void UCoMItemForgeWidget::OnSlotArmor()   { SelectSlot(ECoMItemSlot::Armor);   }
void UCoMItemForgeWidget::OnSlotHelm()    { SelectSlot(ECoMItemSlot::Helm);    }
void UCoMItemForgeWidget::OnSlotBoots()   { SelectSlot(ECoMItemSlot::Boots);   }
void UCoMItemForgeWidget::OnSlotRing()    { SelectSlot(ECoMItemSlot::Ring);    }
void UCoMItemForgeWidget::OnSlotAmulet()  { SelectSlot(ECoMItemSlot::Amulet);  }
void UCoMItemForgeWidget::OnSlotRelic()   { SelectSlot(ECoMItemSlot::Relic);   }

void UCoMItemForgeWidget::SelectSlot(ECoMItemSlot NewSlot)
{
	SelectedSlot = NewSlot;
	// Drop selected powers that are no longer valid for the new slot.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>())
		{
			TArray<FCoMItemPower> Picked;
			for (const FName& ID : SelectedPowerIDs)
			{
				for (const FCoMItemPower& P : Items->GetPowerCatalog())
				{
					if (P.PowerID == ID) { Picked.Add(P); break; }
				}
			}
			if (!Items->ArePowersValidForSlot(NewSlot, Picked))
			{
				// Filter to the largest valid subset by greedy retention.
				TArray<FName> Kept;
				for (int32 i = 0; i < Picked.Num(); ++i)
				{
					TArray<FCoMItemPower> Trial;
					for (const FName& ID : Kept)
					{
						for (const FCoMItemPower& Q : Items->GetPowerCatalog())
						{
							if (Q.PowerID == ID) { Trial.Add(Q); break; }
						}
					}
					Trial.Add(Picked[i]);
					if (Items->ArePowersValidForSlot(NewSlot, Trial))
					{
						Kept.Add(Picked[i].PowerID);
					}
				}
				SelectedPowerIDs = Kept;
			}
		}
	}

	// Repaint slot button highlights.
	for (int32 i = 0; i < 8; ++i)
	{
		if (!SlotButtonBorders[i]) continue;
		SlotButtonBorders[i]->SetBrushColor(SlotMetaList[i].Slot == NewSlot
			? ForgeColours::SlotSel
			: ForgeColours::GoldDim);
	}

	RebuildPowerList();
	RebuildSummary();
}

// =============================================================================
// Forge / Cancel
// =============================================================================

void UCoMItemForgeWidget::OnForgeClicked()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UCoMItemSubsystem*  Items = GI->GetSubsystem<UCoMItemSubsystem>();
	UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
	if (!Items || !Magic) return;

	TArray<FCoMItemPower> Picked;
	for (const FName& ID : SelectedPowerIDs)
	{
		for (const FCoMItemPower& P : Items->GetPowerCatalog())
		{
			if (P.PowerID == ID) { Picked.Add(P); break; }
		}
	}
	if (Picked.Num() == 0) return;

	const int32 Cost = Items->ComputeForgeCost(Picked, bArtifactMode);
	if (Magic->GetCurrentMana(OwnerWizardIndex) < Cost) return;

	const FText Display = (NameInput && !NameInput->GetText().IsEmpty())
		? NameInput->GetText()
		: (bArtifactMode ? LOCTEXT("ArtifactDefault", "Forged Artifact") : LOCTEXT("ItemDefault", "Forged Item"));

	const int32 NewID = Items->ForgeItem(OwnerWizardIndex, SelectedSlot, NAME_None, Display, Picked, bArtifactMode);
	if (NewID == 0) return;

	Magic->SpendManaForSpell(OwnerWizardIndex, ECoMSpellRealm::Arcane, Cost);

	if (UCoMUISubsystem* UI = GI->GetSubsystem<UCoMUISubsystem>())
	{
		UI->HideAllPanels();
	}
}

void UCoMItemForgeWidget::OnCancelClicked()
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
// Power list / summary builders
// =============================================================================

void UCoMItemForgeWidget::RebuildPowerList()
{
	if (!PowerListScroll) return;
	PowerListScroll->ClearChildren();
	RowHelpers.Reset();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	for (const FCoMItemPower& P : Items->GetPowerCatalog())
	{
		// Slot legality probe: this power alone in the current slot.
		TArray<FCoMItemPower> Probe; Probe.Add(P);
		if (!Items->ArePowersValidForSlot(SelectedSlot, Probe)) continue;

		const bool bChosen = SelectedPowerIDs.Contains(P.PowerID);

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>();
		RowBorder->SetBrushColor(bChosen ? ForgeColours::TilePicked : ForgeColours::TileEmpty);
		RowBorder->SetPadding(FMargin(8.0f, 6.0f));

		UButton* RowBtn = WidgetTree->ConstructWidget<UButton>();
		FButtonStyle Style = RowBtn->GetStyle();
		Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
		Style.Normal.TintColor = FSlateColor(FLinearColor(0,0,0,0));
		Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1,1,1,0.05f));
		Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0,0,0,0.20f));
		RowBtn->SetStyle(Style);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(P.DisplayName);
		Name->SetColorAndOpacity(FSlateColor(bChosen ? ForgeColours::Gold : ForgeColours::Silver));
		{ FSlateFontInfo NF = Name->GetFont(); NF.Size = 13; Name->SetFont(NF); }
		UHorizontalBoxSlot* NS = Row->AddChildToHorizontalBox(Name);
		if (NS) { NS->SetVerticalAlignment(VAlign_Center); NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		UTextBlock* Cost = WidgetTree->ConstructWidget<UTextBlock>();
		Cost->SetText(FText::Format(LOCTEXT("PowerCostFmt", "{0} mana"), FText::AsNumber(P.ManaCost)));
		Cost->SetColorAndOpacity(FSlateColor(ForgeColours::Grey));
		{ FSlateFontInfo CF = Cost->GetFont(); CF.Size = 12; Cost->SetFont(CF); }
		UHorizontalBoxSlot* CS = Row->AddChildToHorizontalBox(Cost);
		if (CS) { CS->SetVerticalAlignment(VAlign_Center); CS->SetPadding(FMargin(8, 0)); }

		RowBtn->AddChild(Row);
		RowBorder->AddChild(RowBtn);

		// Per-row helper carries the PowerID into a UFunction the dynamic delegate can call.
		UCoMForgePowerRow* Helper = NewObject<UCoMForgePowerRow>(this);
		Helper->Owner   = this;
		Helper->PowerID = P.PowerID;
		RowHelpers.Add(Helper);
		RowBtn->OnClicked.AddDynamic(Helper, &UCoMForgePowerRow::HandleClick);

		PowerListScroll->AddChild(RowBorder);
	}
}

void UCoMItemForgeWidget::RebuildSummary()
{
	if (!SelectedListScroll || !TotalCostText) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UCoMItemSubsystem* Items = GI->GetSubsystem<UCoMItemSubsystem>();
	if (!Items) return;

	SelectedListScroll->ClearChildren();
	TArray<FCoMItemPower> Picked;
	for (const FName& ID : SelectedPowerIDs)
	{
		for (const FCoMItemPower& P : Items->GetPowerCatalog())
		{
			if (P.PowerID == ID) { Picked.Add(P); break; }
		}
	}

	for (const FCoMItemPower& P : Picked)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>();
		Line->SetText(FText::Format(LOCTEXT("SelLineFmt", "  • {0}"), P.DisplayName));
		Line->SetColorAndOpacity(FSlateColor(ForgeColours::Silver));
		FSlateFontInfo F = Line->GetFont(); F.Size = 12; Line->SetFont(F);
		SelectedListScroll->AddChild(Line);
	}

	const int32 Cost = Items->ComputeForgeCost(Picked, bArtifactMode);
	UCoMMagicSubsystem* Magic = GI->GetSubsystem<UCoMMagicSubsystem>();
	const int32 Have = Magic ? Magic->GetCurrentMana(OwnerWizardIndex) : 0;
	const bool bAfford = (Have >= Cost) && Picked.Num() > 0;
	TotalCostText->SetText(FText::Format(LOCTEXT("TotalFmt", "Total: {0} mana   (You have {1})"),
		FText::AsNumber(Cost), FText::AsNumber(Have)));
	TotalCostText->SetColorAndOpacity(FSlateColor(bAfford ? ForgeColours::Green : ForgeColours::RedDim));

	if (ForgeButton)
	{
		ForgeButton->SetIsEnabled(bAfford);
	}
}

// =============================================================================
// Helper: styled button
// =============================================================================

UButton* UCoMItemForgeWidget::MakeBtn(const FString& Label, float Width)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.DrawAs    = ESlateBrushDrawType::Box;
	Style.Normal.TintColor = FSlateColor(ForgeColours::BtnNormal);
	Style.Hovered.DrawAs   = ESlateBrushDrawType::Box;
	Style.Hovered.TintColor = FSlateColor(ForgeColours::BtnHover);
	Style.Pressed.DrawAs   = ESlateBrushDrawType::Box;
	Style.Pressed.TintColor = FSlateColor(ForgeColours::BtnPress);
	Button->SetStyle(Style);

	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(Label));
	T->SetColorAndOpacity(FSlateColor(ForgeColours::Silver));
	T->SetJustification(ETextJustify::Center);
	FSlateFontInfo F = T->GetFont(); F.Size = 13; T->SetFont(F);
	Button->AddChild(T);
	return Button;
}

// =============================================================================
// Layout
// =============================================================================

void UCoMItemForgeWidget::BuildLayout()
{
	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
	BackgroundBorder->SetBrushColor(ForgeColours::BgDark);
	BackgroundBorder->SetPadding(FMargin(0.0f));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	BackgroundBorder->AddChild(Root);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(ForgeColours::PanelBg);
	Panel->SetPadding(FMargin(40.0f, 30.0f));
	UOverlaySlot* PSlot = Root->AddChildToOverlay(Panel);
	if (PSlot)
	{
		PSlot->SetHorizontalAlignment(HAlign_Fill);
		PSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Col);

	// -- Title ---------------------------------------------------------------
	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	TitleText->SetText(LOCTEXT("TitleEnchant", "Enchant Item"));
	TitleText->SetColorAndOpacity(FSlateColor(ForgeColours::Gold));
	TitleText->SetJustification(ETextJustify::Center);
	{ FSlateFontInfo F = TitleText->GetFont(); F.Size = 28; TitleText->SetFont(F); }
	UVerticalBoxSlot* TSlot = Col->AddChildToVerticalBox(TitleText);
	if (TSlot) { TSlot->SetHorizontalAlignment(HAlign_Center); TSlot->SetPadding(FMargin(0, 0, 0, 12)); }

	// -- Item name -----------------------------------------------------------
	{
		UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>();
		Lbl->SetText(LOCTEXT("NameLbl", "Item Name:"));
		Lbl->SetColorAndOpacity(FSlateColor(ForgeColours::Silver));
		{ FSlateFontInfo F = Lbl->GetFont(); F.Size = 14; Lbl->SetFont(F); }
		UHorizontalBoxSlot* LS = NameRow->AddChildToHorizontalBox(Lbl);
		if (LS) { LS->SetVerticalAlignment(VAlign_Center); LS->SetPadding(FMargin(0, 0, 8, 0)); }

		NameInput = WidgetTree->ConstructWidget<UEditableTextBox>();
		NameInput->SetHintText(LOCTEXT("NameHint", "(optional — leave blank for default)"));
		UHorizontalBoxSlot* NS = NameRow->AddChildToHorizontalBox(NameInput);
		if (NS) { NS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(NameRow);
		if (VS) { VS->SetPadding(FMargin(0, 0, 0, 12)); }
	}

	// -- Slot picker (8 buttons in a row) ------------------------------------
	{
		UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (int32 i = 0; i < 8; ++i)
		{
			USizeBox* Sz = WidgetTree->ConstructWidget<USizeBox>();
			Sz->SetWidthOverride(96.0f);
			Sz->SetHeightOverride(34.0f);

			SlotButtonBorders[i] = WidgetTree->ConstructWidget<UBorder>();
			SlotButtonBorders[i]->SetBrushColor(ForgeColours::GoldDim);
			SlotButtonBorders[i]->SetPadding(FMargin(1.0f));

			SlotButtons[i] = MakeBtn(SlotMetaList[i].Label, 94.f);
			SlotButtonBorders[i]->AddChild(SlotButtons[i]);
			Sz->AddChild(SlotButtonBorders[i]);

			UHorizontalBoxSlot* HS = SlotRow->AddChildToHorizontalBox(Sz);
			if (HS) { HS->SetPadding(FMargin(2, 0)); }
		}
		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(SlotRow);
		if (VS) { VS->SetHorizontalAlignment(HAlign_Center); VS->SetPadding(FMargin(0, 0, 0, 16)); }
	}

	// -- Two columns: power catalog (left) + selected (right) -----------------
	{
		UHorizontalBox* TwoCol = WidgetTree->ConstructWidget<UHorizontalBox>();

		// Left: catalog
		{
			UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>();
			UTextBlock* H = WidgetTree->ConstructWidget<UTextBlock>();
			H->SetText(LOCTEXT("CatalogHdr", "Available Powers"));
			H->SetColorAndOpacity(FSlateColor(ForgeColours::Gold));
			{ FSlateFontInfo F = H->GetFont(); F.Size = 16; H->SetFont(F); }
			LeftCol->AddChildToVerticalBox(H);

			PowerListScroll = WidgetTree->ConstructWidget<UScrollBox>();
			PowerListScroll->SetOrientation(Orient_Vertical);
			UVerticalBoxSlot* PS = LeftCol->AddChildToVerticalBox(PowerListScroll);
			if (PS) { PS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

			UHorizontalBoxSlot* HS = TwoCol->AddChildToHorizontalBox(LeftCol);
			if (HS) { HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); HS->SetPadding(FMargin(0, 0, 12, 0)); }
		}

		// Right: selected summary
		{
			UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>();
			UTextBlock* H = WidgetTree->ConstructWidget<UTextBlock>();
			H->SetText(LOCTEXT("SelectedHdr", "Selected Powers"));
			H->SetColorAndOpacity(FSlateColor(ForgeColours::Gold));
			{ FSlateFontInfo F = H->GetFont(); F.Size = 16; H->SetFont(F); }
			RightCol->AddChildToVerticalBox(H);

			SelectedListScroll = WidgetTree->ConstructWidget<UScrollBox>();
			SelectedListScroll->SetOrientation(Orient_Vertical);
			UVerticalBoxSlot* SS = RightCol->AddChildToVerticalBox(SelectedListScroll);
			if (SS) { SS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }

			TotalCostText = WidgetTree->ConstructWidget<UTextBlock>();
			TotalCostText->SetText(LOCTEXT("TotalDefault", "Total: 0 mana"));
			TotalCostText->SetColorAndOpacity(FSlateColor(ForgeColours::RedDim));
			{ FSlateFontInfo F = TotalCostText->GetFont(); F.Size = 14; TotalCostText->SetFont(F); }
			RightCol->AddChildToVerticalBox(TotalCostText);

			UHorizontalBoxSlot* HS = TwoCol->AddChildToHorizontalBox(RightCol);
			if (HS) { HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
		}

		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(TwoCol);
		if (VS) { VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
	}

	// -- Bottom bar ----------------------------------------------------------
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();

		USizeBox* CSize = WidgetTree->ConstructWidget<USizeBox>();
		CSize->SetWidthOverride(140.0f); CSize->SetHeightOverride(38.0f);
		CancelButton = MakeBtn(TEXT("Cancel"));
		CSize->AddChild(CancelButton);
		UHorizontalBoxSlot* CS = Bottom->AddChildToHorizontalBox(CSize);
		if (CS) { CS->SetPadding(FMargin(0, 0, 12, 0)); }

		USizeBox* FSize = WidgetTree->ConstructWidget<USizeBox>();
		FSize->SetWidthOverride(160.0f); FSize->SetHeightOverride(38.0f);
		ForgeButton = MakeBtn(TEXT("Forge"));
		FSize->AddChild(ForgeButton);
		Bottom->AddChildToHorizontalBox(FSize);

		UVerticalBoxSlot* VS = Col->AddChildToVerticalBox(Bottom);
		if (VS) { VS->SetHorizontalAlignment(HAlign_Right); VS->SetPadding(FMargin(0, 14, 0, 0)); }
	}

	if (WidgetTree)
	{
		WidgetTree->RootWidget = BackgroundBorder;
	}
}

#undef LOCTEXT_NAMESPACE
