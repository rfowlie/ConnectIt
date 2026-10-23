// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "Templates/SubclassOf.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "ConnectIt_BoardLayoutCell.generated.h"

class AGridTileBase;

// One tile of a board layout (UConnectIt_BoardLayout): where it is and what
// it starts with, power tag included. The values (TileData) are copied into
// the board's own tile data when the tile is added. Which tile actor shows it
// is the tile catalog's to say (by its power tag); the variant is only for
// that actor. Plain data.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardLayoutCell
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    FGridPosition Position;

    // The tile's starting values: multiplier, active, shiftable, starting
    // piece, power tag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell", meta = (ShowOnlyInnerProperties))
    FConnectItTileInitialData TileData;

    // Optional: a look for the tile actor to pick (e.g. cracked, mossy) --
    // the actor's own business; gameplay never reads it
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell")
    FGameplayTag VisualVariant;
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
