// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tile/GridTileBase.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_TileBase.generated.h"

class UConnectIt_VisualHandle;


// ConnectIt's tile actor base: builder tiles (AConnectIt_BuilderTile, what
// designers place) and the tile classes a tile catalog slots in derive from
// it. A tile's power isn't on the actor: a builder tile names a power tag,
// the catalog defines it and picks the actor that shows it, and the board
// carries the tag (FConnectItTileData::TilePowerTag).
//
// Hooks: when a board event about this tile plays, the board visualizer
// (AConnectIt_BoardVisualizer) calls the matching hook with a copy of the
// event and its handle. A hook that animates Holds the handle and Releases
// it when done -- the event waits for every hold. Defaults do nothing
// (nothing to wait for).
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_TileBase : public AGridTileBase
{
    GENERATED_BODY()

public:

    // A tile power fired on this tile
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnTilePowerTriggered(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle);

    // A piece arrived at this tile: placed, swapped in, captured (now
    // Faction's), moved in. Event: a copy of the board event that did it --
    // its tag from Get Board Event Tag, its data from the instanced struct
    // (several kinds of event lead here).
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnPieceEnter(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle);

    // A piece left this tile: removed, swapped out, moved off. Event as for
    // On Piece Enter.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnPieceExit(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle);

    // This tile's multiplier changed: adjusted, or -- with a Multiplier
    // Changed made for this tile, its Cause Index that event -- scored
    // through or destroyed
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnMultiplierChanged(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnLockChanged(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnActiveChanged(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle);

    // A Chained tile's partner was set or cleared -- called on the chained
    // tile and on its new partner (Event.Position is the chained one)
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnChainPartnerChanged(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle);

    // The tile caught up with the board: its data on the display board (what
    // the player has seen so far), once a change touching it has played, and
    // when the board is seeded or its layout changes. Set the tile's look
    // from it (multiplier, active, lock, chain, power) -- not timed effects,
    // which belong in the hooks above.
    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Tile Hooks")
    void OnRefreshedFromBoard(const FConnectItTileData& TileData);

    // IGridBoardActor: reads this tile's data from the display board at its
    // position and passes it to OnRefreshedFromBoard (nothing if it isn't on
    // the board)
    virtual void RefreshFromBoard_Implementation() override;

    // Lets go of every hold this tile has on a playing event -- call when
    // the tile leaves play (returned to a pool); leaving the world does it too
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Tile Hooks")
    void ReleaseVisualHolds();

protected:

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnTilePowerTriggered_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnPieceEnter_Implementation(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle);
    virtual void OnPieceExit_Implementation(const FInstancedStruct& Event, int32 Faction, UConnectIt_VisualHandle* Handle);
    virtual void OnMultiplierChanged_Implementation(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnLockChanged_Implementation(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnActiveChanged_Implementation(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle);
    virtual void OnChainPartnerChanged_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle);
};
