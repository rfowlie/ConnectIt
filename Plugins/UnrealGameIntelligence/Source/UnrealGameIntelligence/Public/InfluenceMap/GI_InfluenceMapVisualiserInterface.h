#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapVisualiserInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UGI_InfluenceMapVisualiser : public UInterface
{
    GENERATED_BODY()
};

/**
 * Contract for all influence map visualisers in the GI plugin.
 *
 * A visualiser is responsible for rendering a debug representation of
 * influence map data in the world. It is registered directly with the
 * subsystem, which manages its active state via Activate and Deactivate.
 *
 * --- Data Provider Pattern ---
 * Visualisers are decoupled from specific influence map types. Instead of
 * receiving an influence map directly, a concrete visualiser exposes its own
 * typed setter that accepts a data provider interface. The project calls this
 * setter before registering the visualiser with the subsystem.
 *
 * Example:
 *
 *   // Project constructs and wires
 *   UMyGame_ThreatMap* ThreatMap = NewObject<UMyGame_ThreatMap>(this);
 *   UMyGame_FloatOverlayVisualiser* Vis = NewObject<UMyGame_FloatOverlayVisualiser>(this);
 *   Vis->SetFloatGridDataProvider(ThreatMap); // project-specific setter
 *   Subsystem->RegisterVisualiser(Vis);       // subsystem only sees the interface
 *
 *   // When selected, subsystem calls:
 *   Vis->Activate();   // visualiser uses its stored provider reference to render
 *   Vis->Deactivate(); // visualiser stops rendering and cleans up
 *
 * This means:
 *   - One visualiser type can work with any map that implements its data provider
 *   - Multiple maps producing the same data shape share the same visualiser type
 *   - The subsystem remains completely unaware of data shapes or map types
 *
 * See GI_FloatGridDataProviderInterface.h for an example data provider pattern.
 */
class UNREALGAMEINTELLIGENCE_API IGI_InfluenceMapVisualiser
{
    GENERATED_BODY()

public:

    /**
     * Unique tag identifying this visualiser within the subsystem registry.
     * Used to populate the debug UI and drive selection events.
     * Should match the tag of the influence map this visualiser is paired with.
     */
    virtual FGameplayTag GetTag() const = 0;

    /**
     * Human-readable name shown in the debug UI button for this visualiser.
     */
    virtual FText GetDisplayName() const = 0;

    /**
     * Called by the subsystem when this visualiser is selected in the debug UI
     * or activated programmatically via SelectVisualiser.
     *
     * The visualiser should begin rendering its debug representation.
     * It is responsible for querying its stored data provider reference
     * for the data it needs to render.
     *
     * Guaranteed to only be called when IsActive() returns false.
     */
    virtual void Activate() = 0;

    /**
     * Called by the subsystem when another visualiser is selected, or when
     * ClearSelection is called.
     *
     * The visualiser should stop rendering and clean up any debug draws,
     * persistent lines, or world overlays it created during Activate.
     *
     * Guaranteed to only be called when IsActive() returns true.
     */
    virtual void Deactivate() = 0;

    /** Whether this visualiser is currently rendering. */
    virtual bool IsActive() const = 0;
};