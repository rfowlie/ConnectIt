// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GridMechanicsSettings.generated.h"

class AActor;

// The plugin's project-wide settings (Project Settings > Plugins > Grid
// Mechanics)
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grid Mechanics"))
class UNREALGRIDMECHANICS_API UGridMechanicsSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    // The channel the cursor traces for grid hover and clicks: tiles block
    // it, pieces ignore it (so the tile under a piece is hovered). The plugin
    // defines "GridCursor" (Config/Engine.ini, slot 18) -- set it here to stop
    // anything else that blocks Visibility from taking the cursor. Default
    // Visibility (as before).
    UPROPERTY(Config, EditAnywhere, Category = "Cursor")
    TEnumAsByte<ECollisionChannel> CursorTraceChannel = ECC_Visibility;

    static ECollisionChannel GetCursorTraceChannel();

    // The channel's name in the collision settings ("GridCursor", "Visibility"...)
    static FString GetCursorTraceChannelName();

    // Sets Actor's root primitive's response to the cursor channel (a grid
    // tile blocks it, a piece ignores it). False -- logged once per class --
    // if the root isn't a primitive.
    static bool SetRootCursorResponse(AActor* Actor, ECollisionResponse Response);

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
