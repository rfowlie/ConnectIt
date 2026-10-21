// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameMode/ConnectIt_LevelChange.h"


bool FConnectItLevelChangeQueue::Add(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing, bool bBusy)
{
    if (Timing == EConnectItChangeTiming::NextTurnStart)
    {
        NextTurn.Add(Change);
        return false;
    }
    if (bBusy)
    {
        AfterMove.Add(Change);
        return false;
    }
    return true;
}

TArray<FConnectItLevelChange> FConnectItLevelChangeQueue::TakeAfterMove()
{
    TArray<FConnectItLevelChange> Taken = MoveTemp(AfterMove);
    AfterMove.Reset();
    return Taken;
}

TArray<FConnectItLevelChange> FConnectItLevelChangeQueue::TakeNextTurn()
{
    TArray<FConnectItLevelChange> Taken = MoveTemp(NextTurn);
    NextTurn.Reset();
    return Taken;
}
