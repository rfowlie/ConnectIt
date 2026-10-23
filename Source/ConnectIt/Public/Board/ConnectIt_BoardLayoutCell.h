// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "Templates/SubclassOf.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_BoardLayoutCell.generated.h"

class AGridTileBase;

// One tile of a board layout (UConnectIt_BoardLayout): where it is, what it
// starts with, and which tile actor shows it. The gameplay values (TileData,
// TilePower) are copied into the board's own tile data when the tile is
// added; the class and variant are only for the visuals that spawn it. Plain
// data.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardLayoutCell
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    FGridPosition Position;

    // The tile actor to spawn here; none = the board's Default Tile Class
    // (UConnectIt_LevelConfigDataAsset::DefaultTileClass)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    TSubclassOf<AGridTileBase> TileClass;

    // The tile's starting values: multiplier, active, shiftable, starting piece
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell", meta = (ShowOnlyInnerProperties))
    FConnectItTileInitialData TileData;

    // The tile's power with its values, captured from the placed tile
    // (AConnectIt_TileBase::GetTilePower) by the layout tool. Its name must
    // match the Tile Class's Tile Power Tag. Empty = no power.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    FConnectItTilePower TilePower;

    // Optional: a look for the tile actor to pick (e.g. cracked, mossy) --
    // the actor's own business; gameplay never reads it
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    FGameplayTag VisualVariant;

    // The board tile this cell starts as (its id is the board's to give)
    FConnectItTileData ToTileData(int32 NumFactions) const
    {
        FConnectItTileData Tile = TileData.ToTileData(NumFactions);
        Tile.SetTilePower(TilePower);
        return Tile;
    }
};

// What changes to turn one set of tiles into a layout: the tiles that go and
// the tiles that come. A position in both is left alone (its tile keeps its
// piece and multiplier).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardLayoutDiff
{
    GENERATED_BODY()

    // Tiles to remove, in the order they were given
    UPROPERTY(BlueprintReadOnly, Category = "Layout")
    TArray<FGridPosition> Removed;

    // Tiles to add, in the layout's order
    UPROPERTY(BlueprintReadOnly, Category = "Layout")
    TArray<FConnectItBoardLayoutCell> Added;

    bool IsEmpty() const { return Removed.IsEmpty() && Added.IsEmpty(); }

    // From tiles at CurrentPositions to exactly Cells
    static FConnectItBoardLayoutDiff Between(
        TConstArrayView<FGridPosition> CurrentPositions, TConstArrayView<FConnectItBoardLayoutCell> Cells);
};
