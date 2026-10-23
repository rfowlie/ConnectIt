// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tile/GridTileBase.h"
#include "ConnectIt_TileBase.generated.h"


// ConnectIt's tile actor base: builder tiles (AConnectIt_BuilderTile, what
// designers place) and the tile classes a tile catalog slots in derive from
// it. A tile's power isn't on the actor: a builder tile names a power tag,
// the catalog defines it and picks the actor that shows it, and the board
// carries the tag (FConnectItTileData::TilePowerTag).
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_TileBase : public AGridTileBase
{
    GENERATED_BODY()
};
