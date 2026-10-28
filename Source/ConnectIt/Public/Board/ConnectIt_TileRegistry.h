// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tile/GridTileRegistryBase.h"
#include "Board/ConnectIt_BoardLayoutCell.h"
#include "ConnectIt_TileRegistry.generated.h"

class UConnectIt_BoardStateComponent;

// ConnectIt-specific tile registry -- the plugin's UGridTileRegistryBase
// knows nothing about UConnectIt_BoardStateComponent, so board-state access
// lives here rather than in UnrealGridMechanics, which stays project-agnostic.
//
// No injection needed: UConnectIt_BoardRegistrySubsystem constructs this via
// DuplicateObject(Template, this), outered to the subsystem (itself outered
// to the UWorld) -- so GetWorld() already resolves correctly, exactly the
// precondition UConnectIt_BoardRequestMediator::GetBoardState already relies
// on for the same reason. GetBoardState() just pulls on demand rather than
// being pushed a reference at construction time.
UCLASS(Blueprintable)
class CONNECTIT_API UConnectIt_TileRegistry : public UGridTileRegistryBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardStateComponent* GetBoardState() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Board")
    void HandleGameEventComplete(FGameplayTag EventTag);

    // --- A phased level's tiles coming and going (see UConnectIt_BoardLayoutDataAsset) ---
    // The bookkeeping half of following the board: call these from the
    // handlers of the Tiles Added / Tiles Removed board events, on every
    // machine (tile actors are local, not replicated). How tiles appear and
    // disappear is the Blueprint's. Pieces on those tiles go through the piece
    // registry, as usual: Despawn Piece At for each removed position with a
    // piece; Spawn Piece At for each added cell with a starting piece.

    // Spawns a tile actor for each cell -- the tile catalog's class for the
    // cell's power tag (UConnectIt_TileCatalogDataAsset::FindFor), else the level
    // config's Default Tile Class when there is no catalog -- at its grid
    // position. Each registers itself (queries, hover), and a tile
    // implementing the tile data interface gets its cell's values and Visual
    // Variant (Apply Layout Cell). Returns them in cell order (null for a
    // cell with no class to spawn: logged), e.g. to play an appear
    // animation.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    TArray<AGridTileBase*> SpawnLayoutTiles(const TArray<FConnectItBoardLayoutCell>& Cells);

    // Stops tracking the tiles at Positions (queries, hover) and returns their
    // actors, in order (null where there was none). The actors are left in
    // the world: animate them out, then Destroy Actor.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    TArray<AGridTileBase*> UnregisterTilesAt(const TArray<FGridPosition>& Positions);

    // --- Builder tiles becoming the catalog's tiles ---

    // Replaces every builder tile (AConnectIt_BuilderTile) with its tile
    // catalog's class for its power tag, at the same place, given the
    // builder's values and look (Apply Layout Cell); the builder is
    // unregistered and destroyed. Called once the board is known -- the
    // board state component does it when the board is seeded (server) or
    // first arrives (client). Returns how many were swapped.
    int32 SwapBuilderTiles();

    // --- Tiles moving with a shift ---
    // A shift moves whole tiles (their data, and with it their kind and
    // power), so the tile actors move too. Call from the Board Shifted
    // handler with its Start / End Positions, on every machine: the tile
    // actor at each start position becomes the tile at the matching end
    // position (a rotation: every one is read before any is moved). Returns
    // them in order (null where there was none), to animate each to its end
    // position.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    TArray<AGridTileBase*> ShiftTiles(const TArray<FGridPosition>& StartPositions,
        const TArray<FGridPosition>& EndPositions);
};
