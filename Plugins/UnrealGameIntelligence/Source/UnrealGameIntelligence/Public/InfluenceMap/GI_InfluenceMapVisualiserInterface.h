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
 * A visualiser is responsible for rendering debug representations of a
 * specific influence map type. It is paired with its map at registration
 * time via FGI_InfluenceMapEntry and activated/deactivated by the subsystem
 * in response to UI selection events.
 *
 * Visualisers are optional. A map can be registered without one if the
 * project only needs to query data programmatically.
 *
 * Concrete visualisers should cast to their paired map type internally.
 * The plugin does not enforce type safety between a map and its visualiser —
 * that relationship is established at registration and is the project's
 * responsibility to wire correctly.
 */
class UNREALGAMEINTELLIGENCE_API IGI_InfluenceMapVisualiser
{
    GENERATED_BODY()

public:

    /**
     * The tag of the map this visualiser knows how to render.
     * Must match the tag of the map it is paired with in FGI_InfluenceMapEntry.
     */
    virtual FGameplayTag GetSupportedTag() const = 0;

    /**
     * Called by the subsystem when this map is selected in the debug UI,
     * or when the project explicitly activates this visualiser.
     * The visualiser should begin rendering its debug representation.
     */
    virtual void Activate() = 0;

    /**
     * Called by the subsystem when another map is selected, or when the
     * project explicitly deactivates this visualiser.
     * The visualiser should stop rendering and clean up any debug draws.
     */
    virtual void Deactivate() = 0;

    /** Whether this visualiser is currently rendering. */
    virtual bool IsActive() const = 0;
};
