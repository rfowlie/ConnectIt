// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tile/GridTileBase.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_TileBase.generated.h"


// ConnectIt's tile actor base. The project's tile Blueprints derive from it.
//
// A tile's power is part of what the tile IS: each powered kind of tile is a
// subclass (a Push tile, a Chained tile...) that names its power once, in
// Class Defaults (Tile Power Tag), and builds the power from its own exposed
// variables (Get Tile Power) -- the class decides what the power is, each
// placed tile (or layout cell) tunes how much. When the tile joins the board
// the power is copied onto its tile data and travels with it from then on
// (FConnectItTileData::TilePower); the board never asks the actor again.
//
// Per-tile settings that aren't the tile's identity (starting piece,
// multiplier, active, shiftable...) stay on the placed tile, through
// IConnectIt_TileDataProvider as before.
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_TileBase : public AGridTileBase
{
    GENERATED_BODY()

public:

    // Which power this kind of tile has (ConnectIt.TilePower.*). Set once per
    // subclass, in Class Defaults: not editable on placed tiles, never changed
    // at runtime. Empty = no power.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Power", meta = (Categories = "ConnectIt.TilePower"))
    FGameplayTag TilePowerTag;

    // The tile's full power: trigger, gate, cooldown, effect and its values.
    // A powered subclass overrides this, building the power from its own
    // exposed variables; its Name must be Tile Power Tag. The base has none.
    // CallInEditor: placed tiles in an editor world answer too (validation,
    // the layout tool's capture) -- keep it free of gameplay side effects.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tile Power", meta = (CallInEditor = "true"))
    FConnectItTilePower GetTilePower() const;
    virtual FConnectItTilePower GetTilePower_Implementation() const { return FConnectItTilePower(); }

    // The power this tile brings to the board: Get Tile Power under the
    // class's Tile Power Tag (the tag is the identity), or none when the class
    // has no tag. OutProblem (if given) gets what doesn't match -- a power
    // named differently from the tag, a tag with no power or no effect.
    FConnectItTilePower MakeBoardPower(FString* OutProblem = nullptr) const;

    // What's wrong with a power under a class tag: the checks MakeBoardPower
    // and the layout's validation share. Empty = nothing.
    static FString CheckTilePower(const FGameplayTag& ClassTag, const FConnectItTilePower& Power);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
