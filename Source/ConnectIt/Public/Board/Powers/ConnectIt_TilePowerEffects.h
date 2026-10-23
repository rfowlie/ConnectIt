// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_TilePowerEffects.generated.h"


// The tile power effects ConnectIt ships (each a catalog entry's Effect).
// Test-only effects live with the tests.

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

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override;
};
