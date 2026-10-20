// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debug/SkinDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Debug/SkinDebugRow.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Subsystem/SkinSubsystem.h"
#include "UnrealSkinMechanics.h"

TWeakObjectPtr<USkinDebugWidget> USkinDebugWidget::ActiveInstance;

namespace
{
	FAutoConsoleCommandWithWorld ToggleDebugCommand(
		TEXT("SkinMechanics.ToggleDebug"),
		TEXT("Shows or hides the skin debug panel (categories, dropdowns, configuration issues)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			USkinDebugWidget::ToggleDebugWidget(World);
		}));

	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		return Block;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FString& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->AddChild(MakeText(Tree, Label));
		return Button;
	}
}

void USkinDebugWidget::ToggleDebugWidget(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (USkinDebugWidget* Existing = ActiveInstance.Get())
	{
		Existing->RemoveFromParent();	// NativeDestruct restores input
		ActiveInstance.Reset();
		return;
	}

	USkinSubsystem* Skins = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<USkinSubsystem>() : nullptr;
	if (!PC || !Skins)
	{
		UE_LOG(LogSkinMechanics, Warning, TEXT("SkinDebugWidget: needs a local player controller and a USkinSubsystem"));
		return;
	}

	USkinDebugWidget* Widget = CreateWidget<USkinDebugWidget>(PC, USkinDebugWidget::StaticClass());
	Widget->Init(Skins);
	Widget->AddToViewport(1000);
	ActiveInstance = Widget;

	PC->SetShowMouseCursor(true);
	PC->SetInputMode(FInputModeGameAndUI());
}

void USkinDebugWidget::Init(USkinSubsystem* InSubsystem)
{
	Subsystem = InSubsystem;

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	WidgetTree->RootWidget = Root;

	Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Skin Mechanics -- debug")));

	// Preset picker
	UHorizontalBox* PresetLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* PresetLabelSlot = PresetLine->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("Preset")));
	PresetLabelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	PresetLabelSlot->SetVerticalAlignment(VAlign_Center);
	PresetCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	PresetCombo->OnSelectionChanged.AddDynamic(this, &USkinDebugWidget::HandlePresetSelected);
	PresetLine->AddChildToHorizontalBox(PresetCombo);
	Root->AddChildToVerticalBox(PresetLine)->SetPadding(FMargin(0.f, 8.f));

	// One row per category
	RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(RowBox);
	TArray<FGameplayTag> Categories = Subsystem->GetCategories();
	Categories.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	for (const FGameplayTag& Category : Categories)
	{
		USkinDebugRow* Row = CreateWidget<USkinDebugRow>(this, USkinDebugRow::StaticClass());
		Row->Init(Subsystem, Category);
		Rows.Add(Row);
		RowBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));
	}

	// Save / Revert
	UHorizontalBox* ButtonLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UButton* SaveButton = MakeButton(WidgetTree, TEXT("Save"));
	SaveButton->OnClicked.AddDynamic(this, &USkinDebugWidget::HandleSaveClicked);
	ButtonLine->AddChildToHorizontalBox(SaveButton)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	UButton* RevertButton = MakeButton(WidgetTree, TEXT("Revert"));
	RevertButton->OnClicked.AddDynamic(this, &USkinDebugWidget::HandleRevertClicked);
	ButtonLine->AddChildToHorizontalBox(RevertButton);
	Root->AddChildToVerticalBox(ButtonLine)->SetPadding(FMargin(0.f, 8.f));

	// Configuration issues
	IssuesText = MakeText(WidgetTree, FString());
	IssuesText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	IssuesText->SetAutoWrapText(true);
	Root->AddChildToVerticalBox(IssuesText);

	Subsystem->OnSkinChanged.AddDynamic(this, &USkinDebugWidget::HandleSkinChanged);

	RefreshAll();
}

void USkinDebugWidget::NativeDestruct()
{
	if (Subsystem)
	{
		Subsystem->OnSkinChanged.RemoveDynamic(this, &USkinDebugWidget::HandleSkinChanged);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	Super::NativeDestruct();
}

void USkinDebugWidget::RefreshAll()
{
	if (!Subsystem || !PresetCombo)
	{
		return;
	}

	PresetCombo->ClearOptions();
	for (const FName PresetId : Subsystem->GetAvailablePresetIds())
	{
		PresetCombo->AddOption(PresetId.ToString());
	}

	for (USkinDebugRow* Row : Rows)
	{
		Row->RefreshAll();
	}

	RefreshStatus();
}

void USkinDebugWidget::RefreshStatus()
{
	// Programmatic selection fires HandlePresetSelected with ESelectInfo::Direct, which is ignored.
	const FString Preset = Subsystem->GetPresetId().ToString();
	if (PresetCombo->FindOptionIndex(Preset) != INDEX_NONE)
	{
		PresetCombo->SetSelectedOption(Preset);
	}
	else
	{
		PresetCombo->ClearSelection();
	}

	for (USkinDebugRow* Row : Rows)
	{
		Row->RefreshStatus();
	}

	const TArray<FString> Issues = Subsystem->GetConfigurationIssues();
	if (Issues.IsEmpty())
	{
		IssuesText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		IssuesText->SetText(FText::FromString(FString::Join(Issues, TEXT("\n"))));
		IssuesText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void USkinDebugWidget::HandlePresetSelected(FString SelectedItem, const ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !Subsystem)
	{
		return;
	}

	// OnSkinChanged -> HandleSkinChanged refreshes the rows' status.
	Subsystem->SetPreset(FName(*SelectedItem));
	RefreshStatus();
}

void USkinDebugWidget::HandleSaveClicked()
{
	if (Subsystem)
	{
		Subsystem->SaveSelection();
	}
}

void USkinDebugWidget::HandleRevertClicked()
{
	if (Subsystem)
	{
		Subsystem->ReloadSelection();
		RefreshAll();	// overrides can change without the resolved skin changing
	}
}

void USkinDebugWidget::HandleSkinChanged(FGameplayTag Category)
{
	// Status only: this can fire from inside a dropdown's own selection callback.
	if (Subsystem && PresetCombo)
	{
		RefreshStatus();
	}
}
