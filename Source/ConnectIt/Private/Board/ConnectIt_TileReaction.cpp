// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_TileReaction.h"
#include "Board/Actor/ConnectIt_TileBase.h"


void UConnectIt_TileReaction::Finish()
{
    if (bFinished) return;
    bFinished = true;

    if (AConnectIt_TileBase* Owner = Tile.Get())
    {
        Owner->ForgetReaction(this);
    }
    if (OnFinished)
    {
        // Moved out first: what it calls may finish other reactions
        TFunction<void()> Callback = MoveTemp(OnFinished);
        Callback();
    }
}
