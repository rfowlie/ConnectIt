// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tile/GridTileBase.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_TileBase.generated.h"

class UConnectIt_TileReaction;


// ConnectIt's tile actor base: builder tiles (AConnectIt_BuilderTile, what
// designers place) and the tile classes a tile catalog slots in derive from
// it. A tile's power isn't on the actor: a builder tile names a power tag,
// the catalog defines it and picks the actor that shows it, and the board
// carries the tag (FConnectItTileData::TilePowerTag).
//
// Hooks: when a board event about this tile plays, the tile visualizer
// (UConnectIt_TileVisualizer) calls the matching hook with a Reaction. A
// Blueprint that overrides a hook must call Reaction > Finish once its
// animation ends -- the event waits for it. Each hook's default finishes at
// once. More hooks will come as tiles need them.
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_TileBase : public AGridTileBase
{
    GENERATED_BODY()

public:

    // A tile power fired on this tile
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnTilePowerTriggered(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_TileReaction* Reaction);

    // A piece arrived on this tile: placed, swapped in, captured (now
    // Faction's), moved in. Cause: the board event's tag.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnPieceEnter(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction);

    // A piece left this tile: removed, swapped out, moved off. Cause: the
    // board event's tag.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnPieceExit(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction);

    // This tile's multiplier changed: scored through, adjusted, destroyed
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnMultiplierChanged(float NewMultiplier, UConnectIt_TileReaction* Reaction);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnLockChanged(bool bLocked, UConnectIt_TileReaction* Reaction);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnActiveChanged(bool bActive, UConnectIt_TileReaction* Reaction);

    // A Chained tile's partner was set or cleared -- called on the chained
    // tile and on its new partner (Event.Position is the chained one)
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Tile Hooks")
    void OnChainPartnerChanged(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_TileReaction* Reaction);

    // Finishes every reaction this tile still holds -- call when the tile
    // leaves play (returned to a pool); leaving the world does it too
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Tile Hooks")
    void FinishPendingReactions();

    // The visualizer hands Reaction to this tile (it is finished if the tile
    // leaves play first)
    void HoldReaction(UConnectIt_TileReaction* Reaction);
    void ForgetReaction(UConnectIt_TileReaction* Reaction);

protected:

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void OnTilePowerTriggered_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_TileReaction* Reaction);
    virtual void OnPieceEnter_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction);
    virtual void OnPieceExit_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction);
    virtual void OnMultiplierChanged_Implementation(float NewMultiplier, UConnectIt_TileReaction* Reaction);
    virtual void OnLockChanged_Implementation(bool bLocked, UConnectIt_TileReaction* Reaction);
    virtual void OnActiveChanged_Implementation(bool bActive, UConnectIt_TileReaction* Reaction);
    virtual void OnChainPartnerChanged_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_TileReaction* Reaction);

private:

    // Reactions handed to this tile and not yet finished
    UPROPERTY(Transient)
    TArray<TObjectPtr<UConnectIt_TileReaction>> PendingReactions;
};
