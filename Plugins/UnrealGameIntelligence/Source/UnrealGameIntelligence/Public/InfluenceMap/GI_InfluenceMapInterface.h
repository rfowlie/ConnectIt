#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UGI_InfluenceMap : public UInterface
{
    GENERATED_BODY()
};

/**
 * Core contract for all influence maps in the GI plugin.
 *
 * An influence map is responsible for:
 *   - Identifying itself via a tag and display name
 *   - Binding to an update delegate that determines when it recalculates
 *   - Executing its update logic when that delegate fires
 *
 * The plugin does not prescribe what data a map holds or how it exposes it.
 * Concrete implementations are free to return any data shape they need.
 * Querying a map's data is a project-level concern — cast to the concrete
 * type or expose a project-specific query interface on the implementing class.
 *
 * See GI_InfluenceMaps_CompositeMaps.md for guidance on composite maps.
 */
class UNREALGAMEINTELLIGENCE_API IGI_InfluenceMap
{
    GENERATED_BODY()

public:

    /** Unique tag identifying this map within the subsystem registry. */
    virtual FGameplayTag GetTag() const = 0;

    /** Human-readable name shown in the debug UI. */
    virtual FText GetDisplayName() const = 0;

    /**
     * Binds the delegate that controls when this map updates.
     *
     * The map stores this delegate and binds its internal update logic to it.
     * When the delegate fires, the map recalculates its data.
     *
     * The caller is responsible for:
     *   - Deciding when to fire the delegate (tick, timer, gameplay event, etc.)
     *   - Keeping the delegate alive for the map's lifetime
     *
     * For composite maps, see GI_InfluenceMaps_CompositeMaps.md for patterns
     * on how to coordinate update delegates across child maps.
     */
    virtual void SetUpdateDelegate(FSimpleDelegate InUpdateDelegate) = 0;
};
