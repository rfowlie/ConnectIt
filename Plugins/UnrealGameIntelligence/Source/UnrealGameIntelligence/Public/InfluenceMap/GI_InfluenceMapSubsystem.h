#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapTypes.h"
#include "GI_InfluenceMapSubsystem.generated.h"

class UGI_InfluenceMapWidget;

// Broadcasts when a map is selected via the debug UI or programmatically.
// Tag identifies which map was selected; Index is its position in the registry.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGI_OnInfluenceMapSelected,
    FGameplayTag, Tag,
    int32,        Index);

/**
 * Central subsystem for the GI Influence Map plugin.
 *
 * Responsibilities:
 *   - Maintains the registry of all registered influence maps and their visualisers
 *   - Broadcasts selection events when the debug UI or project code selects a map
 *   - Manages the lifetime of the debug UI widget
 *   - Provides query access to registered maps by tag or index
 *
 * Access via: GetWorld()->GetSubsystem<UGI_InfluenceMapSubsystem>()
 *
 * Maps are registered by the project at startup (BeginPlay or game mode init).
 * The subsystem does not own registered maps — lifetime is the caller's responsibility.
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
     * Registers an influence map with an optional paired visualiser.
     * The entry's Tag must be valid and unique within the registry.
     * Returns true if registration succeeded, false if the tag is already registered
     * or the entry is invalid.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool RegisterMap(const FGI_InfluenceMapEntry& Entry);

    /**
     * Removes a map from the registry by tag.
     * If the map's visualiser is currently active, it will be deactivated first.
     * Returns true if the map was found and removed.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool UnregisterMap(FGameplayTag Tag);

    // -- Selection -----------------------------------------------------------

    /**
     * Selects a map by tag. Deactivates the currently active visualiser,
     * activates the newly selected map's visualiser, and broadcasts
     * OnInfluenceMapSelected. Safe to call with an unregistered tag —
     * logs a warning and returns false.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool SelectMap(FGameplayTag Tag);

    /**
     * Selects a map by its index in the registry.
     * Convenience overload for UI callbacks that operate on index.
     */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    bool SelectMapByIndex(int32 Index);

    /** Deactivates the currently active visualiser without selecting another. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    void ClearSelection();

    // -- Queries -------------------------------------------------------------

    /** Returns the entry for a given tag, or an invalid entry if not found. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    FGI_InfluenceMapEntry GetEntryByTag(FGameplayTag Tag) const;

    /** Returns all registered tags in registry order. Used to populate the UI. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    TArray<FGameplayTag> GetRegisteredTags() const;

    /** Returns all display text in registry order. Used to populate the UI. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    TArray<FText> GetRegisteredText() const;
    
    /** Returns the number of registered maps. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    int32 GetRegisteredMapCount() const { return Registry.Num(); }

    /** Returns the tag of the currently selected map, or an invalid tag if none. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps")
    FGameplayTag GetActiveTag() const { return ActiveTag; }

    // -- UI ------------------------------------------------------------------

    /**
     * Creates and displays the debug widget using the provided widget class.
     * If a widget is already showing it will be removed and replaced.
     * The subsystem wires the widget's selection delegate internally.
     *
     * WidgetClass must implement the UGI_InfluenceMapWidget contract.
     * Pass a null class to remove the widget without replacing it.
     */

    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void DestroyDebugUI();
    
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void CreateDebugWidget(TSubclassOf<UGI_InfluenceMapWidget> WidgetClass, APlayerController* OwningPlayer);
    
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void ShowDebugUI() const;

    /** Removes the debug widget from the viewport if one is showing. */
    UFUNCTION(BlueprintCallable, Category = "GI|InfluenceMaps|UI")
    void HideDebugUI() const;

    // -- Delegates -----------------------------------------------------------

    /** Fired when a map is selected via UI or programmatic call. */
    UPROPERTY(BlueprintAssignable, Category = "GI|InfluenceMaps")
    FGI_OnInfluenceMapSelected OnInfluenceMapSelected;

private:

    // -- Internal ------------------------------------------------------------

    UFUNCTION()
    void OnWidgetSelectionChanged(FGameplayTag Tag, int32 Index);

    int32 FindEntryIndexByTag(FGameplayTag Tag) const;

    // Ordered registry — order determines UI button order
    TArray<FGI_InfluenceMapEntry> Registry;

    // Tag of the currently active (selected) map
    FGameplayTag ActiveTag;

    // Currently displayed debug widget — subsystem owns this instance
    UPROPERTY()
    TObjectPtr<UGI_InfluenceMapWidget> ActiveWidget;
};
