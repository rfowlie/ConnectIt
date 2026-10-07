// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_TilePlaceableRule.generated.h"


// Whether a piece may be placed on a tile. A plain, thread-safe rule struct
// (see the note at the top of ConnectIt_ScoringRule.h).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTilePlaceableRule
{
    GENERATED_BODY()

    virtual ~FConnectItTilePlaceableRule() = default;

    // TileIndex is an index into Board.TileDataArray / TilePositions (always
    // valid when called). By index, not position, because position lookups
    // are linear and the AI's search asks this for every tile of every
    // position it considers; FConnectItRuleSet::IsTilePlaceable resolves a
    // position for callers that have one.
    virtual bool IsTilePlaceable(const FConnectItBoardState& Board, int32 TileIndex) const
    {
        return false;
    }
};

// The classic rule: the tile is active and has nothing on it.
USTRUCT(BlueprintType, meta = (DisplayName = "Unoccupied"))
struct CONNECTIT_API FConnectItTilePlaceableRule_Unoccupied : public FConnectItTilePlaceableRule
{
    GENERATED_BODY()

    virtual bool IsTilePlaceable(const FConnectItBoardState& Board, int32 TileIndex) const override
    {
        const FConnectItTileData& Tile = Board.GetTileDataAt(TileIndex);
        return Tile.bIsActive && !Tile.bIsOccupied;
    }
};
