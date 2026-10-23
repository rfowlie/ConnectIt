// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_TileReaction.generated.h"

class AConnectIt_TileBase;


// One tile's reaction to one board event: handed to a tile hook
// (AConnectIt_TileBase::OnPowerTriggered...) by the tile visualizer, which
// waits for every reaction it handed out before the event queue moves on.
//
// A hook that overrides must call Finish once its animation ends -- cache the
// reaction, finish it when done (like responding to a game event). A hook
// left alone finishes at once. A second Finish is ignored; a tile that leaves
// play (returned to its pool, destroyed) finishes what it still holds.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_TileReaction : public UObject
{
    GENERATED_BODY()

public:

    // This reaction is done: the event can move on once every other one is too
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Tile Reaction")
    void Finish();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Tile Reaction")
    bool IsFinished() const { return bFinished; }

    // The tile it belongs to
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Tile Reaction")
    TWeakObjectPtr<AConnectIt_TileBase> Tile;

    // Called once, on Finish (the visualizer's count)
    TFunction<void()> OnFinished;

private:

    bool bFinished = false;
};
