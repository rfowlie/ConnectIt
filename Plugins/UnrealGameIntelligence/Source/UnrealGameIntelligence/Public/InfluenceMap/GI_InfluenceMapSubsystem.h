#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapSubsystem.generated.h"

class UGI_InfluenceMapWidget;
class IGI_InfluenceMapVisualiser;

// Broadcasts when a visualiser is selected via the debug UI or programmatically.
// Tag identifies which visualiser was selected; Index is its position in the registry.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGI_OnInfluenceMapVisualiserSelected,
    FGameplayTag, Tag,
    int32,        Index);

/**
 * Central subsystem for the GI Influence Map plugin.
 *
 * Responsibilities:
 *   - Maintains the registry of all registered influence map visualisers
 *   - Manages active visualiser state (Activate / Deactivate)
 *   - Broadcasts selection events when the debug UI or project code selects a visualiser
 *   - Manages the lifetime and visibility of the debug UI widget
 *
 * Access via: GetWorld()->GetSubsystem<UGI_InfluenceMapSubsystem>()
 *
 * The subsystem is intentionally unaware of influence maps and data providers.
 * It operates entirely on IGI_InfluenceMapVisualiser. The project is responsible
 * for wiring maps to visualisers before registration.
 *
 * --- UI Lifetime ---
 *   CreateDebugUI  → creates widget, adds to viewport, visibility = Collapsed
 *   ShowDebugUI    → sets visibility = Visible
 *   HideDebugUI    → sets visibility = Collapsed
 *   DestroyDebugUI → removes from viewport, nulls the instance
 */
UCLASS()
class UNREALGAMEINTELLIGENCE_API UGI_InfluenceMapSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    // -- Lifecycle -----------------------------------------------------------

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // -- Registration --------------------------------------------------------

    /**
     * Registers a visualiser with the subsystem.
     * The visualiser's Tag must be valid and unique within the registry.
     * Returns true if registration succeeded.
     * If the debug UI is currently created, the button list will refresh automatically.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool RegisterVisualiser(TScriptInterface<IGI_InfluenceMapVisualiser> Visualiser);

    /**
     * Removes a visualiser from the registry by tag.
     * If the visualiser is currently active it will be deactivated first.
     * Returns true if the visualiser was found and removed.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool UnregisterVisualiser(FGameplayTag Tag);

    // -- Selection -----------------------------------------------------------

    /**
     * Selects a visualiser by tag.
     * Deactivates the currently active visualiser, activates the selected one,
     * and broadcasts OnVisualiserSelected.
     * Returns false and logs a warning if the tag is not registered.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool SelectVisualiser(FGameplayTag Tag);

    /**
     * Selects a visualiser by its index in the registry.
     * Convenience overload for UI callbacks that operate on index.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool SelectVisualiserByIndex(int32 Index);

    /**
     * Deactivates the currently active visualiser without selecting another.
     * Safe to call when nothing is selected.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    void ClearSelection();

    // -- Queries -------------------------------------------------------------

    /** Returns all registered tags in registry order. Used to populate the UI. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    TArray<FGameplayTag> GetRegisteredTags() const;

    /** Returns the number of registered visualisers. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    int32 GetRegisteredVisualiserCount() const { return Registry.Num(); }

    /** Returns the tag of the currently active visualiser, or an invalid tag if none. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    FGameplayTag GetActiveTag() const { return ActiveTag; }

    /** Returns true if the debug UI widget has been created. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    bool IsDebugUICreated() const { return ActiveWidget != nullptr; }

    // -- UI ------------------------------------------------------------------

    /**
     * Creates the debug widget and adds it to the viewport with Collapsed visibility.
     * Call ShowDebugUI to make it visible.
     * If a widget already exists it will be destroyed and replaced.
     * WidgetClass must be a subclass of UGI_InfluenceMapWidget.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void CreateDebugUI(TSubclassOf<UGI_InfluenceMapWidget> WidgetClass, APlayerController* OwningPlayer);

    /**
     * Removes the debug widget from the viewport and nulls the instance.
     * Safe to call when no widget exists.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void DestroyDebugUI();

    /**
     * Sets the debug widget visibility to Visible.
     * CreateDebugUI must have been called first.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void ShowDebugUI();

    /**
     * Sets the debug widget visibility to Collapsed.
     * The widget instance is kept alive — call DestroyDebugUI to fully remove it.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void HideDebugUI();

    // -- Delegates -----------------------------------------------------------

    /** Fired when a visualiser is selected via UI or programmatic call. */
    UPROPERTY(BlueprintAssignable, Category = "GI|InfluenceMaps")
    FGI_OnInfluenceMapVisualiserSelected OnVisualiserSelected;

private:

    // -- Internal ------------------------------------------------------------

    UFUNCTION()
    void OnWidgetSelectionChanged(FGameplayTag Tag, int32 Index);

    int32 FindVisualiserIndexByTag(FGameplayTag Tag) const;

    void RefreshWidgetButtons();

    // Ordered registry — insertion order determines UI button order
    TArray<TScriptInterface<IGI_InfluenceMapVisualiser>> Registry;

    // Tag of the currently active visualiser
    FGameplayTag ActiveTag;

    // Currently active debug widget — subsystem owns this instance
    UPROPERTY()
    TObjectPtr<UGI_InfluenceMapWidget> ActiveWidget;
};