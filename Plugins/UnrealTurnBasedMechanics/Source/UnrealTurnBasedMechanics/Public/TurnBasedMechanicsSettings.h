// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TurnBasedMechanicsSettings.generated.h"

class UItemActionInventoryDataAsset;

// The plugin's project-wide settings (Project Settings > Plugins > Turn Based
// Mechanics)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Turn Based Mechanics"))
class UNREALTURNBASEDMECHANICS_API UTurnBasedMechanicsSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    // The game's item action caps: how many of each item action a player can
    // hold. An item action it doesn't list can't be held.
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UItemActionInventoryDataAsset> ItemInventory;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
