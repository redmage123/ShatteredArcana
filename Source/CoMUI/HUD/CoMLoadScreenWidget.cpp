// Copyright Mythforge Studios. All Rights Reserved.
// CoMLoadScreenWidget.cpp -- Load screen implementation.

#include "CoMLoadScreenWidget.h"

#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"

#include "CoMUISubsystem.h"
#include "CoMCore/Save/CoMSaveSubsystem.h"

namespace LoadScreenColours
{
    static const FLinearColor Background     = FLinearColor(0.020f, 0.020f, 0.063f, 1.0f);
    static const FLinearColor Gold           = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
    static const FLinearColor White          = FLinearColor::White;
    static const FLinearColor Grey           = FLinearColor(0.400f, 0.400f, 0.400f, 1.0f);
    static const FLinearColor LightPurple    = FLinearColor(0.627f, 0.565f, 0.753f, 1.0f);
    static const FLinearColor ButtonBg       = FLinearColor(0.086f, 0.129f, 0.243f, 1.0f);
    static const FLinearColor ButtonHover    = FLinearColor(0.102f, 0.165f, 0.306f, 1.0f);
    static const FLinearColor RowBg          = FLinearColor(0.050f, 0.050f, 0.100f, 1.0f);
    static const FLinearColor DangerRed      = FLinearColor(0.800f, 0.200f, 0.200f, 1.0f);
    static const FLinearColor DangerRedHover = FLinearColor(0.900f, 0.300f, 0.300f, 1.0f);
}

// =============================================================================
// UCoMSlotButtonHelper
// =============================================================================

void UCoMSlotButtonHelper::OnClicked()
{
    UCoMLoadScreenWidget* LoadScreen = Cast<UCoMLoadScreenWidget>(OwnerWidget.Get());
    if (!LoadScreen) { return; }

    if (bIsDeleteAction)
    {
        LoadScreen->HandleDeleteSlot(SlotName);
    }
    else
    {
        LoadScreen->HandleLoadSlot(SlotName);
    }
}

// =============================================================================
// Lifecycle
// =============================================================================

TSharedRef<SWidget> UCoMLoadScreenWidget::RebuildWidget()
{
    BuildLayout();
    return Super::RebuildWidget();
}

void UCoMLoadScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::Visible);
    RefreshSlotList();
}

// =============================================================================
// Layout
// =============================================================================

void UCoMLoadScreenWidget::BuildLayout()
{
    BackgroundBorder = WidgetTree->ConstructWidget<UBorder>();
    BackgroundBorder->SetBrushColor(LoadScreenColours::Background);
    BackgroundBorder->SetPadding(FMargin(0.0f));

    if (WidgetTree)
    {
        WidgetTree->RootWidget = BackgroundBorder;
    }

    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
    BackgroundBorder->AddChild(RootOverlay);

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>();
    UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox);
    if (ContentSlot)
    {
        ContentSlot->SetHorizontalAlignment(HAlign_Center);
        ContentSlot->SetVerticalAlignment(VAlign_Center);
    }

    // Title
    {
        UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>();
        TitleText->SetText(FText::FromString(TEXT("LOAD GAME")));
        TitleText->SetColorAndOpacity(FSlateColor(LoadScreenColours::Gold));
        TitleText->SetJustification(ETextJustify::Center);

        FSlateFontInfo TitleFont = TitleText->GetFont();
        TitleFont.Size = 36;
        TitleFont.TypefaceFontName = FName(TEXT("Bold"));
        TitleText->SetFont(TitleFont);

        UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(TitleText);
        if (SlotRef)
        {
            SlotRef->SetHorizontalAlignment(HAlign_Center);
            SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
        }
    }

    // Scrollable slot list
    SlotScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    {
        USizeBox* ScrollSizeBox = WidgetTree->ConstructWidget<USizeBox>();
        ScrollSizeBox->SetWidthOverride(600.0f);
        ScrollSizeBox->SetHeightOverride(400.0f);
        ScrollSizeBox->AddChild(SlotScrollBox);

        UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(ScrollSizeBox);
        if (SlotRef)
        {
            SlotRef->SetHorizontalAlignment(HAlign_Center);
            SlotRef->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
        }
    }

    // Back button
    {
        USizeBox* BackSizeBox = WidgetTree->ConstructWidget<USizeBox>();
        BackSizeBox->SetWidthOverride(200.0f);
        BackSizeBox->SetHeightOverride(45.0f);

        BackButton = WidgetTree->ConstructWidget<UButton>();

        // Style the back button (same as CreateActionButton but using WidgetTree)
        FButtonStyle Style = BackButton->GetStyle();
        Style.Normal.DrawAs = ESlateBrushDrawType::Box;
        Style.Normal.TintColor = FSlateColor(LoadScreenColours::ButtonBg);
        Style.Normal.OutlineSettings.Color = FSlateColor(LoadScreenColours::Gold);
        Style.Normal.OutlineSettings.Width = 1.0f;
        Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
        Style.Hovered.TintColor = FSlateColor(LoadScreenColours::ButtonHover);
        Style.Hovered.OutlineSettings.Color = FSlateColor(LoadScreenColours::Gold);
        Style.Hovered.OutlineSettings.Width = 1.0f;
        Style.Pressed = Style.Normal;
        BackButton->SetStyle(Style);

        UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
        BackLabel->SetText(FText::FromString(TEXT("Back")));
        BackLabel->SetColorAndOpacity(FSlateColor(LoadScreenColours::White));
        BackLabel->SetJustification(ETextJustify::Center);
        FSlateFontInfo BtnFont = BackLabel->GetFont();
        BtnFont.Size = 14;
        BackLabel->SetFont(BtnFont);
        BackButton->AddChild(BackLabel);

        BackButton->OnClicked.AddDynamic(this, &UCoMLoadScreenWidget::OnBackClicked);
        BackSizeBox->AddChild(BackButton);

        UVerticalBoxSlot* SlotRef = ContentBox->AddChildToVerticalBox(BackSizeBox);
        if (SlotRef)
        {
            SlotRef->SetHorizontalAlignment(HAlign_Center);
        }
    }
}

void UCoMLoadScreenWidget::RefreshSlotList()
{
    if (!SlotScrollBox) { return; }

    SlotScrollBox->ClearChildren();
    ButtonHelpers.Empty();

    UGameInstance* GI = GetGameInstance();
    if (!GI) { return; }

    UCoMSaveSubsystem* SaveSub = GI->GetSubsystem<UCoMSaveSubsystem>();
    if (!SaveSub) { return; }

    TArray<FCoMSaveSlotInfo> Slots = SaveSub->GetSaveSlots();

    if (Slots.Num() == 0)
    {
        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("No saved games found.")));
        EmptyText->SetColorAndOpacity(FSlateColor(LoadScreenColours::Grey));
        EmptyText->SetJustification(ETextJustify::Center);
        FSlateFontInfo Font = EmptyText->GetFont();
        Font.Size = 16;
        EmptyText->SetFont(Font);
        SlotScrollBox->AddChild(EmptyText);
        return;
    }

    for (const FCoMSaveSlotInfo& Info : Slots)
    {
        CreateSlotRow(Info);
    }
}

void UCoMLoadScreenWidget::CreateSlotRow(const FCoMSaveSlotInfo& Info)
{
    UBorder* RowBorder = NewObject<UBorder>(this);
    RowBorder->SetBrushColor(LoadScreenColours::RowBg);
    RowBorder->SetPadding(FMargin(10.0f, 8.0f));

    UHorizontalBox* RowBox = NewObject<UHorizontalBox>(this);
    RowBorder->AddChild(RowBox);

    // Slot info text
    {
        UVerticalBox* InfoBox = NewObject<UVerticalBox>(this);

        UTextBlock* NameText = NewObject<UTextBlock>(this);
        FString DisplayName = Info.WizardName.IsEmpty() ? Info.SlotName : Info.WizardName;
        if (Info.bIsAutosave)
        {
            DisplayName = FString::Printf(TEXT("[Auto] %s"), *DisplayName);
        }
        NameText->SetText(FText::FromString(DisplayName));
        NameText->SetColorAndOpacity(FSlateColor(LoadScreenColours::Gold));
        FSlateFontInfo NameFont = NameText->GetFont();
        NameFont.Size = 16;
        NameText->SetFont(NameFont);
        InfoBox->AddChildToVerticalBox(NameText);

        UTextBlock* DetailText = NewObject<UTextBlock>(this);
        FString DateStr = Info.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M"));
        DetailText->SetText(FText::FromString(
            FString::Printf(TEXT("Turn %d  |  %s  |  %s"), Info.TurnNumber, *Info.SlotName, *DateStr)));
        DetailText->SetColorAndOpacity(FSlateColor(LoadScreenColours::LightPurple));
        FSlateFontInfo DetailFont = DetailText->GetFont();
        DetailFont.Size = 12;
        DetailText->SetFont(DetailFont);
        InfoBox->AddChildToVerticalBox(DetailText);

        UHorizontalBoxSlot* InfoSlot = RowBox->AddChildToHorizontalBox(InfoBox);
        if (InfoSlot)
        {
            InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // Load button
    {
        UButton* LoadBtn = CreateActionButton(TEXT("Load"),
                                              LoadScreenColours::ButtonBg,
                                              LoadScreenColours::ButtonHover);

        UCoMSlotButtonHelper* Helper = NewObject<UCoMSlotButtonHelper>(this);
        Helper->SlotName = Info.SlotName;
        Helper->OwnerWidget = this;
        Helper->bIsDeleteAction = false;
        ButtonHelpers.Add(Helper);

        LoadBtn->OnClicked.AddDynamic(Helper, &UCoMSlotButtonHelper::OnClicked);

        USizeBox* LoadSizeBox = NewObject<USizeBox>(this);
        LoadSizeBox->SetWidthOverride(70.0f);
        LoadSizeBox->SetHeightOverride(35.0f);
        LoadSizeBox->AddChild(LoadBtn);

        UHorizontalBoxSlot* LoadSlot = RowBox->AddChildToHorizontalBox(LoadSizeBox);
        if (LoadSlot)
        {
            LoadSlot->SetPadding(FMargin(5.0f, 0.0f));
            LoadSlot->SetVerticalAlignment(VAlign_Center);
        }
    }

    // Delete button
    {
        UButton* DeleteBtn = CreateActionButton(TEXT("Delete"),
                                                LoadScreenColours::DangerRed,
                                                LoadScreenColours::DangerRedHover);

        UCoMSlotButtonHelper* Helper = NewObject<UCoMSlotButtonHelper>(this);
        Helper->SlotName = Info.SlotName;
        Helper->OwnerWidget = this;
        Helper->bIsDeleteAction = true;
        ButtonHelpers.Add(Helper);

        DeleteBtn->OnClicked.AddDynamic(Helper, &UCoMSlotButtonHelper::OnClicked);

        USizeBox* DeleteSizeBox = NewObject<USizeBox>(this);
        DeleteSizeBox->SetWidthOverride(80.0f);
        DeleteSizeBox->SetHeightOverride(35.0f);
        DeleteSizeBox->AddChild(DeleteBtn);

        UHorizontalBoxSlot* DeleteSlot = RowBox->AddChildToHorizontalBox(DeleteSizeBox);
        if (DeleteSlot)
        {
            DeleteSlot->SetPadding(FMargin(5.0f, 0.0f));
            DeleteSlot->SetVerticalAlignment(VAlign_Center);
        }
    }

    SlotScrollBox->AddChild(RowBorder);
}

UButton* UCoMLoadScreenWidget::CreateActionButton(const FString& Label,
                                                    const FLinearColor& NormalColor,
                                                    const FLinearColor& HoverColor)
{
    UButton* Button = NewObject<UButton>(this);

    FButtonStyle Style = Button->GetStyle();
    Style.Normal.DrawAs = ESlateBrushDrawType::Box;
    Style.Normal.TintColor = FSlateColor(NormalColor);
    Style.Normal.OutlineSettings.Color = FSlateColor(LoadScreenColours::Gold);
    Style.Normal.OutlineSettings.Width = 1.0f;
    Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
    Style.Hovered.TintColor = FSlateColor(HoverColor);
    Style.Hovered.OutlineSettings.Color = FSlateColor(LoadScreenColours::Gold);
    Style.Hovered.OutlineSettings.Width = 1.0f;
    Style.Pressed = Style.Normal;
    Button->SetStyle(Style);

    UTextBlock* ButtonLabel = NewObject<UTextBlock>(this);
    ButtonLabel->SetText(FText::FromString(Label));
    ButtonLabel->SetColorAndOpacity(FSlateColor(LoadScreenColours::White));
    ButtonLabel->SetJustification(ETextJustify::Center);
    FSlateFontInfo BtnFont = ButtonLabel->GetFont();
    BtnFont.Size = 14;
    ButtonLabel->SetFont(BtnFont);
    Button->AddChild(ButtonLabel);

    return Button;
}

// =============================================================================
// Callbacks
// =============================================================================

void UCoMLoadScreenWidget::OnBackClicked()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UCoMUISubsystem* UISS = GI->GetSubsystem<UCoMUISubsystem>())
        {
            UISS->HideLoadScreen();
            UISS->ShowMainMenu();
        }
    }
}

void UCoMLoadScreenWidget::HandleLoadSlot(const FString& SlotName)
{
    UE_LOG(LogTemp, Log, TEXT("CoMLoadScreenWidget: Loading slot '%s'..."), *SlotName);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UCoMSaveSubsystem* SaveSub = GI->GetSubsystem<UCoMSaveSubsystem>())
        {
            SaveSub->LoadGame(SlotName);
        }
    }
}

void UCoMLoadScreenWidget::HandleDeleteSlot(const FString& SlotName)
{
    UE_LOG(LogTemp, Log, TEXT("CoMLoadScreenWidget: Deleting slot '%s'..."), *SlotName);

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UCoMSaveSubsystem* SaveSub = GI->GetSubsystem<UCoMSaveSubsystem>())
        {
            SaveSub->DeleteSave(SlotName);
            RefreshSlotList();
        }
    }
}
