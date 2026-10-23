// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ConnectIt_ProjectSettings.generated.h"

class UConnectIt_TileCatalog;
class AConnectIt_BuilderTile;


// Project-wide ConnectIt settings (Project Settings > Game > ConnectIt)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ConnectIt"))
class CONNECTIT_API UConnectIt_ProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    // The tile catalog a level uses when its level config names none
    UPROPERTY(Config, EditAnywhere, Category = "Tiles")
    TSoftObjectPtr<UConnectIt_TileCatalog> DefaultTileCatalog;

    // The builder tile the layout tool places when it loads a layout into a
    // level (a Blueprint of AConnectIt_BuilderTile with its blockout look)
    UPROPERTY(Config, EditAnywhere, Category = "Tiles")
    TSoftClassPtr<AConnectIt_BuilderTile> BuilderTileClass;

    virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
