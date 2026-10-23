// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_LayoutToolLibrary.generated.h"

class UConnectIt_BoardLayout;
class UConnectIt_TileCatalog;

// The building blocks of the board layout tool: an editor utility widget,
// used in a sandbox level, that lays builder tiles out and saves them as a
// UConnectIt_BoardLayout (and loads one back to edit), and previews a tile
// catalog over a level's builder tiles. Editor only. The sandbox map needs a
// level config in its World Settings: positions convert through that board's
// grid definition.
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
    // Replaces Layout's cells with the tiles placed in this level (builder
    // tiles; a catalog preview's actors are skipped): each one's grid
    // position and starting values, power tag included (from the tile data
    // interface; defaults for a tile without it). A cell keeps the Visual
    // Variant it already had at that position. Cells are sorted by row, then
    // column. Returns how many cells were captured (-1 if the level has no
    // board to convert positions with).
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static int32 CaptureLayoutFromLevel(const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout);

    // Places Layout's tiles in this level to edit, as builder tiles (Project
    // Settings > ConnectIt > Builder Tile Class) at their grid positions,
    // each given its cell's starting values and look. bClearLevelTiles
    // removes the level's tiles first (and any catalog preview). Returns how
    // many tiles were placed.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static int32 LoadLayoutIntoLevel(const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout,
        bool bClearLevelTiles = true);

    // Shows each builder tile in this level as Catalog's tile class for its
    // power tag, to see how the level will look: the catalog's actors are
    // spawned as transient (never saved, hidden from the outliner) and the
    // builder tiles are hidden with the editor's temporary hide (not saved
    // either), so nothing here can reach the saved level. Replaces any
    // preview already showing. Returns how many tiles are previewed.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static int32 PreviewTileCatalog(const UObject* WorldContextObject, const UConnectIt_TileCatalog* Catalog);

    // Removes the catalog preview's actors and shows the builder tiles again
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool", meta = (WorldContext = "WorldContextObject"))
    static void ClearTileCatalogPreview(const UObject* WorldContextObject);

    // Sets (bHasCamera) or clears Layout's camera
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Layout Tool")
    static void SetLayoutCamera(UConnectIt_BoardLayout* Layout, bool bHasCamera, const FTransform& CameraTransform);
#endif
};
