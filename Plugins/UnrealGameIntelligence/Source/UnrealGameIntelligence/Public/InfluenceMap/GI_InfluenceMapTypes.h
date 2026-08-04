#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GI_InfluenceMapTypes.generated.h"

class IGI_InfluenceMap;
class IGI_InfluenceMapVisualiser;

/**
 * A single entry in the influence map registry.
 * Pairs a map with its visualiser — the subsystem treats these as a unit.
 * The visualiser is optional; maps can be registered without one if
 * the project only intends to query data rather than render debug visuals.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEINTELLIGENCE_API FGI_InfluenceMapEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    // Raw pointers to interface implementors.
    // Lifetime is the responsibility of whoever registered the entry.
    // The subsystem does not own these objects.
    TScriptInterface<IGI_InfluenceMap>           Map;
    TScriptInterface<IGI_InfluenceMapVisualiser>  Visualiser;

    bool IsValid() const { return Tag.IsValid() && Map != nullptr; }
};
