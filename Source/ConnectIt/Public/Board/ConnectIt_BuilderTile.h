// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Board/ConnectIt_TileBase.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "ConnectIt_BuilderTile.generated.h"


// The tile designers place to block out a level: a placeholder that says
// what tile goes here -- its power tag and per-tile settings (starting piece,
// multiplier, active, shiftable). Never seen in play: when the match starts
// the tile registry swaps each one for its tile catalog's class
// (UConnectIt_TileRegistry::SwapBuilderTiles). In the editor, the layout
// tool's Preview Tile Catalog shows them as the catalog's classes without
// saving the swap.
//
// A Blueprint subclass gives it a blockout look (e.g. showing the tag).
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_BuilderTile : public AConnectIt_TileBase, public IConnectIt_TileDataProvider
{
    GENERATED_BODY()

public:

    // What tile goes here: its power tag (the catalog's entry) and starting
    // values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder Tile", meta = (ShowOnlyInnerProperties))
    FConnectItTileInitialData TileData;

    // Optional: a look for the catalog's tile actor to pick (e.g. cracked)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder Tile")
    FGameplayTag VisualVariant;

    virtual FConnectItTileInitialData GetInitialTileData_Implementation() const override { return TileData; }
    virtual void ApplyLayoutCell_Implementation(const FConnectItTileInitialData& Data, FGameplayTag InVisualVariant) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
