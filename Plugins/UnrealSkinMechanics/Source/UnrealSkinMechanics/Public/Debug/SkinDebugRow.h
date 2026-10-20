// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Types/SlateEnums.h"
#include "SkinDebugRow.generated.h"

class UComboBoxString;
class USkinSubsystem;
class UTextBlock;

// One category line of the skin debug panel: category tag, a dropdown of its available skin
// IDs, and the resolved ID with the precedence level it came from. Built entirely in C++.
// One widget per category so each owns its dropdown handler (the delegate carries no payload).
UCLASS()
class UNREALSKINMECHANICS_API USkinDebugRow : public UUserWidget
{
	GENERATED_BODY()

public:

	// Builds the widget tree. Call once, after CreateWidget and before adding to a parent.
	void Init(USkinSubsystem* InSubsystem, FGameplayTag InCategory);

	// Re-populates the dropdown options from the subsystem, then refreshes the status.
	// Not safe to call from inside this row's own selection callback.
	void RefreshAll();

	// Updates the selected option and the resolved-ID text; leaves the option list alone.
	void RefreshStatus();

	UFUNCTION()
	void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

private:

	UPROPERTY(Transient)
	TObjectPtr<USkinSubsystem> Subsystem;

	FGameplayTag Category;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> Combo;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};
