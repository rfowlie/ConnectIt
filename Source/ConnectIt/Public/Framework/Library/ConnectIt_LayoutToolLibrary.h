// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_LayoutToolLibrary.generated.h"

class UConnectIt_BoardLayout;

// The building blocks of the board layout tool: an editor utility widget,
// used in a sandbox level, that lays tiles out and saves them as a
// UConnectIt_BoardLayout (and loads one back to edit). Editor only. The
// sandbox map needs a level config in its World Settings: positions convert
// through that board's grid definition, and its Default Tile Class is used for
// cells that name none.
//
// What the widget does itself: pick or create the layout asset, read the
// editor camera (Unreal Editor Subsystem > Get Level Viewport Camera Info) for
// Set Layout Camera, and save the asset (Editor Asset Library > Save Loaded
// Asset). These change the asset in memory and mark it dirty; none of them
// can be undone with Ctrl+Z.
UCLASS()
class CONNECTIT_API UConnectIt_LayoutToolLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

#if WITH_EDITOR
    // Replaces Layout's cells with the tiles placed in this level: each one's
    // grid position, starting values (from the tile data interface; defaults
    // for a tile without it) and class (left empty when it is the board's
    // Default Tile Class). A cell keeps the Visual Variant it already had at
    // that position. Cells are sorted by row, then column. Returns how many
    // cells were captured (-1 if the level has no board to convert positions
    // with).
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static int32 CaptureLayoutFromLevel(const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout);

    // Places Layout's tiles in this level to edit: each cell's class (else the
    // board's Default Tile Class) at its grid position, given its starting
    // values and look (the tile data interface's Apply Layout Cell -- a tile
    // without it keeps its own). bClearLevelTiles removes the level's tiles
    // first. Returns how many tiles were placed.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static int32 LoadLayoutIntoLevel(const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout,
        bool bClearLevelTiles = true);

    // Sets (bHasCamera) or clears Layout's camera
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool")
    static void SetLayoutCamera(UConnectIt_BoardLayout* Layout, bool bHasCamera, const FTransform& CameraTransform);
#endif
};
