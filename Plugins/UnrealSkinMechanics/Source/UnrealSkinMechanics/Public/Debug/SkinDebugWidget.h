// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Types/SlateEnums.h"
#include "SkinDebugWidget.generated.h"

class UComboBoxString;
class USkinDebugRow;
class USkinSubsystem;
class UTextBlock;
class UVerticalBox;

// Generic, deliberately unstyled panel for checking a skin setup: lists every configured
// category with a dropdown of its available skin IDs and the resolved ID + source, a preset
// dropdown, Save / Revert, and any configuration issues. Built entirely in C++ (no asset),
// and it drives the same USkinSubsystem API a real options menu would.
//
// Toggle it in-game with the console command `SkinMechanics.ToggleDebug`, or call
// ToggleDebugWidget from Blueprint.
UCLASS()
class UNREALSKINMECHANICS_API USkinDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Shows the panel if hidden, removes it if shown. Needs a local player controller.
	UFUNCTION(BlueprintCallable, Category = "Skin|Debug", meta = (WorldContext = "WorldContextObject"))
	static void ToggleDebugWidget(const UObject* WorldContextObject);

	// Builds the widget tree. Call once, after CreateWidget and before adding to the viewport.
	void Init(USkinSubsystem* InSubsystem);

	virtual void NativeDestruct() override;

	// Handlers are public so the dynamic delegates can bind to them.
	UFUNCTION()
	void HandlePresetSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleRevertClicked();

	UFUNCTION()
	void HandleSkinChanged(FGameplayTag Category);

private:

	void RefreshAll();
	void RefreshStatus();

	UPROPERTY(Transient)
	TObjectPtr<USkinSubsystem> Subsystem;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> PresetCombo;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IssuesText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkinDebugRow>> Rows;

	static TWeakObjectPtr<USkinDebugWidget> ActiveInstance;
};
