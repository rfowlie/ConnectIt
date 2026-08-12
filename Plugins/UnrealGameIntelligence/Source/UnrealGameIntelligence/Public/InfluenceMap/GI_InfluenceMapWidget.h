#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapWidget.generated.h"

// Fired when the user selects a map button.
// Tag identifies the selected map; Index is its position in the registry.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGI_OnInfluenceMapWidgetSelectionChanged,
    FGameplayTag, Tag,
    int32,        Index);

/**
 * Abstract base for the GI Influence Map debug widget.
 *
 * Contract:
 *   - SetupButtons receives an ordered list of tags from the subsystem
 *     and is responsible for building the button list in the UI
 *   - When a button is pressed, OnSelectionChanged must be broadcast
 *     with the selected tag and its index in the provided list
 *
 * Projects create a concrete UMG widget subclass that implements SetupButtons
 * and wires button click events to broadcast OnSelectionChanged. The subsystem
 * binds to OnSelectionChanged automatically when the widget is shown via
 * UGI_InfluenceMapSubsystem::ShowDebugUI.
 *
 * The widget does not hold a reference to the subsystem. All communication
 * flows outward via OnSelectionChanged — the subsystem listens and responds.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class UNREALGAMEINTELLIGENCE_API UGI_InfluenceMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    /**
     * Called by the subsystem to populate the widget with map buttons.
     * Implement in a concrete subclass to build the button list.
     * Called on initial show and again whenever the registry changes
     * (map registered or unregistered while the widget is visible).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "GI|InfluenceMaps|UI")
    void SetupButtons(const TArray<FGameplayTag>& Tags);

    /**
     * Broadcast this when a button is pressed.
     * Tag must match the tag passed into SetupButtons for that button.
     * Index must match the button's position in the Tags array.
     * The subsystem listens to this delegate to drive map selection and
     * visualiser activation.
     */
    UPROPERTY(BlueprintAssignable, Category = "GI|InfluenceMaps|UI")
    FGI_OnInfluenceMapWidgetSelectionChanged OnSelectionChanged;
};
