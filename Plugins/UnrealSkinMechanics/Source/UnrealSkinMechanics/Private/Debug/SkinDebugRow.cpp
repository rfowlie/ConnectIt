// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/SkinDebugRow.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Subsystem/SkinSubsystem.h"

namespace
{
	const FString PresetDefaultOption = TEXT("(preset default)");

	const TCHAR* SourceToString(const ESkinSource Source)
	{
		switch (Source)
		{
		case ESkinSource::LevelForced:    return TEXT("level-forced");
		case ESkinSource::PlayerOverride: return TEXT("override");
		case ESkinSource::PlayerPreset:   return TEXT("preset");
		case ESkinSource::DefaultPreset:  return TEXT("default preset");
		default:                          return TEXT("nothing");
		}
	}
}

void USkinDebugRow::Init(USkinSubsystem* InSubsystem, const FGameplayTag InCategory)
{
	Subsystem = InSubsystem;
	Category = InCategory;

	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	WidgetTree->RootWidget = Root;

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(Category.ToString()));
	UHorizontalBoxSlot* LabelSlot = Root->AddChildToHorizontalBox(Label);
	LabelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	Combo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	Combo->OnSelectionChanged.AddDynamic(this, &USkinDebugRow::HandleSelectionChanged);
	UHorizontalBoxSlot* ComboSlot = Root->AddChildToHorizontalBox(Combo);
	ComboSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UHorizontalBoxSlot* StatusSlot = Root->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetVerticalAlignment(VAlign_Center);

	RefreshAll();
}

void USkinDebugRow::RefreshAll()
{
	if (!Combo || !Subsystem)
	{
		return;
	}

	Combo->ClearOptions();
	Combo->AddOption(PresetDefaultOption);
	for (const FName SkinId : Subsystem->GetAvailableSkinIds(Category))
	{
		Combo->AddOption(SkinId.ToString());
	}

	RefreshStatus();
}

void USkinDebugRow::RefreshStatus()
{
	if (!Combo || !StatusText || !Subsystem)
	{
		return;
	}

	// Programmatic selection fires HandleSelectionChanged with ESelectInfo::Direct, which is ignored.
	const FName Override = Subsystem->GetOverride(Category);
	const FString Selected = Override.IsNone() ? PresetDefaultOption : Override.ToString();
	Combo->SetSelectedOption(Combo->FindOptionIndex(Selected) != INDEX_NONE ? Selected : PresetDefaultOption);

	const FSkinResolution Resolution = Subsystem->GetSkinResolution(Category);
	StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s  (from %s)"),
		Resolution.SkinId.IsNone() ? TEXT("<none>") : *Resolution.SkinId.ToString(),
		SourceToString(Resolution.Source))));
}

void USkinDebugRow::HandleSelectionChanged(FString SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !Subsystem)
	{
		return;
	}

	if (SelectedItem == PresetDefaultOption)
	{
		Subsystem->ClearOverride(Category);
	}
	else
	{
		Subsystem->SetOverride(Category, FName(*SelectedItem));
	}

	RefreshStatus();
}
