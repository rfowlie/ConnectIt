// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_TilePowerEffects.generated.h"


// The tile power effects ConnectIt ships (each a catalog entry's Effect).
// Test-only effects live with the tests.

// Push (Scoring, ConnectIt.TilePower.Push): pushes one neighbouring piece one
// tile directly away. The candidates are the orthogonal neighbours holding a
// piece whose next tile beyond is one it may move to (Move Piece: empty,
// active, neither end locked); it picks one at random (the board's seed). A
// single candidate is always the one pushed, so a player can see it coming.
// Never its own tile's piece. No candidate: no effect.
USTRUCT(BlueprintType, meta = (DisplayName = "Push"))
struct CONNECTIT_API FConnectItTilePowerEffect_Push : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    // The powered tile's multiplier it needs to fire (0 = none)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FConnectItMultiplierGate Gate;

    virtual FGameplayTag GetPowerTag() const override;
    virtual bool PassesGate(const FConnectItTilePowerContext& Context) const override { return Gate.Passes(Context); }

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override;
};

// Vacuum (Piece Exit + Scoring, ConnectIt.TilePower.Vacuum): when its own tile
// is empty, pulls a piece onto it -- the nearest piece along one of the 8
// directions, picked at random. A look along a direction passes over empty
// tiles and stops at the edge (or a missing tile) or an inactive tile; a piece
// that can't be moved (a locked tile) isn't a candidate.
USTRUCT(BlueprintType, meta = (DisplayName = "Vacuum"))
struct CONNECTIT_API FConnectItTilePowerEffect_Vacuum : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    // The powered tile's multiplier it needs to fire (0 = none)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FConnectItMultiplierGate Gate;

    virtual FGameplayTag GetPowerTag() const override;
    virtual bool PassesGate(const FConnectItTilePowerContext& Context) const override { return Gate.Passes(Context); }

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override;
};

// Repel (Turn Start, ConnectIt.TilePower.Repel; its cooldown is the
// countdown): when the countdown runs out with a piece on the tile, the piece
// moves to a random open tile among the 8 around it (one it may move to:
// empty, active, unlocked). Boxed in, or no piece: nothing, and the countdown
// resets all the same.
USTRUCT(BlueprintType, meta = (DisplayName = "Repel"))
struct CONNECTIT_API FConnectItTilePowerEffect_Repel : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    // The powered tile's multiplier it needs to fire (0 = none)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FConnectItMultiplierGate Gate;

    virtual FGameplayTag GetPowerTag() const override;
    virtual bool PassesGate(const FConnectItTilePowerContext& Context) const override { return Gate.Passes(Context); }

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override;
};

// Chained (a Sync power, ConnectIt.TilePower.Chained): the tile is locked
// exactly while its partner (FConnectItTileData::ChainPartnerId) has no piece.
// Partner has a piece and the tile is locked: unlock it. No partner, or an
// empty one, and the tile is unlocked: remove its piece (if any), then lock
// it. Checks the board, not what happened, so it also follows a partner
// cleared by scoring; changes nothing when the board already agrees.
USTRUCT(BlueprintType, meta = (DisplayName = "Chained"))
struct CONNECTIT_API FConnectItTilePowerEffect_Chained : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    virtual FGameplayTag GetPowerTag() const override;

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override;
};
