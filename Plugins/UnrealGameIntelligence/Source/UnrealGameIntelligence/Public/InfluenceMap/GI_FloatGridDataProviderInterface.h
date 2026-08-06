#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Math/IntPoint.h"
#include "GI_FloatGridDataProviderInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UGI_FloatGridDataProviderInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Example data provider interface for influence maps that produce
 * a float score per grid position.
 *
 * --- Purpose ---
 * This interface illustrates the data provider pattern used by the GI
 * Influence Map plugin. It sits between an influence map and a visualiser,
 * allowing a visualiser to query data without knowing which specific map
 * produced it.
 *
 * --- Pattern ---
 * 1. An influence map implements this interface alongside IGI_InfluenceMap:
 *
 *      class UMyGame_ThreatMap
 *          : public UObject
 *          , public IGI_InfluenceMap           // plugin contract
 *          , public IGI_FloatGridDataProvider  // data shape contract
 *      {
 *          virtual float GetData(FIntPoint InGridPosition) const override;
 *      };
 *
 * 2. A visualiser exposes a typed setter for this data provider:
 *
 *      class UMyGame_FloatOverlayVisualiser
 *          : public UObject
 *          , public IGI_InfluenceMapVisualiser
 *      {
 *      public:
 *          // Project calls this before registering with the subsystem
 *          void SetDataProvider(TScriptInterface<IGI_FloatGridDataProvider> InProvider)
 *          {
 *              DataProvider = InProvider;
 *          }
 *
 *          virtual void Activate() override
 *          {
 *              // Query via the data provider interface — no cast to a specific map type
 *              for (const FIntPoint& Position : GridPositions)
 *              {
 *                  float Score = DataProvider->GetData(Position);
 *                  // render score at position...
 *              }
 *          }
 *
 *      private:
 *          TScriptInterface<IGI_FloatGridDataProvider> DataProvider;
 *      };
 *
 * 3. The project wires them:
 *
 *      UMyGame_ThreatMap* ThreatMap = NewObject<UMyGame_ThreatMap>(this);
 *      UMyGame_FloatOverlayVisualiser* Vis = NewObject<UMyGame_FloatOverlayVisualiser>(this);
 *      Vis->SetDataProvider(ThreatMap);
 *      Subsystem->RegisterVisualiser(Vis);
 *
 * --- Flexibility ---
 * Because the visualiser only knows about IGI_FloatGridDataProvider, it works
 * with any map that implements this interface regardless of how the map
 * internally calculates its scores. A ThreatMap and a ResourceMap that both
 * produce float scores per grid position can share the same visualiser type.
 *
 * Projects are encouraged to define their own data provider interfaces
 * following this pattern for other data shapes (enumerations, tags, vectors, etc.).
 *
 * --- Note on FIntPoint ---
 * This interface uses FIntPoint to avoid any dependency on project-specific
 * grid position types. Concrete implementations are free to wrap or convert
 * to their own grid coordinate type internally.
 */
class UNREALGAMEINTELLIGENCE_API IGI_FloatGridDataProviderInterface
{
    GENERATED_BODY()

public:

    /**
     * Returns the float influence score at the given grid position.
     * What the score represents is defined by the concrete map implementation.
     * Returns 0.0f for positions outside the map's valid range.
     */
    virtual float GetData(FIntPoint InPoint) const = 0;
};