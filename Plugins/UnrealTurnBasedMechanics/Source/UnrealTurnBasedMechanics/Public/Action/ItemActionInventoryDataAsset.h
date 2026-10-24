// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "ItemActionInventoryDataAsset.generated.h"

class UTurnBasedPlayerAction;

// The game's item actions and how many of each a player can hold -- static
// tuning, the same in every match. Every item action has a cap: one this asset
// doesn't list can't be held (an inventory drops it, a grant of it adds
// nothing). Named in Project Settings > Plugins > Turn Based Mechanics > Item
// Inventory (Get).
UCLASS(BlueprintType)
class UNREALTURNBASEDMECHANICS_API UItemActionInventoryDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // Item action -> the most a player can hold (at least 1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (ClampMin = 1))
    TMap<TSubclassOf<UTurnBasedPlayerAction>, int32> MaxHeld;

    // The most of ActionClass a player can hold; 0 for an action this asset
    // doesn't list (it can't be held)
    UFUNCTION(BlueprintPure, Category = "Items")
    int32 GetMaxHeld(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const;

    // The project's (Project Settings), or null
    static const UItemActionInventoryDataAsset* Get();

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
